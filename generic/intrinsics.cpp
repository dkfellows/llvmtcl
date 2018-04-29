#include "llvmtcl.h"
#include <tcl.h>
#include <llvm/IR/Intrinsics.h>
#include <llvm-c/Core.h>
#include <unordered_map>
#include <string>
#include <stdexcept>

/*
 * Manage the table of intrinsic functions. This table is driven by LLVM's
 * configuration.
 *
 * Tricky point: this table is *not* sorted by name. It is sorted by intrinsic
 * ID, and the IDs are arranged in thematic blocks (and usually alphabetically
 * within a block, though that's not guaranteed). Binary search doesn't work.
 *
 * Second tricky point: IDs are actually offset by 1 from the index into this
 * table. Yeah.
 */

static const char *const intrinsicNames[] = {
#define GET_INTRINSIC_NAME_TABLE
#include <llvm/IR/Intrinsics.gen>
#undef GET_INTRINSIC_NAME_TABLE
};

using namespace std;
using namespace llvm;

TCL_DECLARE_MUTEX(initLock)
static unordered_map<string, Intrinsic::ID> *intrMap;
static bool intrMapInit = false;

/*
 * ------------------------------------------------------------------------
 *
 * GetLLVMIntrinsicIDFromObj --
 *	Converts the name of an intrinsic into its ID.
 *
 * ------------------------------------------------------------------------
 */

static inline int
GetLLVMIntrinsicIDFromObj(
    Tcl_Interp *interp,
    Tcl_Obj *obj,
    Intrinsic::ID &id)
{
    /*
     * Initialise the mapping if it wasn't already.
     */

    Tcl_MutexLock(&initLock);
    if (!intrMapInit) {
	const size_t num = sizeof(intrinsicNames) / sizeof(const char *);
	intrMapInit = true;
	intrMap = new unordered_map<string, Intrinsic::ID>();
	for (size_t i=0 ; i<num ; i++) {
	    (*intrMap)[intrinsicNames[i]] = static_cast<Intrinsic::ID>(i + 1);
	}
    }
    Tcl_MutexUnlock(&initLock);

    /*
     * Do the search.
     */

    auto search = intrMap->find(Tcl_GetString(obj));
    if (search == intrMap->end()) {
	Tcl_SetObjResult(interp, Tcl_ObjPrintf(
		"expected intrinsic name but got \"%s\"", Tcl_GetString(obj)));
	return TCL_ERROR;
    }
    id = search->second;
    return TCL_OK;
}

/*
 * ------------------------------------------------------------------------
 *
 * NewIntrinsicIDObj --
 *	Converts the ID of an intrinsic into a Tcl value.
 *
 * ------------------------------------------------------------------------
 */

static inline Tcl_Obj *
NewIntrinsicIDObj(
    unsigned id)
{
    if (id <= 0 || id > sizeof(intrinsicNames) / sizeof(const char *)) {
	return Tcl_NewStringObj("<unknown LLVMIntrinsic>", -1);
    }

    string s = intrinsicNames[id - 1];
    return Tcl_NewStringObj(s.c_str(), -1);
}

/*
 * ------------------------------------------------------------------------
 *
 * MakeIntrinsicIsOverloadedError --
 *	Constructs a Tcl error message (with some useful information) that is
 *	used when an intrinsic doesn't have the right type specialisation
 *	information (not needed for all intrinsics).
 *
 * ------------------------------------------------------------------------
 */

static inline void
MakeIntrinsicIsOverloadedError(
    Tcl_Interp *interp,
    Tcl_Obj *objPtr,
    const vector<Intrinsic::IITDescriptor::ArgKind> &kinds)
{
    Tcl_SetObjResult(interp, Tcl_ObjPrintf(
	    "%s is overloaded; %d type arguments must be specified",
	    Tcl_GetString(objPtr), int(kinds.size())));
    if (kinds.size()) {
	const char *sep = " (";
	for (auto kind : kinds) {
	    switch (kind) {
	    case Intrinsic::IITDescriptor::AK_Any:
		Tcl_AppendResult(interp, sep, "any", NULL);
		break;
	    case Intrinsic::IITDescriptor::AK_AnyInteger:
		Tcl_AppendResult(interp, sep, "integer", NULL);
		break;
	    case Intrinsic::IITDescriptor::AK_AnyFloat:
		Tcl_AppendResult(interp, sep, "float", NULL);
		break;
	    case Intrinsic::IITDescriptor::AK_AnyVector:
		Tcl_AppendResult(interp, sep, "vector", NULL);
		break;
	    case Intrinsic::IITDescriptor::AK_AnyPointer:
		Tcl_AppendResult(interp, sep, "pointer", NULL);
		break;
	    }
	    sep = ", ";
	}
	Tcl_AppendResult(interp, ")", NULL);
    }
}

/*
 * ------------------------------------------------------------------------
 *
 * LLVMGetIntrinsicDefinitionObjCmd --
 *	Implements a Tcl command for correctly getting an LLVM intrinsic.
 *
 *	Makes reaching into the table of intrinsics as safe as possible.
 *	Intrinsics mostly look like (special) function definitions to the rest
 *	of LLVM.
 *
 * ------------------------------------------------------------------------
 */

MODULE_SCOPE int
LLVMGetIntrinsicDefinitionObjCmd(
    ClientData clientData,
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const objv[])
{
    if (objc < 3) {
        Tcl_WrongNumArgs(interp, 1, objv, "M Name Ty...");
        return TCL_ERROR;
    }

    Module *mod;
    Intrinsic::ID id;

    if (GetModuleFromObj(interp, objv[1], mod) != TCL_OK)
        return TCL_ERROR;
    if (GetLLVMIntrinsicIDFromObj(interp, objv[2], id) != TCL_OK)
        return TCL_ERROR;

    /*
     * The following code is nasty, but not nearly as nasty as a hard crash in
     * LLVM itself when one of its internal checks fails. First, we parse the
     * descriptor for the intrinsic (obtained from its ID) to see what type
     * parameters (if any) are required.
     */

    SmallVector<Intrinsic::IITDescriptor, 8> descs;
    Intrinsic::getIntrinsicInfoTableEntries(id, descs);
    size_t reqs = 0;
    vector<Intrinsic::IITDescriptor::ArgKind> requiredKinds;
    for (auto desc : descs) {
	if (desc.Kind >= Intrinsic::IITDescriptor::Argument &&
		desc.Kind <= Intrinsic::IITDescriptor::PtrToArgument) {
	    size_t num = 1 + desc.getArgumentNumber();
	    reqs = (num>reqs ? num : reqs);
	    while (requiredKinds.size() < num)
		requiredKinds.push_back(Intrinsic::IITDescriptor::AK_Any);
	    requiredKinds[desc.getArgumentNumber()] = desc.getArgumentKind();
	}
    }

    /*
     * Now we can validate the arguments. First the total number...
     */

    if (requiredKinds.size() != (size_t) objc - 3) {
	MakeIntrinsicIsOverloadedError(interp, objv[2], requiredKinds);
	return TCL_ERROR;
    }

    /*
     * ... and lastly we check that the type arguments passed are actually
     * acceptable.
     */

    vector<Type *> arg_types;
    for (size_t i = 0 ; i<requiredKinds.size() ; i++) {
	auto kind = requiredKinds[i];
	Type *ty;

	if (GetTypeFromObj(interp, objv[i + 3], ty) != TCL_OK)
	    return TCL_ERROR;
	arg_types.push_back(ty);

	/*
	 * Do the real argument type check if required.
	 */

	switch (kind) {
	case Intrinsic::IITDescriptor::AK_Any:
	    // 0 means any type is acceptable
	    break;
	case Intrinsic::IITDescriptor::AK_AnyInteger:
	    if (!ty->isIntegerTy()) {
		Tcl_SetObjResult(interp, Tcl_ObjPrintf(
			"argument %d to %s must be an integer type",
			int(i + 1), Tcl_GetString(objv[2])));
		return TCL_ERROR;
	    }
	    break;
	case Intrinsic::IITDescriptor::AK_AnyFloat:
	    if (!ty->isFloatingPointTy()) {
		Tcl_SetObjResult(interp, Tcl_ObjPrintf(
			"type argument %d to %s must be a floating point type",
			int(i + 1), Tcl_GetString(objv[2])));
		return TCL_ERROR;
	    }
	    break;
	case Intrinsic::IITDescriptor::AK_AnyVector:
	    if (!ty->isVectorTy()) {
		Tcl_SetObjResult(interp, Tcl_ObjPrintf(
			"type argument %d to %s must be a vector type",
			int(i + 1), Tcl_GetString(objv[2])));
		return TCL_ERROR;
	    }
	    break;
	case Intrinsic::IITDescriptor::AK_AnyPointer:
	    if (!ty->isPointerTy()) {
		Tcl_SetObjResult(interp, Tcl_ObjPrintf(
			"type argument %d to %s must be a pointer type",
			int(i + 1), Tcl_GetString(objv[2])));
		return TCL_ERROR;
	    }
	    break;
	}
    }

    /*
     * This really ought to work now. We don't assume that it *must* though.
     */

    auto intrinsic = Intrinsic::getDeclaration(mod, id, arg_types);
    if (intrinsic == NULL) {
	SetStringResult(interp, "no such intrinsic");
	return TCL_ERROR;
    }

    Tcl_SetObjResult(interp, SetLLVMValueRefAsObj(interp, intrinsic));
    return TCL_OK;
}

/*
 * ------------------------------------------------------------------------
 *
 * GetIntrinsicID --
 *	Implements a Tcl command for getting the ID of an intrinsic (or the
 *	empty string if the function is not an intrinsic).
 *
 * ------------------------------------------------------------------------
 */

MODULE_SCOPE int
LLVMGetIntrinsicIDObjCmd(
    ClientData clientData,
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const objv[])
{
    if (objc != 2) {
        Tcl_WrongNumArgs(interp, 1, objv, "Fn ");
        return TCL_ERROR;
    }

    LLVMValueRef intrinsic = 0;
    if (GetLLVMValueRefFromObj(interp, objv[1], intrinsic) != TCL_OK)
        return TCL_ERROR;

    unsigned id = LLVMGetIntrinsicID(intrinsic);

    if (id != 0)
	Tcl_SetObjResult(interp, NewIntrinsicIDObj(id));
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
