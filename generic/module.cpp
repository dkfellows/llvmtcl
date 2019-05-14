/*
 * module.cpp --
 *
 *	This file contains the Tcl commands for working with modules.
 *
 * Copyright (c) 2015-2018 Donal K. Fellows.
 * Copyright (c) 2018 Kevin B. Kenny.
 *
 * See the file "license.terms" for information on usage and redistribution of
 * this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

#include <tcl.h>
#include "llvmtcl.h"
#include <llvm/Transforms/Utils/Cloning.h>
#include <llvm-c/Core.h>
#include <llvm-c/BitReader.h>
#include "version.h"

/*
 * -------------------------------------------------------------------
 *
 * InstallStdio --
 *
 *	Map stdin, stdout, and stderr explicitly; they're usually macros in C
 *	and that makes them normally unavailable from the LLVM level without
 *	using platform-specific hacks.
 *
 * -------------------------------------------------------------------
 */

static void
InstallStdio(
    llvm::Module *mod,
    LLVMExecutionEngineRef eeRef)
{
    auto engine = llvm::unwrap(eeRef);
    auto void_ptr_type = llvm::Type::getInt8PtrTy(mod->getContext());

    engine->addGlobalMapping(llvm::cast<llvm::GlobalVariable>(
	    mod->getOrInsertGlobal("stdin", void_ptr_type)), stdin);
    engine->addGlobalMapping(llvm::cast<llvm::GlobalVariable>(
	    mod->getOrInsertGlobal("stdout", void_ptr_type)), stdout);
    engine->addGlobalMapping(llvm::cast<llvm::GlobalVariable>(
	    mod->getOrInsertGlobal("stderr", void_ptr_type)), stderr);
}

/*
 * ------------------------------------------------------------------------
 *
 * PatchRuntimeEnvironment --
 *	UGLY HACK TIME. Replaces a part of the LLVM runtime that's not always
 *	present.
 *
 * ------------------------------------------------------------------------
 */

static void
PatchRuntimeEnvironment(
    LLVMExecutionEngineRef eeRef)
{
    auto engine = llvm::unwrap(eeRef);
    const void *powidf2 = (const void *) __powidf2;
    engine->addGlobalMapping("__powidf2", (uint64_t) powidf2);
}

/*
 * ------------------------------------------------------------------------
 *
 * CreateMCJITCompilerForModule --
 *	Implements a Tcl command for creating a machine-code-issuing compiler
 *	for a module.
 *
 * ------------------------------------------------------------------------
 */

int
CreateMCJITCompilerForModule(
    ClientData clientData,
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const objv[])
{
    if (objc < 2 || objc > 4) {
        Tcl_WrongNumArgs(interp, 1, objv, "M ?OptLevel? ?EnableStdio?");
        return TCL_ERROR;
    }

    llvm::Module *mod;
    if (GetModuleFromObj(interp, objv[1], mod) != TCL_OK)
        return TCL_ERROR;
    int level = 0;
    if (objc >= 3 && Tcl_GetIntFromObj(interp, objv[2], &level) != TCL_OK)
	return TCL_ERROR;
    int enableStdio = 1;
    if (objc >= 4 && Tcl_GetBooleanFromObj(interp, objv[3], &enableStdio) != TCL_OK)
	return TCL_ERROR;

    LLVMMCJITCompilerOptions options;
    LLVMInitializeMCJITCompilerOptions(&options, sizeof(options));
    options.OptLevel = (unsigned) level;

    LLVMExecutionEngineRef eeRef = 0; // output argument (engine)
    char *error = 0; // output argument (error message)
    if (LLVMCreateMCJITCompilerForModule(&eeRef, llvm::wrap(mod),
	    &options, sizeof(options), &error)) {
	SetStringResult(interp, error);
	return TCL_ERROR;
    }

    // Replace a part of the LLVM runtime that's not always present
    PatchRuntimeEnvironment(eeRef);
    // Map stdin, stdout, and stderr explicitly; they're usually macros in C
    // and that makes them normally unavailable from the LLVM level without
    // using platform-specific hacks.
    if (enableStdio)
	InstallStdio(mod, eeRef);

    Tcl_SetObjResult(interp, SetLLVMExecutionEngineRefAsObj(interp, eeRef));
    return TCL_OK;
}

/*
 * ------------------------------------------------------------------------
 *
 * GetHostTriple --
 *	Implements a Tcl command for retrieving the description of the host
 *	platform.
 *
 * ------------------------------------------------------------------------
 */

int
GetHostTriple(
    ClientData clientData,
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const objv[])
{
    if (objc != 1) {
	Tcl_WrongNumArgs(interp, 1, objv, "");
	return TCL_ERROR;
    }
    SetStringResult(interp, llvm::sys::getProcessTriple());
    return TCL_OK;
}

/*
 * ------------------------------------------------------------------------
 *
 * CopyModuleFromModule --
 *	Implements a Tcl command for duplicating a module.
 *
 * ------------------------------------------------------------------------
 */

int
CopyModuleFromModule(
    ClientData clientData,
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const objv[])
{
    if (objc < 2 || objc > 4) {
        Tcl_WrongNumArgs(interp, 1, objv, "M ?NewModuleID? ?NewSourceFile?");
        return TCL_ERROR;
    }
    llvm::Module *srcmod;
    if (GetModuleFromObj(interp, objv[1], srcmod) != TCL_OK)
        return TCL_ERROR;
#ifdef API_7
    auto tgtmod = llvm::CloneModule(*srcmod);
#else // !API_7
    auto tgtmod = llvm::CloneModule(srcmod);
#endif // API_7
    if (objc > 2) {
	std::string tgtid = Tcl_GetString(objv[2]);
	tgtmod->setModuleIdentifier(tgtid);
    }
    if (objc > 3) {
	std::string tgtfile = Tcl_GetString(objv[3]);
	tgtmod->setSourceFileName(tgtfile);
    }
    Tcl_SetObjResult(interp, NewObj(interp, tgtmod.release()));
    return TCL_OK;
}

/*
 * ------------------------------------------------------------------------
 *
 * CreateModuleFromBitcode --
 *	Implements a Tcl command for loading a module from a file.
 *
 * ------------------------------------------------------------------------
 */

int
CreateModuleFromBitcode(
    ClientData clientData,
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const objv[])
{
    char *msg = nullptr;
    LLVMMemoryBufferRef buffer = nullptr;
    LLVMModuleRef module = nullptr;
    LLVMBool failed;

    if (objc != 2) {
	Tcl_WrongNumArgs(interp, 1, objv, "Filename");
	return TCL_ERROR;
    }

    if (LLVMCreateMemoryBufferWithContentsOfFile(Tcl_GetString(objv[1]),
	    &buffer, &msg))
	goto error;
    failed = LLVMParseBitcode(buffer, &module, &msg);
    LLVMDisposeMemoryBuffer(buffer);
    if (failed)
	goto error;

    Tcl_SetObjResult(interp, NewObj(interp, module));
    return TCL_OK;

  error:
    SetStringResult(interp, msg);
    free(msg);
    return TCL_ERROR;
}

/*
 * ------------------------------------------------------------------------
 *
 * GarbageCollectUnusdFunctionsInModule --
 *	Implements a Tcl command for removing functions without known uses or
 *	external visibility.
 *
 * ------------------------------------------------------------------------
 */

int
GarbageCollectUnusedFunctionsInModule(
    ClientData clientData,
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const objv[])
{
    llvm::Module *module;

    if (objc != 2) {
	Tcl_WrongNumArgs(interp, 1, objv, "Module");
	return TCL_ERROR;
    }
    if (GetModuleFromObj(interp, objv[1], module) != TCL_OK)
	return TCL_ERROR;

    bool didDeletion;
    do {
	didDeletion = false;
	std::vector<llvm::Function *> to_delete;

	/*
	 * Find functions that are not being used. CAREFUL! Must not delete
	 * any functions that are defined by this module and which are
	 * exported. But we can delete functions that are unreferenced and are
	 * either internal or declarations that are brought in from outside.
	 *
	 * The deletion is done as a two-stage loop so that the iterator over
	 * the list of functions is guaranteed to not have to handle
	 * concurrent modification trickiness.
	 */

	for (auto &fn : module->functions())
	    if (fn.user_empty() &&
		    (fn.getVisibility() == llvm::GlobalValue::HiddenVisibility
		    || fn.isDeclaration()))
		to_delete.push_back(&fn);

	for (auto f : to_delete) {
	    f->eraseFromParent();
	    didDeletion = true;
	}

	/*
	 * Note: there's a finite number of functions to start with, so this
	 * loop MUST terminate. But we must loop: deleting a function might
	 * have made more functions become unreferenced.
	 */
    } while (didDeletion);
    return TCL_OK;
}

/*
 * ------------------------------------------------------------------------
 *
 * MakeTargetMachine --
 *	Implements a Tcl command for creating a machine descriptor that
 *	platform-specific code can be issued against.
 *
 * ------------------------------------------------------------------------
 */

int
MakeTargetMachine(
    ClientData clientData,
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const objv[])
{
    if (objc < 1 || objc > 2) {
	Tcl_WrongNumArgs(interp, 1, objv, "?Target?");
	return TCL_ERROR;
    }
    const char *triple = LLVMTCL_TARGET;
    if (objc > 1)
	triple = Tcl_GetString(objv[1]);
    if (triple[0] == '\0')
	triple = LLVMTCL_TARGET;

    LLVMTargetRef target;
    char *err;
    if (LLVMGetTargetFromTriple(triple, &target, &err)) {
	SetStringResult(interp, err);
	LLVMDisposeMessage(err);
	return TCL_ERROR;
    }

    auto level = LLVMCodeGenLevelAggressive;
    const char *cpu = llvm::sys::getHostCPUName().data();
    const char *features = "";
    auto targetMachine = LLVMCreateTargetMachine(
	    target, triple, cpu, features, level, LLVMRelocPIC,
	    LLVMCodeModelDefault);
    Tcl_SetObjResult(interp, SetLLVMTargetMachineRefAsObj(
	    nullptr, targetMachine));
    return TCL_OK;
}

/*
 * ------------------------------------------------------------------------
 *
 * WriteModuleMachineCodetoFile --
 *	Implements a Tcl command for converting a module to assembly code or
 *	an object file.
 *
 * ------------------------------------------------------------------------
 */

int
WriteModuleMachineCodeToFile(
    ClientData clientData,
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const objv[])
{
    llvm::Module *module;

    if (objc < 3 || objc > 4) {
	Tcl_WrongNumArgs(interp, 1, objv,
		"Module ObjectFile ?assembly|object?");
	return TCL_ERROR;
    }
    if (GetModuleFromObj(interp, objv[1], module) != TCL_OK)
	return TCL_ERROR;

    auto file = Tcl_GetString(objv[2]);
    auto dumpType = LLVMObjectFile;

    if (objc > 3) {
	static const char *types[] = {
	    "assembly", "object", NULL
	};
	int idx;
	if (Tcl_GetIndexFromObj(interp, objv[3], types, "code type", 0,
		&idx) != TCL_OK)
	    return TCL_ERROR;
	switch (idx) {
	case 0: dumpType = LLVMAssemblyFile; break;
	case 1: dumpType = LLVMObjectFile; break;
	}
    }

    std::string triple = module->getTargetTriple();
    LLVMTargetRef target;
    char *err;
    if (LLVMGetTargetFromTriple(triple.c_str(), &target, &err)) {
	SetStringResult(interp, err);
	LLVMDisposeMessage(err);
	return TCL_ERROR;
    }
    auto level = LLVMCodeGenLevelAggressive;
    const char *cpu = llvm::sys::getHostCPUName().data();
    const char *features = "";
    auto targetMachine = LLVMCreateTargetMachine(
	    target, triple.c_str(), cpu, features, level, LLVMRelocPIC,
	    LLVMCodeModelDefault);
    if (LLVMTargetMachineEmitToFile(targetMachine, llvm::wrap(module),
	    file, dumpType, &err)) {
	SetStringResult(interp, err);
	LLVMDisposeMessage(err);
	LLVMDisposeTargetMachine(targetMachine);
	return TCL_ERROR;
    }
    LLVMDisposeTargetMachine(targetMachine);
    return TCL_OK;
}

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 4
 * fill-column: 78
 * tab-width: 8
 * End:
 */
