/*
 * llvmtcl.h --
 *
 *	This file contains internal cross-file API definitions for llvmtcl. It
 *	is not a public header file.
 *
 * Copyright (c) 2010-2016 Jos Decoster.
 * Copyright (c) 2014-2018 Donal K. Fellows.
 * Copyright (c) 2018 Kevin B. Kenny.
 *
 * See the file "license.terms" for information on usage and redistribution of
 * this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

#include "tcl.h"
#include <string>
#include "llvm/IR/DIBuilder.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Value.h"
#include "llvm/ExecutionEngine/ExecutionEngine.h"

#ifndef MODULE_SCOPE
#define MODULE_SCOPE extern
#endif

MODULE_SCOPE int	GetBasicBlockFromObj(Tcl_Interp *interp,
			    Tcl_Obj *obj, llvm::BasicBlock *&block);
MODULE_SCOPE int	GetBuilderFromObj(Tcl_Interp *interp, Tcl_Obj *obj,
			    llvm::IRBuilder<> *&builder);
MODULE_SCOPE int	GetDIBuilderFromObj(Tcl_Interp *interp, Tcl_Obj *obj,
			    llvm::DIBuilder *&ref);
MODULE_SCOPE int	GetEngineFromObj(Tcl_Interp *interp, Tcl_Obj *obj,
			    llvm::ExecutionEngine *&engine);
MODULE_SCOPE int	GetModuleFromObj(Tcl_Interp *interp, Tcl_Obj *obj,
			    llvm::Module *&module);
MODULE_SCOPE std::string GetRefName(std::string prefix);
MODULE_SCOPE int	GetTypeFromObj(Tcl_Interp *interp, Tcl_Obj *obj,
			    llvm::Type *&type);
MODULE_SCOPE int	GetValueFromObj(Tcl_Interp *interp, Tcl_Obj *obj,
			    llvm::Value *&module);
MODULE_SCOPE Tcl_Obj *	NewObj(llvm::Value *value);

extern "C" double	__powidf2(double a, int b);

#define DECL_CMD(cName) \
    MODULE_SCOPE int cName(ClientData clientData, Tcl_Interp *interp, \
	    int objc, Tcl_Obj *const objv[])

// debuginfo.cpp
DECL_CMD(AttachToFunction);
DECL_CMD(BuildDbgValue);
DECL_CMD(CreateDebugBuilder);
DECL_CMD(DefineCompileUnit);
DECL_CMD(DefineFile);
DECL_CMD(DefineAliasType);
DECL_CMD(DefineArrayType);
DECL_CMD(DefineBasicType);
DECL_CMD(DefineFunction);
DECL_CMD(DefineFunctionType);
DECL_CMD(DefineLocal);
DECL_CMD(DefineLocation);
DECL_CMD(DefineNamespace);
DECL_CMD(DefineParameter);
DECL_CMD(DefinePointerType);
DECL_CMD(DefineStructType);
DECL_CMD(DefineUnspecifiedType);
DECL_CMD(DisposeDebugBuilder);
DECL_CMD(SetInstructionLocation);
// testcode.cpp
DECL_CMD(AddLLVMTclTestCommands);
// attributes.cpp
DECL_CMD(AddAttribute);
DECL_CMD(AddFunctionAttr);
DECL_CMD(AddInstrAttribute);
DECL_CMD(GetAttribute);
DECL_CMD(GetFunctionAttr);
DECL_CMD(RemoveAttribute);
DECL_CMD(RemoveFunctionAttr);
DECL_CMD(RemoveInstrAttribute);
// intrinsics.cpp
DECL_CMD(GetIntrinsicDefinition);
DECL_CMD(GetIntrinsicID);
// verify.cpp
DECL_CMD(VerifyFunction);
DECL_CMD(VerifyModule);
// module.cpp
DECL_CMD(CopyModuleFromModule);
DECL_CMD(CreateMCJITCompilerForModule);
DECL_CMD(CreateModuleFromBitcode);
DECL_CMD(GarbageCollectUnusedFunctionsInModule);
DECL_CMD(GetHostTriple);
DECL_CMD(MakeTargetMachine);
DECL_CMD(WriteModuleMachineCodeToFile);
// execute.cpp
DECL_CMD(CreateGenericValueOfTclInterp);
DECL_CMD(CreateGenericValueOfTclObj);
DECL_CMD(GenericValueToTclObj);
DECL_CMD(CallInitialisePackageFunction);

template<typename T>//T subclass of llvm::MDNode
MODULE_SCOPE int	GetMetadataFromObj(Tcl_Interp *interp,
			    Tcl_Obj *obj, const char *typeName,
			    T *&ref);
MODULE_SCOPE int	GetLLVMTypeRefFromObj(Tcl_Interp*, Tcl_Obj*,
			    LLVMTypeRef&);
MODULE_SCOPE int	GetLLVMValueRefFromObj(Tcl_Interp*, Tcl_Obj*,
			    LLVMValueRef&);

static inline void
SetStringResult(
    Tcl_Interp *interp,
    std::string msg)
{
    Tcl_SetObjResult(interp, Tcl_NewStringObj(msg.c_str(), msg.size()));
}

static inline void
SetStringResult(
    Tcl_Interp *interp,
    const char *msg)
{
    Tcl_SetObjResult(interp, Tcl_NewStringObj(msg, -1));
}

static inline void
TrimRight(
    std::string &msg)
{
    while (msg.length() > 0) {
	if (msg[msg.length() - 1] != '\n' && msg[msg.length() - 1] != '\r')
	    break;
	msg.replace(msg.length() - 1, 1, "");
    }
}

namespace tcl {
    typedef Tcl_Obj *value;
}

static inline tcl::value
NewObj(const std::string &str)
{
    return Tcl_NewStringObj(str.c_str(), str.length());
}

static inline tcl::value
NewObj(bool boolean)
{
    return Tcl_NewBooleanObj(boolean);
}

static inline tcl::value
NewObj(const std::vector<tcl::value> &vec)
{
    return Tcl_NewListObj(vec.size(), vec.data());
}

template<typename T>//T subclass of llvm::Type
static inline int
GetTypeFromObj(
    Tcl_Interp *interp,
    Tcl_Obj *obj,
    std::string msg,
    T *&type)
{
    LLVMTypeRef typeref;
    if (GetLLVMTypeRefFromObj(interp, obj, typeref) != TCL_OK)
	return TCL_ERROR;
    if (!(type = llvm::dyn_cast<T>(llvm::unwrap(typeref)))) {
	SetStringResult(interp, msg);
	return TCL_ERROR;
    }
    return TCL_OK;
}

template<typename T>//T subclass of llvm::Value
static inline int
GetValueFromObj(
    Tcl_Interp *interp,
    Tcl_Obj *obj,
    std::string msg,
    T *&value)
{
    LLVMValueRef valref;
    if (GetLLVMValueRefFromObj(interp, obj, valref) != TCL_OK)
	return TCL_ERROR;
    if (!(value = llvm::dyn_cast<T>(llvm::unwrap(valref)))) {
	SetStringResult(interp, msg);
	return TCL_ERROR;
    }
    return TCL_OK;
}

MODULE_SCOPE Tcl_Obj *SetLLVMValueRefAsObj(
	Tcl_Interp *interp, LLVMValueRef ref);
MODULE_SCOPE Tcl_Obj *SetLLVMExecutionEngineRefAsObj(
	Tcl_Interp *interp, LLVMExecutionEngineRef ref);
MODULE_SCOPE Tcl_Obj *SetLLVMTargetMachineRefAsObj(
	Tcl_Interp *interp, LLVMTargetMachineRef ref);
MODULE_SCOPE Tcl_Obj *SetLLVMGenericValueRefAsObj(
	Tcl_Interp *interp, LLVMGenericValueRef ref);
MODULE_SCOPE int GetLLVMGenericValueRefFromObj(
	Tcl_Interp* interp, Tcl_Obj* obj, LLVMGenericValueRef& ref);

static inline Tcl_Obj* SetLLVMValueRefAsObj(
	Tcl_Interp* interp, llvm::Value *value) {
    return SetLLVMValueRefAsObj(interp, llvm::wrap(value));
}

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 4
 * fill-column: 78
 * End:
 */
