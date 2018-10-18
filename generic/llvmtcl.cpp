/*
 * llvmtcl.cpp --
 *
 *	This file contains the bulk of the mapping of LLVM into Tcl. It's
 *	built partly from automatically-generated code in the generated
 *	subdirectory.
 *
 * Copyright (c) 2010-2016 Jos Decoster.
 * Copyright (c) 2014-2018 Donal K. Fellows.
 * Copyright (c) 2018 Kevin B. Kenny.
 *
 * See the file "license.terms" for information on usage and redistribution of
 * this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

#include <tcl.h>
#include <tclTomMath.h>
#include <iostream>
#include <sstream>
#include <map>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Support/TargetRegistry.h>
#include <llvm/Support/Host.h>

#include "version.h"
#include <llvm/IR/PassManager.h>

#include <llvm/Transforms/IPO/PassManagerBuilder.h>
#include <llvm/Transforms/IPO.h>
#include <llvm/Transforms/Coroutines.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Intrinsics.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/IR/Verifier.h>
#include <llvm/Support/CommandLine.h>
#include <llvm/Support/DynamicLibrary.h>
#include <llvm/ExecutionEngine/ExecutionEngine.h>
#include <llvm/ExecutionEngine/GenericValue.h>
#include <llvm/ExecutionEngine/SectionMemoryManager.h>
#include <llvm-c/Analysis.h>
#include <llvm-c/Core.h>
#include <llvm-c/ExecutionEngine.h>
#include <llvm-c/Target.h>
#include <llvm-c/TargetMachine.h>
#include <llvm-c/BitWriter.h>
#include <llvm-c/BitReader.h>
#include <llvm-c/Transforms/PassManagerBuilder.h>
#include <llvm-c/Transforms/IPO.h>
#include <llvm-c/Transforms/Scalar.h>
#include <llvm-c/Transforms/Vectorize.h>
#include "llvmtcl.h"
#ifdef API_7
#include <llvm-c/Transforms/Utils.h>
#endif // API_7

/*
 * ------------------------------------------------------------------------
 *
 * Missing piece of LLVM C API.
 *
 * ------------------------------------------------------------------------
 */

static void
LLVMAddCoroutinePassesToExtensionPoints(
    LLVMPassManagerBuilderRef ref)
{
    auto Builder = reinterpret_cast<llvm::PassManagerBuilder*>(ref);

    addCoroutinePassesToExtensionPoints(*Builder);
}

/*
 * ------------------------------------------------------------------------
 *
 * Mapping functions, used to represent LLVM values within Tcl. Many are
 * automatically generated.
 *
 * ------------------------------------------------------------------------
 */

TCL_DECLARE_MUTEX(idLock)

std::string GetRefName(std::string prefix)
{
    static volatile int LLVMRef_id = 0;
    int id;
    Tcl_MutexLock(&idLock);
    id = LLVMRef_id++;
    Tcl_MutexUnlock(&idLock);
    std::ostringstream os;
    os << prefix << id;
    return os.str();
}

#include "generated/llvmtcl-gen-map.h"

static std::string
LLVMDumpModuleTcl(
    LLVMModuleRef moduleRef)
{
    std::string s;
    llvm::raw_string_ostream os(s);
    os << *(llvm::unwrap(moduleRef));
    return s;
}

static std::string
LLVMPrintModuleToStringTcl(
    LLVMModuleRef moduleRef)
{
    char *p = LLVMPrintModuleToString(moduleRef);
    std::string s = p;
    LLVMDisposeMessage(p);
    return s;
}

static std::string
LLVMPrintTypeToStringTcl(
    LLVMTypeRef typeRef)
{
    char *p = LLVMPrintTypeToString(typeRef);
    std::string s = p;
    LLVMDisposeMessage(p);
    return s;
}

static std::string
LLVMPrintValueToStringTcl(
    LLVMValueRef valueRef)
{
    char *p = LLVMPrintValueToString(valueRef);
    std::string s = p;
    LLVMDisposeMessage(p);
    return s;
}

static LLVMGenericValueRef
LLVMRunFunction(
    LLVMExecutionEngineRef EE,
    LLVMValueRef F,
    LLVMGenericValueRef *Args,
    unsigned NumArgs)
{
    return LLVMRunFunction(EE, F, NumArgs, Args);
}

MODULE_SCOPE int
GetModuleFromObj(
    Tcl_Interp *interp,
    Tcl_Obj *obj,
    llvm::Module *&module)
{
    LLVMModuleRef modref;
    if (GetLLVMModuleRefFromObj(interp, obj, modref) != TCL_OK)
	return TCL_ERROR;
    module = llvm::unwrap(modref);
    return TCL_OK;
}

MODULE_SCOPE int
GetBasicBlockFromObj(
    Tcl_Interp *interp,
    Tcl_Obj *obj,
    llvm::BasicBlock *&block)
{
    LLVMBasicBlockRef blkref;
    if (GetLLVMBasicBlockRefFromObj(interp, obj, blkref) != TCL_OK)
	return TCL_ERROR;
    block = llvm::unwrap(blkref);
    return TCL_OK;
}

MODULE_SCOPE int
GetBuilderFromObj(
    Tcl_Interp *interp,
    Tcl_Obj *obj,
    llvm::IRBuilder<> *&builder)
{
    LLVMBuilderRef bref;
    if (GetLLVMBuilderRefFromObj(interp, obj, bref) != TCL_OK)
	return TCL_ERROR;
    builder = llvm::unwrap(bref);
    return TCL_OK;
}

static inline Tcl_Obj *
NewObj(
    LLVMValueRef value)
{
    return SetLLVMValueRefAsObj(nullptr, value);
}

Tcl_Obj *
NewObj(
    llvm::Value *value)
{
    auto ref = llvm::wrap(value);
    return SetLLVMValueRefAsObj(nullptr, ref);
}

Tcl_Obj *
NewObj(
    const llvm::Value *value)
{
    auto ref = llvm::wrap(value);
    return SetLLVMValueRefAsObj(nullptr, ref);
}

int
GetTypeFromObj(
    Tcl_Interp *interp,
    Tcl_Obj *obj,
    llvm::Type *&type)
{
    return GetTypeFromObj(interp, obj, "expected type but got type", type);
}

int
GetValueFromObj(
    Tcl_Interp *interp,
    Tcl_Obj *obj,
    llvm::Value *&value)
{
    return GetValueFromObj(interp, obj, "expected value but got value", value);
}

static inline Tcl_Obj *
NewObj(
    llvm::Type *value)
{
    return SetLLVMTypeRefAsObj(nullptr, llvm::wrap(value));
}

static inline Tcl_Obj *
NewObj(
    llvm::BasicBlock *value)
{
    return SetLLVMBasicBlockRefAsObj(nullptr, llvm::wrap(value));
}

int
GetEngineFromObj(
    Tcl_Interp *interp,
    Tcl_Obj *obj,
    llvm::ExecutionEngine *&engine)
{
    LLVMExecutionEngineRef eeref;
    if (GetLLVMExecutionEngineRefFromObj(interp, obj, eeref) != TCL_OK)
	return TCL_ERROR;
    engine = llvm::unwrap(eeref);
    return TCL_OK;
}

/*
 * ------------------------------------------------------------------------
 *
 * Disposal functions
 *
 * ------------------------------------------------------------------------
 */

static void
LLVMDisposeBuilderTcl(
    LLVMBuilderRef builderRef)
{
    LLVMDisposeBuilder(builderRef);
    LLVMBuilderRef_map.erase(LLVMBuilderRef_refmap[builderRef]);
    LLVMBuilderRef_refmap.erase(builderRef);
}

static void
LLVMDisposeModuleTcl(
    LLVMModuleRef moduleRef)
{
    LLVMDisposeModule(moduleRef);
    LLVMModuleRef_map.erase(LLVMModuleRef_refmap[moduleRef]);
    LLVMModuleRef_refmap.erase(moduleRef);
}

static void
LLVMDisposePassManagerTcl(
    LLVMPassManagerRef passManagerRef)
{
    LLVMDisposePassManager(passManagerRef);
    LLVMPassManagerRef_map.erase(LLVMPassManagerRef_refmap[passManagerRef]);
    LLVMPassManagerRef_refmap.erase(passManagerRef);
}

static int
LLVMDeleteFunctionTcl(
    LLVMValueRef functionRef)
{
    LLVMDeleteFunction(functionRef);
    LLVMValueRef_map.erase(LLVMValueRef_refmap[functionRef]);
    LLVMValueRef_refmap.erase(functionRef);
    return TCL_OK;
}

static void
LLVMDeleteBasicBlockTcl(
    LLVMBasicBlockRef basicBlockRef)
{
    LLVMDeleteBasicBlock(basicBlockRef);
    LLVMBasicBlockRef_map.erase( LLVMBasicBlockRef_refmap[basicBlockRef]);
    LLVMBasicBlockRef_refmap.erase(basicBlockRef);
}

/*
 * ------------------------------------------------------------------------
 *
 * ParseCommandLineOptions --
 *	Implements a Tcl command for parsing command line options.
 *
 * ------------------------------------------------------------------------
 */

static int
ParseCommandLineOptions(
    ClientData clientData,
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const objv[])
{
    std::vector<const char*> argv(objc);
    for (int i = 0; i < objc; ++i)
	argv[i] = Tcl_GetString(objv[i]);

    std::string s;
    llvm::raw_string_ostream os(s);
#ifdef API_5
    bool result = llvm::cl::ParseCommandLineOptions(
	    objc, argv.data(), "called from Tcl", &os);
#else // !API_5
    /*
     * Errors are splurged straight to stderr. Marginally better than losing
     * them entirely.
     */
    bool result = llvm::cl::ParseCommandLineOptions(
	    objc, argv.data(), "called from Tcl");
    if (!result)
	os << "failed to parse command line options";
#endif // API_5
    os.flush();
    if (!result) {
	TrimRight(s);
	SetStringResult(interp, s);
	return TCL_ERROR;
    }
    return TCL_OK;
}

/*
 * ------------------------------------------------------------------------
 *
 * AddIncoming --
 *	Implements a Tcl command for adding incoming arcs to a 'phi' node.
 *
 * ------------------------------------------------------------------------
 */

static int
AddIncoming(
    ClientData clientData,
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const objv[])
{
    if (objc != 4) {
        Tcl_WrongNumArgs(interp, 1, objv,
		"PhiNode IncomingValuesList IncomingBlocksList");
        return TCL_ERROR;
    }

    llvm::PHINode *phiNode;
    if (GetValueFromObj(interp, objv[1],
	    "can only add incoming arcs to a phi", phiNode) != TCL_OK)
        return TCL_ERROR;

    int ivobjc = 0;
    Tcl_Obj **ivobjv = 0;
    if (Tcl_ListObjGetElements(interp, objv[2], &ivobjc, &ivobjv) != TCL_OK) {
	SetStringResult(interp, "IncomingValuesList not specified as list");
	return TCL_ERROR;
    }

    int ibobjc = 0;
    Tcl_Obj **ibobjv = 0;
    if (Tcl_ListObjGetElements(interp, objv[3], &ibobjc, &ibobjv) != TCL_OK) {
	SetStringResult(interp, "IncomingBlocksList not specified as list");
	return TCL_ERROR;
    }

    if (ivobjc != ibobjc) {
	SetStringResult(interp,
		"IncomingValuesList and IncomingBlocksList have different length");
	return TCL_ERROR;
    }

    for(int i = 0; i < ivobjc; i++) {
	llvm::Value *value;
	LLVMBasicBlockRef bbref;

	if (GetValueFromObj(interp, ivobjv[i], value) != TCL_OK)
	    return TCL_ERROR;
	if (GetLLVMBasicBlockRefFromObj(interp, ibobjv[i], bbref) != TCL_OK)
	    return TCL_ERROR;
	phiNode->addIncoming(value, llvm::unwrap(bbref));
    }
    return TCL_OK;
}

/*
 * ------------------------------------------------------------------------
 *
 * BuildAggregateRet --
 *	Implements a Tcl command for building an aggregate 'ret' instruction.
 *
 * ------------------------------------------------------------------------
 */

static int
BuildAggregateRet(
    ClientData clientData,
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const objv[])
{
    if (objc != 3) {
        Tcl_WrongNumArgs(interp, 1, objv, "BuilderRef RetValsList");
        return TCL_ERROR;
    }

    LLVMBuilderRef builder = 0;
    if (GetLLVMBuilderRefFromObj(interp, objv[1], builder) != TCL_OK)
        return TCL_ERROR;

    int rvobjc = 0;
    Tcl_Obj **rvobjv = 0;
    if (Tcl_ListObjGetElements(interp, objv[2], &rvobjc, &rvobjv) != TCL_OK) {
	SetStringResult(interp, "RetValsList not specified as list");
	return TCL_ERROR;
    }

    std::vector<LLVMValueRef> returnValues(rvobjc);
    for(int i = 0; i < rvobjc; i++)
	if (GetLLVMValueRefFromObj(interp, rvobjv[i],
		returnValues[i]) != TCL_OK)
	    return TCL_ERROR;

    LLVMValueRef rt = LLVMBuildAggregateRet(builder, returnValues.data(),
	    rvobjc);

    Tcl_SetObjResult(interp, NewObj(rt));
    return TCL_OK;
}

/*
 * ------------------------------------------------------------------------
 *
 * BuildInvoke --
 *	Implements a Tcl command for building an 'invoke' instruction.
 *
 * ------------------------------------------------------------------------
 */

static int
BuildInvoke(
    ClientData clientData,
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const objv[])
{
    if (objc != 7) {
        Tcl_WrongNumArgs(interp, 1, objv,
		"BuilderRef Fn ArgsList ThenBlock CatchBlock Name");
        return TCL_ERROR;
    }

    LLVMBuilderRef builder = 0;
    if (GetLLVMBuilderRefFromObj(interp, objv[1], builder) != TCL_OK)
        return TCL_ERROR;

    LLVMValueRef fn = 0;
    if (GetLLVMValueRefFromObj(interp, objv[2], fn) != TCL_OK)
        return TCL_ERROR;

    int aobjc = 0;
    Tcl_Obj **aobjv = 0;
    if (Tcl_ListObjGetElements(interp, objv[3], &aobjc, &aobjv) != TCL_OK) {
	SetStringResult(interp, "ArgsList not specified as list");
	return TCL_ERROR;
    }

    LLVMBasicBlockRef thenBlock = 0;
    LLVMBasicBlockRef catchBlock = 0;
    std::string name = Tcl_GetStringFromObj(objv[6], 0);

    std::vector<LLVMValueRef> args(aobjc);
    for(int i = 0; i < aobjc; i++)
	if (GetLLVMValueRefFromObj(interp, aobjv[i], args[i]) != TCL_OK)
	    return TCL_ERROR;
    if (GetLLVMBasicBlockRefFromObj(interp, objv[4], thenBlock) != TCL_OK)
        return TCL_ERROR;
    if (GetLLVMBasicBlockRefFromObj(interp, objv[5], catchBlock) != TCL_OK)
        return TCL_ERROR;

    LLVMValueRef rt = LLVMBuildInvoke(builder, fn, args.data(), aobjc,
	    thenBlock, catchBlock, name.c_str());

    Tcl_SetObjResult(interp, NewObj(rt));
    return TCL_OK;
}

/*
 * ------------------------------------------------------------------------
 *
 * GetParamTypes --
 *	Implements a Tcl command for getting the parameter types of a function
 *	type.
 *
 * ------------------------------------------------------------------------
 */

static int
GetParamTypes(
    ClientData clientData,
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const objv[])
{
    if (objc != 2) {
        Tcl_WrongNumArgs(interp, 1, objv, "FunctionTy ");
        return TCL_ERROR;
    }

    llvm::FunctionType *functionType;
    if (GetTypeFromObj(interp, objv[1],
	    "can only get parameter types of function types",
	    functionType) != TCL_OK)
        return TCL_ERROR;

    Tcl_Obj *rtl = Tcl_NewListObj(0, NULL);
    for (auto type : functionType->params())
	Tcl_ListObjAppendElement(NULL, rtl, NewObj(type));

    Tcl_SetObjResult(interp, rtl);
    return TCL_OK;
}

/*
 * ------------------------------------------------------------------------
 *
 * GetParams --
 *	Implements a Tcl command for getting the parameters of a function.
 *
 * ------------------------------------------------------------------------
 */

static int
GetParams(
    ClientData clientData,
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const objv[])
{
    if (objc != 2) {
        Tcl_WrongNumArgs(interp, 1, objv, "Function ");
        return TCL_ERROR;
    }

    llvm::Function *function;
    if (GetValueFromObj(interp, objv[1],
	    "can only get parameters of a function", function) != TCL_OK)
        return TCL_ERROR;

#ifdef API_5
    const auto &args = function->args();
#else // !API_5
    const auto &args = function->getArgumentList();
#endif // API_5

    Tcl_Obj *rtl = Tcl_NewListObj(0, NULL);
    for (auto &value : args)
	Tcl_ListObjAppendElement(NULL, rtl, NewObj(&value));
    Tcl_SetObjResult(interp, rtl);
    return TCL_OK;
}

/*
 * ------------------------------------------------------------------------
 *
 * GetStructElementTypes --
 *	Implements a Tcl command for getting the element types of a structure
 *	type.
 *
 * ------------------------------------------------------------------------
 */

static int
GetStructElementTypes(
    ClientData clientData,
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const objv[])
{
    if (objc != 2) {
        Tcl_WrongNumArgs(interp, 1, objv, "StructTy ");
        return TCL_ERROR;
    }

    llvm::StructType *structType;
    if (GetTypeFromObj(interp, objv[1],
	    "can only get elements of struct types", structType) != TCL_OK)
        return TCL_ERROR;

    Tcl_Obj *rtl = Tcl_NewListObj(0, NULL);
    for (auto &type : structType->elements())
	Tcl_ListObjAppendElement(interp, rtl, NewObj(type));
    Tcl_SetObjResult(interp, rtl);
    return TCL_OK;
}

/*
 * ------------------------------------------------------------------------
 *
 * GetBasicBlocks --
 *	Implements a Tcl command for geting the basic blocks of a function.
 *
 * ------------------------------------------------------------------------
 */

static int
GetBasicBlocks(
    ClientData clientData,
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const objv[])
{
    if (objc != 2) {
        Tcl_WrongNumArgs(interp, 1, objv, "Function ");
        return TCL_ERROR;
    }

    llvm::Function *function;
    if (GetValueFromObj(interp, objv[1],
	    "can only list basic blocks of functions", function) != TCL_OK)
        return TCL_ERROR;

    Tcl_Obj *rtl = Tcl_NewListObj(0, NULL);
    for (auto &BB : *function)
	Tcl_ListObjAppendElement(interp, rtl, NewObj(&BB));

    Tcl_SetObjResult(interp, rtl);
    return TCL_OK;
}

/*
 * ------------------------------------------------------------------------
 *
 * NamedStructType --
 *	Implements a Tcl command for creating named structure types.
 *
 * ------------------------------------------------------------------------
 */

static int
NamedStructType(
    ClientData clientData,
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const objv[])
{
    if (objc != 4) {
        Tcl_WrongNumArgs(interp, 1, objv, "Name ElementTypes Packed");
        return TCL_ERROR;
    }

    std::string name = Tcl_GetString(objv[1]);
    int numTypes = 0;
    LLVMTypeRef *types = 0;
    if (GetListOfLLVMTypeRefFromObj(interp, objv[2], types,
	    numTypes) != TCL_OK)
        return TCL_ERROR;
    int packed = 0;
    if (Tcl_GetIntFromObj(interp, objv[3], &packed) != TCL_OK)
        return TCL_ERROR;

    llvm::Type *rt;
    if (numTypes < 1) {
	rt = llvm::StructType::create(*llvm::unwrap(LLVMGetGlobalContext()),
		name);
    } else {
	llvm::ArrayRef<llvm::Type*> elements(
		llvm::unwrap(types), unsigned(numTypes));
	rt = llvm::StructType::create(elements, name, packed);
    }

    Tcl_SetObjResult(interp, NewObj(rt));
    return TCL_OK;
}

/*
 * ------------------------------------------------------------------------
 *
 * TokenType --
 *	Implements a Tcl command that creates the 'token' type.
 *
 * ------------------------------------------------------------------------
 */

static int
TokenType(
    ClientData clientData,
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const objv[])
{
    if (objc != 1) {
        Tcl_WrongNumArgs(interp, 1, objv, "");
        return TCL_ERROR;
    }

    auto tokenType = llvm::Type::getTokenTy(
	    *llvm::unwrap(LLVMGetGlobalContext()));
    Tcl_SetObjResult(interp, NewObj(tokenType));
    return TCL_OK;
}

/*
 * ------------------------------------------------------------------------
 *
 * ConstNone --
 *	Implements a Tcl command that creates the 'none' constant.
 *
 * ------------------------------------------------------------------------
 */

static int
ConstNone(
    ClientData clientData,
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const objv[])
{
    if (objc != 1) {
        Tcl_WrongNumArgs(interp, 1, objv, "");
        return TCL_ERROR;
    }

    auto noneConstant = llvm::ConstantTokenNone::get(
	    *llvm::unwrap(LLVMGetGlobalContext()));
    Tcl_SetObjResult(interp, NewObj(noneConstant));
    return TCL_OK;
}

/*
 * ------------------------------------------------------------------------
 *
 * Create the commands with automatically-build bindings. Lots of commands...
 *
 * ------------------------------------------------------------------------
 */

#include "generated/llvmtcl-gen.h"

/*
 * ------------------------------------------------------------------------
 *
 * StoreExternalStringInTclVar --
 *	Writes a string defined by the system encoding into a Tcl variable.
 *
 * ------------------------------------------------------------------------
 */

static const char *
StoreExternalStringInTclVar(
    Tcl_Interp *interp,
    const char *tclVariable,
    const char *externString)
{
    Tcl_DString buf;
    Tcl_DStringInit(&buf);
    const auto internString = Tcl_ExternalToUtfDString(
	    Tcl_GetEncoding(interp, NULL), externString, -1, &buf);
    auto result = Tcl_SetVar(interp, tclVariable, internString,
	    TCL_GLOBAL_ONLY|TCL_LEAVE_ERR_MSG);
    Tcl_DStringFree(&buf);
    return result;
}

/*
 * ------------------------------------------------------------------------
 *
 * DefineCommand --
 *	Convenience function that creates Tcl commands.
 *
 * ------------------------------------------------------------------------
 */

static inline void
DefineCommand(
    Tcl_Interp *interp,
    const char *tclName,
    Tcl_ObjCmdProc *cName)
{
    // Theoretically, this can blow up. It only actually does this if
    // the interpreter is being deleted.
    Tcl_CreateObjCommand(interp, tclName, cName, nullptr, nullptr);
}

/*
 * ------------------------------------------------------------------------
 *
 * Llvmtcl_Init, Llvmtcl_SafeInit --
 *	Library entry points.
 *
 * ------------------------------------------------------------------------
 */

#define LLVMObjCmd(tclName, cName)  DefineCommand(interp, "::" tclName, cName)
#define LLVMStrVar(varName, value) \
    do { \
	if (StoreExternalStringInTclVar(interp, "::" varName, value) == NULL)\
	    return TCL_ERROR;\
    } while (false)\

extern "C" {
DLLEXPORT int Llvmtcl_Init(Tcl_Interp *interp)
{
    if (Tcl_InitStubs(interp, TCL_VERSION, 0) == NULL)
	return TCL_ERROR;
    if (Tcl_TomMath_InitStubs(interp, TCL_VERSION) == NULL)
	return TCL_ERROR;
    if (Tcl_PkgRequire(interp, "Tcl", TCL_VERSION, 0) == NULL)
	return TCL_ERROR;
    if (Tcl_PkgProvide(interp, PACKAGE_NAME, PACKAGE_VERSION) != TCL_OK)
	return TCL_ERROR;

    if (LLVMInitializeNativeTarget() || LLVMInitializeNativeAsmPrinter()) {
	SetStringResult(interp, "failed to initialise native target");
	return TCL_ERROR;
    }

#include "generated/llvmtcl-gen-cmddef.h"

    LLVMObjCmd("llvmtcl::ParseCommandLineOptions", ParseCommandLineOptions);
    LLVMObjCmd("llvmtcl::TokenType", TokenType);
    LLVMObjCmd("llvmtcl::ConstNone", ConstNone);
    LLVMObjCmd("llvmtcl::VerifyModule", VerifyModule);
    LLVMObjCmd("llvmtcl::VerifyFunction", VerifyFunction);
    LLVMObjCmd("llvmtcl::CreateGenericValueOfTclInterp",
	    CreateGenericValueOfTclInterp);
    LLVMObjCmd("llvmtcl::CreateGenericValueOfTclObj",
	    CreateGenericValueOfTclObj);
    LLVMObjCmd("llvmtcl::GenericValueToTclObj", GenericValueToTclObj);
    LLVMObjCmd("llvmtcl::AddLLVMTclCommands", AddLLVMTclTestCommands);
    LLVMObjCmd("llvmtcl::AddIncoming", AddIncoming);
    LLVMObjCmd("llvmtcl::BuildAggregateRet", BuildAggregateRet);
    LLVMObjCmd("llvmtcl::BuildInvoke", BuildInvoke);
    LLVMObjCmd("llvmtcl::GetParamTypes", GetParamTypes);
    LLVMObjCmd("llvmtcl::GetParams", GetParams);
    LLVMObjCmd("llvmtcl::GetStructElementTypes", GetStructElementTypes);
    LLVMObjCmd("llvmtcl::GetBasicBlocks", GetBasicBlocks);
    LLVMObjCmd("llvmtcl::CallInitialisePackageFunction",
	    CallInitialisePackageFunction);
    LLVMObjCmd("llvmtcl::GetIntrinsicDefinition", GetIntrinsicDefinition);
    LLVMObjCmd("llvmtcl::GetIntrinsicID", GetIntrinsicID);
    LLVMObjCmd("llvmtcl::NamedStructType", NamedStructType);
    LLVMObjCmd("llvmtcl::CreateMCJITCompilerForModule",
	    CreateMCJITCompilerForModule);
    LLVMObjCmd("llvmtcl::GetHostTriple", GetHostTriple);
    LLVMObjCmd("llvmtcl::CopyModuleFromModule", CopyModuleFromModule);
    LLVMObjCmd("llvmtcl::CreateModuleFromBitcode", CreateModuleFromBitcode);
    LLVMObjCmd("llvmtcl::GarbageCollectUnusedFunctionsInModule",
	    GarbageCollectUnusedFunctionsInModule);
    LLVMObjCmd("llvmtcl::MakeTargetMachine", MakeTargetMachine);
    LLVMObjCmd("llvmtcl::WriteModuleMachineCodeToFile",
	    WriteModuleMachineCodeToFile);
    // Debugging info support
    LLVMObjCmd("llvmtcl::DebugInfo::BuildDbgValue", BuildDbgValue);
    LLVMObjCmd("llvmtcl::DebugInfo::CreateBuilder", CreateDebugBuilder);
    LLVMObjCmd("llvmtcl::DebugInfo::DisposeBuilder", DisposeDebugBuilder);
    LLVMObjCmd("llvmtcl::DebugInfo::CompileUnit", DefineCompileUnit);
    LLVMObjCmd("llvmtcl::DebugInfo::File", DefineFile);
    LLVMObjCmd("llvmtcl::DebugInfo::Namespace", DefineNamespace);
    LLVMObjCmd("llvmtcl::DebugInfo::Location", DefineLocation);
    LLVMObjCmd("llvmtcl::DebugInfo::UnspecifiedType", DefineUnspecifiedType);
    LLVMObjCmd("llvmtcl::DebugInfo::AliasType", DefineAliasType);
    LLVMObjCmd("llvmtcl::DebugInfo::ArrayType", DefineArrayType);
    LLVMObjCmd("llvmtcl::DebugInfo::BasicType", DefineBasicType);
    LLVMObjCmd("llvmtcl::DebugInfo::PointerType", DefinePointerType);
    LLVMObjCmd("llvmtcl::DebugInfo::StructType", DefineStructType);
    LLVMObjCmd("llvmtcl::DebugInfo::FunctionType", DefineFunctionType);
    LLVMObjCmd("llvmtcl::DebugInfo::Parameter", DefineParameter);
    LLVMObjCmd("llvmtcl::DebugInfo::Local", DefineLocal);
    LLVMObjCmd("llvmtcl::DebugInfo::Function", DefineFunction);
    LLVMObjCmd("llvmtcl::DebugInfo::Instruction.SetLocation",
	    SetInstructionLocation);
    LLVMObjCmd("llvmtcl::DebugInfo::AttachToFunction", AttachToFunction);

    LLVMObjCmd("llvmtcl::AddFunctionAttr", AddFunctionAttr);
    LLVMObjCmd("llvmtcl::GetFunctionAttr", GetFunctionAttr);
    LLVMObjCmd("llvmtcl::RemoveFunctionAttr", RemoveFunctionAttr);
    LLVMObjCmd("llvmtcl::AddArgumentAttribute", AddAttribute);
    LLVMObjCmd("llvmtcl::RemoveArgumentAttribute", RemoveAttribute);
    LLVMObjCmd("llvmtcl::GetArgumentAttribute", GetAttribute);
    LLVMObjCmd("llvmtcl::AddCallAttribute", AddInstrAttribute);
    LLVMObjCmd("llvmtcl::RemoveCallAttribute", RemoveInstrAttribute);

    LLVMStrVar("llvmtcl::llvm_version", LLVM_VERSION_STRING);
    LLVMStrVar("llvmtcl::host_triple", LLVMTCL_TARGET);
    LLVMStrVar("llvmtcl::llvmbindir", LLVMBINDIR);

    return TCL_OK;
}

DLLEXPORT int Llvmtcl_SafeInit(Tcl_Interp *interp)
{
    if (Tcl_InitStubs(interp, TCL_VERSION, 0) == NULL)
	return TCL_ERROR;
    SetStringResult(interp, "extension is completely unsafe");
    return TCL_ERROR;
}

} /* extern "C" */

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 4
 * fill-column: 78
 * tab-width: 8
 * End:
 */
