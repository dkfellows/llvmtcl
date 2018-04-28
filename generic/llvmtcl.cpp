#include <tcl.h>
#include <tclTomMath.h>
#include <iostream>
#include <sstream>
#include <map>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Support/TargetRegistry.h>
#include <llvm/Support/Host.h>

#include "version.h"
#ifdef API_2
#include <llvm/IR/PassManager.h>
#else // !API_2
#include <llvm/PassManager.h>
#endif // API_2

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

static int
ParseCommandLineOptionsObjCmd(
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
    bool result = llvm::cl::ParseCommandLineOptions(
	    objc, argv.data(), "called from Tcl", &os);
    os.flush();
    if (!result) {
	TrimRight(s);
	SetStringResult(interp, s);
	return TCL_ERROR;
    }
    return TCL_OK;
}

static void
LLVMAddCoroutinePassesToExtensionPoints(
    LLVMPassManagerBuilderRef ref)
{
    auto Builder = reinterpret_cast<llvm::PassManagerBuilder*>(ref);
    addCoroutinePassesToExtensionPoints(*Builder);
}

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

static inline Tcl_Obj *
NewObj(LLVMModuleRef module)
{
    return SetLLVMModuleRefAsObj(nullptr, module);
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

static int
CreateGenericValueOfTclInterpObjCmd(
    ClientData clientData,
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const objv[])
{
    if (objc != 1) {
        Tcl_WrongNumArgs(interp, 1, objv, "");
        return TCL_ERROR;
    }

    LLVMGenericValueRef rt = LLVMCreateGenericValueOfPointer(interp);
    Tcl_SetObjResult(interp, SetLLVMGenericValueRefAsObj(interp, rt));
    return TCL_OK;
}

static int
CreateGenericValueOfTclObjObjCmd(
    ClientData clientData,
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const objv[])
{
    if (objc != 2) {
        Tcl_WrongNumArgs(interp, 1, objv, "val");
        return TCL_ERROR;
    }

    Tcl_IncrRefCount(objv[1]);
    LLVMGenericValueRef rt = LLVMCreateGenericValueOfPointer(objv[1]);
    Tcl_SetObjResult(interp, SetLLVMGenericValueRefAsObj(interp, rt));
    return TCL_OK;
}

static int
GenericValueToTclObjObjCmd(
    ClientData clientData,
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const objv[])
{
    if (objc != 2) {
        Tcl_WrongNumArgs(interp, 1, objv, "GenVal");
        return TCL_ERROR;
    }

    LLVMGenericValueRef arg1 = 0;
    if (GetLLVMGenericValueRefFromObj(interp, objv[1], arg1) != TCL_OK)
        return TCL_ERROR;

    Tcl_Obj *rt = (Tcl_Obj *) LLVMGenericValueToPointer(arg1);
    Tcl_SetObjResult(interp, rt);
    return TCL_OK;
}

static int
LLVMAddIncomingObjCmd(
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

static int
LLVMBuildAggregateRetObjCmd(
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

static int
LLVMBuildInvokeObjCmd(
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

static int
LLVMGetParamTypesObjCmd(
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

static int
LLVMGetParamsObjCmd(
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

static int
LLVMGetStructElementTypesObjCmd(
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

static int
LLVMGetBasicBlocksObjCmd(
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

#include "generated/llvmtcl-gen.h"

// Done this way because the C wrapper for verifyFunction sucks
static int VerifyFunctionObjCmd(
    ClientData clientData,
    Tcl_Interp* interp,
    int objc,
    Tcl_Obj* const objv[])
{
    /*
     * Parse arguments (ignored argument supports old API)
     */

    if (objc < 2 || objc > 3) {
	Tcl_WrongNumArgs(interp, 1, objv, "Fn ?ignored?");
	return TCL_ERROR;
    }
    llvm::Function *fun;
    if (GetValueFromObj(interp, objv[1],
	    "expected function but got another type of value", fun) != TCL_OK)
	return TCL_ERROR;

    /*
     * Perform verification
     */

    std::string Messages;
    llvm::raw_string_ostream MsgsOS(Messages);
    if (llvm::verifyFunction(*fun, &MsgsOS)) {
	MsgsOS.flush();
	TrimRight(Messages);
	SetStringResult(interp, Messages);
	return TCL_ERROR;
    }

    /*
     * Make result; weird API for backward compatibility reasons.
     */

    Tcl_SetObjResult(interp, NewObj(true));
    return TCL_OK;
}

// Done this way because the C wrapper for verifyModule sucks
static int VerifyModuleObjCmd(
    ClientData clientData,
    Tcl_Interp* interp,
    int objc,
    Tcl_Obj* const objv[])
{
    /*
     * Parse arguments
     */

    if (objc < 2 || objc > 4) {
	Tcl_WrongNumArgs(interp, 1, objv,
		"Module ?ignored? ?debugInfoFailuresNonfatal?");
	return TCL_ERROR;
    }
    llvm::Module *mod;
    if (GetModuleFromObj(interp, objv[1], mod) != TCL_OK)
	return TCL_ERROR;
    int dbnonfatal = 0;		/* Whether to report Debug Info failures as
				 * non-fatal problems; by default, they fail
				 * the verification entirely. */
    if ((objc > 3)
	    && Tcl_GetBooleanFromObj(interp, objv[3], &dbnonfatal) != TCL_OK)
	return TCL_ERROR;

    /*
     * Perform verification
     */

    std::string Messages;
    llvm::raw_string_ostream MsgsOS(Messages);
    bool DebugInfoBroken = false, *debugInfo = nullptr;
    if (dbnonfatal)
	debugInfo = &DebugInfoBroken;
    bool failedVerification = llvm::verifyModule(*mod, &MsgsOS, debugInfo);
    MsgsOS.flush();

    /*
     * Convert result; weird API for backward compatibility reasons.
     */

    TrimRight(Messages);
    std::vector<tcl::value> result { NewObj(failedVerification), NewObj(Messages) };
    Tcl_SetObjResult(interp, NewObj(result));
    return TCL_OK;
}

static int
LLVMCallInitialisePackageFunction(
    ClientData clientData,
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const objv[])
{
    typedef int (*initFunction_t)(Tcl_Interp*);

    if (objc != 3) {
	Tcl_WrongNumArgs(interp, 1, objv, "EE F");
	return TCL_ERROR;
    }

    llvm::ExecutionEngine *engine;
    if (GetEngineFromObj(interp, objv[1], engine) != TCL_OK)
	return TCL_ERROR;
    llvm::Function *function;
    if (GetValueFromObj(interp, objv[2],
	    "can only initialise using a function", function) != TCL_OK)
	return TCL_ERROR;

    auto address = engine->getFunctionAddress(function->getName());
    auto initFunction = reinterpret_cast<initFunction_t>(address);
    if (initFunction == nullptr) {
	SetStringResult(interp, "no address for initialiser");
	return TCL_ERROR;
    }

    return initFunction(interp);
}

static int
NamedStructTypeObjCmd(
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
    LLVMModuleRef mod,
    LLVMExecutionEngineRef eeRef)
{
    auto m = llvm::unwrap(mod);
    auto engine = llvm::unwrap(eeRef);
    auto void_ptr_type = llvm::Type::getInt8PtrTy(m->getContext());

    engine->addGlobalMapping(llvm::cast<llvm::GlobalVariable>(
	    m->getOrInsertGlobal("stdin", void_ptr_type)), stdin);
    engine->addGlobalMapping(llvm::cast<llvm::GlobalVariable>(
	    m->getOrInsertGlobal("stdout", void_ptr_type)), stdout);
    engine->addGlobalMapping(llvm::cast<llvm::GlobalVariable>(
	    m->getOrInsertGlobal("stderr", void_ptr_type)), stderr);
}

static void
PatchRuntimeEnvironment(
    LLVMExecutionEngineRef eeRef)
{
    // Replace a part of the LLVM runtime that's not always present

    auto engine = llvm::unwrap(eeRef);
    const void *powidf2 = (const void *) __powidf2;
    engine->addGlobalMapping("__powidf2", (uint64_t) powidf2);
}

static int
CreateMCJITCompilerForModuleObjCmd(
    ClientData clientData,
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const objv[])
{
    if (objc < 2 || objc > 4) {
        Tcl_WrongNumArgs(interp, 1, objv, "M ?OptLevel? ?EnableStdio?");
        return TCL_ERROR;
    }

    LLVMModuleRef mod = 0;
    if (GetLLVMModuleRefFromObj(interp, objv[1], mod) != TCL_OK)
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
    if (LLVMCreateMCJITCompilerForModule(&eeRef, mod,
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

static int
GetHostTripleObjCmd(
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

static int
CopyModuleFromModuleCmd(
    ClientData clientData,
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const objv[])
{
    if (objc < 2 || objc > 4) {
        Tcl_WrongNumArgs(interp, 1, objv, "M ?NewModuleID? ?NewSourceFile?");
        return TCL_ERROR;
    }
    LLVMModuleRef srcmod = 0;
    if (GetLLVMModuleRefFromObj(interp, objv[1], srcmod) != TCL_OK)
        return TCL_ERROR;
    LLVMModuleRef tgtmod = LLVMCloneModule(srcmod);
    if (objc > 2) {
	std::string tgtid = Tcl_GetString(objv[2]);
	llvm::unwrap(tgtmod)->setModuleIdentifier(tgtid);
    }
    if (objc > 3) {
#ifdef API_4
	std::string tgtfile = Tcl_GetString(objv[3]);
	llvm::unwrap(tgtmod)->setSourceFileName(tgtfile);
#else // !API_4
	LLVMDisposeModule(tgtmod);
	Tcl_SetObjResult(interp, Tcl_ObjPrintf(
	    "setting source filename not supported in older LLVMs: upgrade to 4.0 or later"));
	return TCL_ERROR;
#endif // API_4
    }
    Tcl_SetObjResult(interp, NewObj(tgtmod));
    return TCL_OK;
}

static int
CreateModuleFromBitcodeCmd(
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

    Tcl_SetObjResult(interp, NewObj(module));
    return TCL_OK;

  error:
    SetStringResult(interp, msg);
    free(msg);
    return TCL_ERROR;
}

static int
GarbageCollectUnusedFunctionsInModuleCmd(
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

static int
MakeTargetMachineCmd(
    ClientData clientData,
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const objv[])
{
    if (objc < 1 || objc > 2) {
	Tcl_WrongNumArgs(interp, 1, objv, "?Target?");
	return TCL_ERROR;
    }
    const char *triple = nullptr;
    if (objc > 1) {
	triple = Tcl_GetString(objv[1]);
    }
    if (!triple || triple[0] == '\0')
	triple = LLVMTCL_TARGET;

    LLVMTargetRef target;
    char *err;
    if (LLVMGetTargetFromTriple(triple, &target, &err)) {
	Tcl_SetResult(interp, err, TCL_VOLATILE);
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

static int
WriteModuleMachineCodeToFileCmd(
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
    std::cerr << "Target triple:" << triple << std::endl;
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

#ifdef API_3
static int
TokenTypeObjCmd(
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

static int
ConstNoneObjCmd(
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
#endif // API_3

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

    LLVMObjCmd("llvmtcl::ParseCommandLineOptions",
	    ParseCommandLineOptionsObjCmd);
#ifdef API_3
    LLVMObjCmd("llvmtcl::TokenType", TokenTypeObjCmd);
    LLVMObjCmd("llvmtcl::ConstNone", ConstNoneObjCmd);
#endif // API_3
    LLVMObjCmd("llvmtcl::VerifyModule", VerifyModuleObjCmd);
    LLVMObjCmd("llvmtcl::VerifyFunction", VerifyFunctionObjCmd);
    LLVMObjCmd("llvmtcl::CreateGenericValueOfTclInterp",
	    CreateGenericValueOfTclInterpObjCmd);
    LLVMObjCmd("llvmtcl::CreateGenericValueOfTclObj",
	    CreateGenericValueOfTclObjObjCmd);
    LLVMObjCmd("llvmtcl::GenericValueToTclObj", GenericValueToTclObjObjCmd);
    LLVMObjCmd("llvmtcl::AddLLVMTclCommands", LLVMAddLLVMTclCommandsObjCmd);
    LLVMObjCmd("llvmtcl::AddIncoming", LLVMAddIncomingObjCmd);
    LLVMObjCmd("llvmtcl::BuildAggregateRet", LLVMBuildAggregateRetObjCmd);
    LLVMObjCmd("llvmtcl::BuildInvoke", LLVMBuildInvokeObjCmd);
    LLVMObjCmd("llvmtcl::GetParamTypes", LLVMGetParamTypesObjCmd);
    LLVMObjCmd("llvmtcl::GetParams", LLVMGetParamsObjCmd);
    LLVMObjCmd("llvmtcl::GetStructElementTypes",
	    LLVMGetStructElementTypesObjCmd);
    LLVMObjCmd("llvmtcl::GetBasicBlocks", LLVMGetBasicBlocksObjCmd);
    LLVMObjCmd("llvmtcl::CallInitialisePackageFunction",
	    LLVMCallInitialisePackageFunction);
    LLVMObjCmd("llvmtcl::GetIntrinsicDefinition",
	    LLVMGetIntrinsicDefinitionObjCmd);
    LLVMObjCmd("llvmtcl::GetIntrinsicID", LLVMGetIntrinsicIDObjCmd);
    LLVMObjCmd("llvmtcl::NamedStructType", NamedStructTypeObjCmd);
    LLVMObjCmd("llvmtcl::CreateMCJITCompilerForModule",
	    CreateMCJITCompilerForModuleObjCmd);
    LLVMObjCmd("llvmtcl::GetHostTriple", GetHostTripleObjCmd);
    LLVMObjCmd("llvmtcl::CopyModuleFromModule", CopyModuleFromModuleCmd);
    LLVMObjCmd("llvmtcl::CreateModuleFromBitcode",
	    CreateModuleFromBitcodeCmd);
    LLVMObjCmd("llvmtcl::GarbageCollectUnusedFunctionsInModule",
	    GarbageCollectUnusedFunctionsInModuleCmd);
    LLVMObjCmd("llvmtcl::MakeTargetMachine", MakeTargetMachineCmd);
    LLVMObjCmd("llvmtcl::WriteModuleMachineCodeToFile",
	    WriteModuleMachineCodeToFileCmd);
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

    LLVMObjCmd("llvmtcl::AddFunctionAttr", LLVMAddFunctionAttrObjCmd);
    LLVMObjCmd("llvmtcl::GetFunctionAttr", LLVMGetFunctionAttrObjCmd);
    LLVMObjCmd("llvmtcl::RemoveFunctionAttr", LLVMRemoveFunctionAttrObjCmd);
    LLVMObjCmd("llvmtcl::AddArgumentAttribute", LLVMAddAttributeObjCmd);
    LLVMObjCmd("llvmtcl::RemoveArgumentAttribute", LLVMRemoveAttributeObjCmd);
    LLVMObjCmd("llvmtcl::GetArgumentAttribute", LLVMGetAttributeObjCmd);
    LLVMObjCmd("llvmtcl::AddCallAttribute", LLVMAddInstrAttributeObjCmd);
    LLVMObjCmd("llvmtcl::RemoveCallAttribute", LLVMRemoveInstrAttributeObjCmd);

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
