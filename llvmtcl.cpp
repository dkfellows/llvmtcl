#include "tcl.h"
#include <iostream>
#include <sstream>
#include <map>
#include "llvm-c/Analysis.h"
#include "llvm-c/Core.h"
#include "llvm-c/ExecutionEngine.h"
#include "llvm-c/Target.h"

static std::map<std::string, LLVMModuleRef> LLVMModuleRef_map;
static std::map<std::string, LLVMBuilderRef> LLVMBuilderRef_map;
static std::map<std::string, LLVMTypeRef> LLVMTypeRef_map;
static std::map<std::string, LLVMValueRef> LLVMValueRef_map;

static std::string GetRefName(std::string prefix)
{
    static int LLVMRef_id = 0;
    std::ostringstream os;
    os << prefix << LLVMRef_id;
    LLVMRef_id++;
    return os.str();
}

static int GetLLVMModuleRefFromObj(Tcl_Interp* interp, Tcl_Obj* obj, LLVMModuleRef& moduleRef)
{
    moduleRef = 0;
    std::string moduleName = Tcl_GetStringFromObj(obj, 0);
    if (LLVMModuleRef_map.find(moduleName) == LLVMModuleRef_map.end()) {
	std::ostringstream os;
	os << "expected module but got \"" << moduleName << "\"";
	Tcl_SetObjResult(interp, Tcl_NewStringObj(os.str().c_str(), -1));
	return TCL_ERROR;
    }
    moduleRef = LLVMModuleRef_map[moduleName];
    return TCL_OK;
}

static int GetLLVMTypeRefFromObj(Tcl_Interp* interp, Tcl_Obj* obj, LLVMTypeRef& typeRef)
{
    typeRef = 0;
    std::string typeName = Tcl_GetStringFromObj(obj, 0);
    if (LLVMTypeRef_map.find(typeName) == LLVMTypeRef_map.end()) {
	std::ostringstream os;
	os << "expected type but got \"" << typeName << "\"";
	Tcl_SetObjResult(interp, Tcl_NewStringObj(os.str().c_str(), -1));
	return TCL_ERROR;
    }
    typeRef = LLVMTypeRef_map[typeName];
    return TCL_OK;
}

static int GetListOfLLVMTypeRefFromObj(Tcl_Interp* interp, Tcl_Obj* obj, LLVMTypeRef*& typeList, int& typeCount)
{
    typeCount = 0;
    typeList = 0;
    Tcl_Obj** typeObjs = 0;
    if (Tcl_ListObjGetElements(interp, obj, &typeCount, &typeObjs) != TCL_OK) {
	std::ostringstream os;
	os << "expected list of types but got \"" << Tcl_GetStringFromObj(obj, 0) << "\"";
	Tcl_SetObjResult(interp, Tcl_NewStringObj(os.str().c_str(), -1));
	return TCL_ERROR;
    }
    if (typeCount == 0)
	return TCL_OK;
    typeList = new LLVMTypeRef[typeCount];
    for(int i = 0; i < typeCount; i++) {
	if (GetLLVMTypeRefFromObj(interp, typeObjs[i], typeList[i]) != TCL_OK) {
	    delete [] typeList;
	    return TCL_ERROR;
	}
    }
    return TCL_OK;
}

int LLVMCreateBuilderObjCmd(ClientData clientData,
			    Tcl_Interp* interp,
			    int objc,
			    Tcl_Obj* const objv[])
{
    if (objc != 2) {
	Tcl_WrongNumArgs(interp, 2, objv, "");
	return TCL_ERROR;
    }
    LLVMBuilderRef builder = LLVMCreateBuilder();
    if (!builder) {
	Tcl_SetObjResult(interp, Tcl_NewStringObj("failed to create new builder", -1));
	return TCL_ERROR;
    }
    std::string builderName = GetRefName("LLVMBuilderRef_");
    LLVMBuilderRef_map[builderName] = builder;
    Tcl_SetObjResult(interp, Tcl_NewStringObj(builderName.c_str(), -1));
    return TCL_OK;
}

int LLVMDisposeBuilderObjCmd(ClientData clientData,
			     Tcl_Interp* interp,
			     int objc,
			     Tcl_Obj* const objv[])
{
    if (objc != 3) {
	Tcl_WrongNumArgs(interp, 2, objv, "builderRef");
	return TCL_ERROR;
    }
    std::string builder = Tcl_GetStringFromObj(objv[2], 0);
    if (LLVMBuilderRef_map.find(builder) == LLVMBuilderRef_map.end()) {
	std::ostringstream os;
	os << "expected builder but got \"" << builder << "\"";
	Tcl_SetObjResult(interp, Tcl_NewStringObj(os.str().c_str(), -1));
	return TCL_ERROR;
    }
    LLVMDisposeBuilder(LLVMBuilderRef_map[builder]);
    LLVMBuilderRef_map.erase(builder);
    return TCL_OK;
}

int LLVMDisposeModuleObjCmd(ClientData clientData,
			    Tcl_Interp* interp,
			    int objc,
			    Tcl_Obj* const objv[])
{
    if (objc != 3) {
	Tcl_WrongNumArgs(interp, 2, objv, "moduleRef");
	return TCL_ERROR;
    }
    std::string module = Tcl_GetStringFromObj(objv[2], 0);
    if (LLVMModuleRef_map.find(module) == LLVMModuleRef_map.end()) {
	std::ostringstream os;
	os << "expected module but got \"" << module << "\"";
	Tcl_SetObjResult(interp, Tcl_NewStringObj(os.str().c_str(), -1));
	return TCL_ERROR;
    }
    LLVMDisposeModule(LLVMModuleRef_map[module]);
    LLVMModuleRef_map.erase(module);
    return TCL_OK;
}

int LLVMInitializeNativeTargetObjCmd(ClientData clientData,
				     Tcl_Interp* interp,
				     int objc,
				     Tcl_Obj* const objv[])
{
    if (objc != 2) {
	Tcl_WrongNumArgs(interp, 2, objv, "");
	return TCL_ERROR;
    }
    LLVMInitializeNativeTarget();
    return TCL_OK;
}

int LLVMLinkInJITObjCmd(ClientData clientData,
			Tcl_Interp* interp,
			int objc,
			Tcl_Obj* const objv[])
{
    if (objc != 2) {
	Tcl_WrongNumArgs(interp, 2, objv, "");
	return TCL_ERROR;
    }
    LLVMLinkInJIT();
    return TCL_OK;
}

int LLVMModuleCreateWithNameObjCmd(ClientData clientData,
				   Tcl_Interp* interp,
				   int objc,
				   Tcl_Obj* const objv[])
{
    if (objc != 3) {
	Tcl_WrongNumArgs(interp, 2, objv, "name");
	return TCL_ERROR;
    }
    std::string name = Tcl_GetStringFromObj(objv[2], 0);
    LLVMModuleRef module = LLVMModuleCreateWithName(name.c_str());
    if (!module) {
	Tcl_SetObjResult(interp, Tcl_NewStringObj("failed to create new module", -1));
	return TCL_ERROR;
    }
    std::string moduleName = GetRefName("LLVMModuleRef_");
    LLVMModuleRef_map[moduleName] = module;
    Tcl_SetObjResult(interp, Tcl_NewStringObj(moduleName.c_str(), -1));
    return TCL_OK;
}

int LLVMAddFunctionObjCmd(ClientData clientData,
			  Tcl_Interp* interp,
			  int objc,
			  Tcl_Obj* const objv[])
{
    if (objc != 5) {
	Tcl_WrongNumArgs(interp, 2, objv, "moduleRef functionName functionTypeRef");
	return TCL_ERROR;
    }
    LLVMModuleRef moduleRef = 0;
    if (GetLLVMModuleRefFromObj(interp, objv[2], moduleRef) != TCL_OK)
	return TCL_ERROR;
    std::string functionName = Tcl_GetStringFromObj(objv[3], 0);
    LLVMTypeRef functionType = 0;
    if (GetLLVMTypeRefFromObj(interp, objv[4], functionType) != TCL_OK)
	return TCL_ERROR;
    LLVMValueRef functionRef = LLVMAddFunction(moduleRef, functionName.c_str(), functionType);
    std::string functionRefName = GetRefName("LLVMValueRef_");
    LLVMValueRef_map[functionRefName] = functionRef;
    Tcl_SetObjResult(interp, Tcl_NewStringObj(functionRefName.c_str(), -1));
    return TCL_OK;
}

int LLVMDeleteFunctionObjCmd(ClientData clientData,
			     Tcl_Interp* interp,
			     int objc,
			     Tcl_Obj* const objv[])
{
    if (objc != 3) {
	Tcl_WrongNumArgs(interp, 2, objv, "functionRef");
	return TCL_ERROR;
    }
    std::string function = Tcl_GetStringFromObj(objv[2], 0);
    if (LLVMValueRef_map.find(function) == LLVMValueRef_map.end()) {
	std::ostringstream os;
	os << "expected function but got \"" << function << "\"";
	Tcl_SetObjResult(interp, Tcl_NewStringObj(os.str().c_str(), -1));
	return TCL_ERROR;
    }
    LLVMDeleteFunction(LLVMValueRef_map[function]);
    LLVMValueRef_map.erase(function);
    return TCL_OK;
}

int LLVMConstIntObjCmd(ClientData clientData,
		       Tcl_Interp* interp,
		       int objc,
		       Tcl_Obj* const objv[])
{
    if (objc != 5) {
	Tcl_WrongNumArgs(interp, 2, objv, "typeRef value signExtended");
	return TCL_ERROR;
    }
    LLVMTypeRef constType = 0;
    if (GetLLVMTypeRefFromObj(interp, objv[2], constType) != TCL_OK)
	return TCL_ERROR;
    Tcl_WideInt value = 0;
    if (Tcl_GetWideIntFromObj(interp, objv[3], &value) != TCL_OK)
	return TCL_ERROR;
    int signExtend = 0;
    if (Tcl_GetBooleanFromObj(interp, objv[4], &signExtend) != TCL_OK)
	return TCL_ERROR;
    LLVMValueRef valueRef = LLVMConstInt(constType, value, signExtend);
    std::string valueName = GetRefName("LLVMValueRef_");
    LLVMValueRef_map[valueName] = valueRef;
    Tcl_SetObjResult(interp, Tcl_NewStringObj(valueName.c_str(), -1));
    return TCL_OK;
}

int LLVMConstIntOfStringObjCmd(ClientData clientData,
			       Tcl_Interp* interp,
			       int objc,
			       Tcl_Obj* const objv[])
{
    if (objc != 5) {
	Tcl_WrongNumArgs(interp, 2, objv, "typeRef value radix");
	return TCL_ERROR;
    }
    LLVMTypeRef constType = 0;
    if (GetLLVMTypeRefFromObj(interp, objv[2], constType) != TCL_OK)
	return TCL_ERROR;
    std::string value = Tcl_GetStringFromObj(objv[3], 0);
    int radix = 0;
    if (Tcl_GetIntFromObj(interp, objv[4], &radix) != TCL_OK)
	return TCL_ERROR;
    if (radix != 2 && radix != 8 && radix != 10 && radix != 16) {
	Tcl_SetObjResult(interp, Tcl_NewStringObj("radix should be 2, 8, 10, or 16", -1));
	return TCL_ERROR;
    }
    LLVMValueRef valueRef = LLVMConstIntOfString(constType, value.c_str(), radix);
    std::string valueName = GetRefName("LLVMValueRef_");
    LLVMValueRef_map[valueName] = valueRef;
    Tcl_SetObjResult(interp, Tcl_NewStringObj(valueName.c_str(), -1));
    return TCL_OK;
}

int LLVMConstRealObjCmd(ClientData clientData,
			Tcl_Interp* interp,
			int objc,
			Tcl_Obj* const objv[])
{
    if (objc != 4) {
	Tcl_WrongNumArgs(interp, 2, objv, "typeRef value");
	return TCL_ERROR;
    }
    LLVMTypeRef constType = 0;
    if (GetLLVMTypeRefFromObj(interp, objv[2], constType) != TCL_OK)
	return TCL_ERROR;
    double value = 0.0;
    if (Tcl_GetDoubleFromObj(interp, objv[3], &value) != TCL_OK)
	return TCL_ERROR;
    LLVMValueRef valueRef = LLVMConstReal(constType, value);
    std::string valueName = GetRefName("LLVMValueRef_");
    LLVMValueRef_map[valueName] = valueRef;
    Tcl_SetObjResult(interp, Tcl_NewStringObj(valueName.c_str(), -1));
    return TCL_OK;
}

int LLVMConstRealOfStringObjCmd(ClientData clientData,
				Tcl_Interp* interp,
				int objc,
				Tcl_Obj* const objv[])
{
    if (objc != 4) {
	Tcl_WrongNumArgs(interp, 2, objv, "typeRef value");
	return TCL_ERROR;
    }
    LLVMTypeRef constType = 0;
    if (GetLLVMTypeRefFromObj(interp, objv[2], constType) != TCL_OK)
	return TCL_ERROR;
    std::string value = Tcl_GetStringFromObj(objv[3], 0);
    LLVMValueRef valueRef = LLVMConstRealOfString(constType, value.c_str());
    std::string valueName = GetRefName("LLVMValueRef_");
    LLVMValueRef_map[valueName] = valueRef;
    Tcl_SetObjResult(interp, Tcl_NewStringObj(valueName.c_str(), -1));
    return TCL_OK;
}

int LLVMTypeObjCmd(ClientData clientData,
		   Tcl_Interp* interp,
		   int objc,
		   Tcl_Obj* const objv[])
{
    static const char *subCommands[] = {
	"LLVMArrayType",
	"LLVMDoubleType",
	"LLVMFP128Type",
	"LLVMFloatType",
	"LLVMFunctionType",
	"LLVMInt16Type",
	"LLVMInt1Type",
	"LLVMInt32Type",
	"LLVMInt64Type",
	"LLVMInt8Type",
	"LLVMIntType",
	"LLVMLabelType",
	"LLVMOpaqueType",
	"LLVMPPCFP128Type",
	"LLVMPointerType",
	"LLVMStructType",
	"LLVMUnionType",
	"LLVMVectorType",
	"LLVMVoidType",
	"LLVMX86FP80Type",
	NULL
    };
    enum SubCmds {
	eLLVMArrayType,
	eLLVMDoubleType,
	eLLVMFP128Type,
	eLLVMFloatType,
	eLLVMFunctionType,
	eLLVMInt16Type,
	eLLVMInt1Type,
	eLLVMInt32Type,
	eLLVMInt64Type,
	eLLVMInt8Type,
	eLLVMIntType,
	eLLVMLabelType,
	eLLVMOpaqueType,
	eLLVMPPCFP128Type,
	eLLVMPointerType,
	eLLVMStructType,
	eLLVMUnionType,
	eLLVMVectorType,
	eLLVMVoidType,
	eLLVMX86FP80Type
    };
    int index = -1;
    if (Tcl_GetIndexFromObj(interp, objv[1], subCommands, "type", 0, &index) != TCL_OK)
        return TCL_ERROR;
    // Check number of arguments
    switch ((enum SubCmds) index) {
    // 2 arguments
    case eLLVMDoubleType:
    case eLLVMFP128Type:
    case eLLVMFloatType:
    case eLLVMInt16Type:
    case eLLVMInt1Type:
    case eLLVMInt32Type:
    case eLLVMInt64Type:
    case eLLVMInt8Type:
    case eLLVMLabelType:
    case eLLVMOpaqueType:
    case eLLVMPPCFP128Type:
    case eLLVMVoidType:
    case eLLVMX86FP80Type:
	if (objc != 2) {
	    Tcl_WrongNumArgs(interp, 2, objv, "");
	    return TCL_ERROR;
	}
	break;
    // 3 arguments
    case eLLVMIntType:
	if (objc != 3) {
	    Tcl_WrongNumArgs(interp, 2, objv, "width");
	    return TCL_ERROR;
	}
	break;
    case eLLVMUnionType:
	if (objc != 3) {
	    Tcl_WrongNumArgs(interp, 2, objv, "listOfElementTypeRefs");
	    return TCL_ERROR;
	}
	break;
    // 4 arguments
    case eLLVMArrayType:
    case eLLVMVectorType:
	if (objc != 4) {
	    Tcl_WrongNumArgs(interp, 2, objv, "elementTypeRef elementCount");
	    return TCL_ERROR;
	}
	break;
    case eLLVMPointerType:
	if (objc != 4) {
	    Tcl_WrongNumArgs(interp, 2, objv, "elementTypeRef addressSpace");
	    return TCL_ERROR;
	}
	break;
    case eLLVMStructType:
	if (objc != 4) {
	    Tcl_WrongNumArgs(interp, 2, objv, "listOfElementTypeRefs packed");
	    return TCL_ERROR;
	}
	break;
    // 5 arguments
    case eLLVMFunctionType:
	if (objc != 5) {
	    Tcl_WrongNumArgs(interp, 2, objv, "returnTypeRef listOfArgumentTypeRefs isVarArg");
	    return TCL_ERROR;
	}
	break;
    }
    // Create the requested type
    LLVMTypeRef tref = 0;
    switch ((enum SubCmds) index) {
    case eLLVMArrayType:
    {
	LLVMTypeRef elementType = 0;
	if (GetLLVMTypeRefFromObj(interp, objv[2], elementType) != TCL_OK)
	    return TCL_ERROR;
	int elementCount = 0;
	if (Tcl_GetIntFromObj(interp, objv[3], &elementCount) != TCL_OK)
	    return TCL_ERROR;
	tref = LLVMArrayType(elementType, elementCount);
	break;
    }
    case eLLVMDoubleType:
	tref = LLVMDoubleType();
	break;
    case eLLVMFP128Type:
	tref = LLVMFP128Type();
	break;
    case eLLVMFloatType:
	tref = LLVMFloatType();
	break;
    case eLLVMFunctionType:
    {
	LLVMTypeRef returnType = 0;
	if (GetLLVMTypeRefFromObj(interp, objv[2], returnType) != TCL_OK)
	    return TCL_ERROR;
	int isVarArg = 0;
	if (Tcl_GetBooleanFromObj(interp, objv[4], &isVarArg) != TCL_OK)
	    return TCL_ERROR;
	int argumentCount = 0;
	LLVMTypeRef* argumentTypes = 0;
	if (GetListOfLLVMTypeRefFromObj(interp, objv[3], argumentTypes, argumentCount) != TCL_OK)
	    return TCL_ERROR;
	tref = LLVMFunctionType(returnType, argumentTypes, argumentCount, isVarArg);
	if (argumentCount)
	    delete [] argumentTypes;
	break;
    }
    case eLLVMInt16Type:
	tref = LLVMInt16Type();
	break;
    case eLLVMInt1Type:
	tref = LLVMInt1Type();
	break;
    case eLLVMInt32Type:
	tref = LLVMInt32Type();
	break;
    case eLLVMInt64Type:
	tref = LLVMInt64Type();
	break;
    case eLLVMInt8Type:
	tref = LLVMInt8Type();
	break;
    case eLLVMIntType:
    {
	int width = 0;
	if (Tcl_GetIntFromObj(interp, objv[2], &width) != TCL_OK)
	    return TCL_ERROR;
	tref = LLVMIntType(width);
	break;
    }
    case eLLVMLabelType:
	tref = LLVMLabelType();
	break;
    case eLLVMOpaqueType:
	tref = LLVMOpaqueType();
	break;
    case eLLVMPPCFP128Type:
	tref = LLVMPPCFP128Type();
	break;
    case eLLVMPointerType:
    {
	LLVMTypeRef elementType = 0;
	if (GetLLVMTypeRefFromObj(interp, objv[2], elementType) != TCL_OK)
	    return TCL_ERROR;
	int addressSpace = 0;
	if (Tcl_GetIntFromObj(interp, objv[3], &addressSpace) != TCL_OK)
	    return TCL_ERROR;
	tref = LLVMPointerType(elementType, addressSpace);
	break;
    }
    case eLLVMStructType:
    {
	int elementCount = 0;
	LLVMTypeRef* elementTypes = 0;
	if (GetListOfLLVMTypeRefFromObj(interp, objv[2], elementTypes, elementCount) != TCL_OK)
	    return TCL_ERROR;
	if (!elementCount) {
	    Tcl_SetObjResult(interp, Tcl_NewStringObj("no element types specified", -1));
	    return TCL_ERROR;
	}
	int packed = 0;
	if (Tcl_GetBooleanFromObj(interp, objv[3], &packed) != TCL_OK)
	    return TCL_ERROR;
	tref = LLVMStructType(elementTypes, elementCount, packed);
	delete [] elementTypes;
	break;
    }
    case eLLVMUnionType:
    {
	int elementCount = 0;
	LLVMTypeRef* elementTypes = 0;
	if (GetListOfLLVMTypeRefFromObj(interp, objv[2], elementTypes, elementCount) != TCL_OK)
	    return TCL_ERROR;
	if (!elementCount) {
	    Tcl_SetObjResult(interp, Tcl_NewStringObj("no element types specified", -1));
	    return TCL_ERROR;
	}
	tref = LLVMUnionType(elementTypes, elementCount);
	delete [] elementTypes;
	break;
    }
    case eLLVMVectorType:
    {
	LLVMTypeRef elementType = 0;
	if (GetLLVMTypeRefFromObj(interp, objv[2], elementType) != TCL_OK)
	    return TCL_ERROR;
	int elementCount = 0;
	if (Tcl_GetIntFromObj(interp, objv[3], &elementCount) != TCL_OK)
	    return TCL_ERROR;
	tref = LLVMVectorType(elementType, elementCount);
	break;
    }
    case eLLVMVoidType:
	tref = LLVMVoidType();
	break;
    case eLLVMX86FP80Type:
	tref = LLVMX86FP80Type();
	break;
    }
    if (!tref) {
	Tcl_SetObjResult(interp, Tcl_NewStringObj("failed to create new type", -1));
	return TCL_ERROR;
    }
    std::string typeRefName = GetRefName("LLVMTypeRef_");
    LLVMTypeRef_map[typeRefName] = tref;
    Tcl_SetObjResult(interp, Tcl_NewStringObj(typeRefName.c_str(), -1));
    return TCL_OK;
}

typedef int (*LLVMObjCmdPtr)(ClientData clientData,
			     Tcl_Interp* interp,
			     int objc,
			     Tcl_Obj* const objv[]);

extern "C" int llvmtcl(ClientData clientData,
		       Tcl_Interp* interp,
		       int objc,
		       Tcl_Obj* const objv[])
{
    // objv[1] is llvm c function to be executed
    if (objc < 2) {
	Tcl_WrongNumArgs(interp, 1, objv, "subcommand ?arg ...?");
	return TCL_ERROR;
    }
    static std::map<std::string, LLVMObjCmdPtr> subObjCmds;
    if (subObjCmds.size() == 0) {
	subObjCmds["LLVMAddFunction"] = &LLVMAddFunctionObjCmd;
	subObjCmds["LLVMArrayType"] = &LLVMTypeObjCmd;
	subObjCmds["LLVMConstInt"] = &LLVMConstIntObjCmd;
	subObjCmds["LLVMConstIntOfString"] = &LLVMConstIntOfStringObjCmd;
	subObjCmds["LLVMConstReal"] = &LLVMConstRealObjCmd;
	subObjCmds["LLVMConstRealOfString"] = &LLVMConstRealOfStringObjCmd;
	subObjCmds["LLVMCreateBuilder"] = &LLVMCreateBuilderObjCmd;
	subObjCmds["LLVMDeleteFunction"] = &LLVMDeleteFunctionObjCmd;
	subObjCmds["LLVMDisposeBuilder"] = &LLVMDisposeBuilderObjCmd;
	subObjCmds["LLVMDisposeModule"] = &LLVMDisposeModuleObjCmd;
	subObjCmds["LLVMDoubleType"] = &LLVMTypeObjCmd;
	subObjCmds["LLVMFP128Type"] = &LLVMTypeObjCmd;
	subObjCmds["LLVMFloatType"] = &LLVMTypeObjCmd;
	subObjCmds["LLVMFunctionType"] = &LLVMTypeObjCmd;
	subObjCmds["LLVMInitializeNativeTarget"] = &LLVMInitializeNativeTargetObjCmd;
	subObjCmds["LLVMInt16Type"] = &LLVMTypeObjCmd;
	subObjCmds["LLVMInt1Type"] = &LLVMTypeObjCmd;
	subObjCmds["LLVMInt32Type"] = &LLVMTypeObjCmd;
	subObjCmds["LLVMInt64Type"] = &LLVMTypeObjCmd;
	subObjCmds["LLVMInt8Type"] = &LLVMTypeObjCmd;
	subObjCmds["LLVMIntType"] = &LLVMTypeObjCmd;
	subObjCmds["LLVMLabelType"] = &LLVMTypeObjCmd;
	subObjCmds["LLVMLinkInJIT"] = &LLVMLinkInJITObjCmd;
	subObjCmds["LLVMModuleCreateWithName"] = &LLVMModuleCreateWithNameObjCmd;
	subObjCmds["LLVMOpaqueType"] = &LLVMTypeObjCmd;
	subObjCmds["LLVMPPCFP128Type"] = &LLVMTypeObjCmd;
	subObjCmds["LLVMPointerType"] = &LLVMTypeObjCmd;
	subObjCmds["LLVMStructType"] = &LLVMTypeObjCmd;
	subObjCmds["LLVMUnionType"] = &LLVMTypeObjCmd;
	subObjCmds["LLVMVectorType"] = &LLVMTypeObjCmd;
	subObjCmds["LLVMVoidType"] = &LLVMTypeObjCmd;
	subObjCmds["LLVMX86FP80Type"] = &LLVMTypeObjCmd;
    }
    std::string subCmd = Tcl_GetStringFromObj(objv[1], 0);
    if (subObjCmds.find(subCmd) == subObjCmds.end()) {
	std::ostringstream os;
	os << "bad subcommand \"" << subCmd << "\": must be ";
	for(std::map<std::string, LLVMObjCmdPtr>::const_iterator i = subObjCmds.begin(); i !=  subObjCmds.end(); i++)
	    os << " " << i->first;
	Tcl_SetObjResult(interp, Tcl_NewStringObj(os.str().c_str(), -1));
	return TCL_ERROR;
    }
    return subObjCmds[subCmd](clientData, interp, objc, objv);
}

extern "C" DLLEXPORT int Llvmtcl_Init(Tcl_Interp *interp)
{
    if (Tcl_InitStubs(interp, TCL_VERSION, 0) == NULL) {
	return TCL_ERROR;
    }
    if (Tcl_PkgRequire(interp, "Tcl", TCL_VERSION, 0) == NULL) {
	return TCL_ERROR;
    }
    if (Tcl_PkgProvide(interp, "llvmtcl", "0.1") != TCL_OK) {
	return TCL_ERROR;
    }

    Tcl_CreateObjCommand(interp, "llvmtcl::llvmtcl",
			 (Tcl_ObjCmdProc*)llvmtcl,
			 (ClientData)NULL, (Tcl_CmdDeleteProc*)NULL);

    return TCL_OK;
}
