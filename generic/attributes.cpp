/*
 * attributes.cpp --
 *
 *	This file contains handling of LLVM's attributes. Can't be mapped
 *	ordinarily as there are more attribute flags than bits in a 32-bit
 *	integer.
 *
 * Copyright (c) 2016-2018 Donal K. Fellows.
 *
 * See the file "license.terms" for information on usage and redistribution of
 * this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

#include "llvmtcl.h"
#include "version.h"
#include "llvm/IR/CallSite.h"

using namespace llvm;

/*
 * ----------------------------------------------------------------------
 *
 * Definition of the mapping between attribute names and their kinds.
 * There's no simple two-way functions for this.
 *
 * ----------------------------------------------------------------------
 */

struct AttributeMapper {
    const char *name;
    const Attribute::AttrKind kind;
};

#define ATTRDEF(a, b) {#b, Attribute::a}
static const AttributeMapper attrMap[] = {
    ATTRDEF(Alignment,		align),
    ATTRDEF(StackAlignment,	alignstack),
    ATTRDEF(AlwaysInline,	alwaysinline),
    ATTRDEF(ArgMemOnly,		argmemonly),
    ATTRDEF(Builtin,		builtin),
    ATTRDEF(ByVal,		byval),
    ATTRDEF(Cold,		cold),
    ATTRDEF(Convergent,		convergent),
    ATTRDEF(Dereferenceable,	dereferenceable),
    ATTRDEF(DereferenceableOrNull, dereferenceable_or_null),
    ATTRDEF(InAlloca,		inalloca),
    ATTRDEF(InReg,		inreg),
    ATTRDEF(InlineHint,		inlinehint),
    ATTRDEF(JumpTable,		jumptable),
    ATTRDEF(MinSize,		minsize),
    ATTRDEF(Naked,		naked),
    ATTRDEF(Nest,		nest),
    ATTRDEF(NoAlias,		noalias),
    ATTRDEF(NoBuiltin,		nobuiltin),
    ATTRDEF(NoCapture,		nocapture),
    ATTRDEF(NoDuplicate,	noduplicate),
    ATTRDEF(NoImplicitFloat,	noimplicitfloat),
    ATTRDEF(NoInline,		noinline),
    ATTRDEF(NonLazyBind,	nonlazybind),
    ATTRDEF(NonNull,		nonnull),
    ATTRDEF(NoRedZone,		noredzone),
    ATTRDEF(NoReturn,		noreturn),
    ATTRDEF(NoUnwind,		nounwind),
    ATTRDEF(OptimizeNone,	optnone),
    ATTRDEF(OptimizeForSize,	optsize),
    ATTRDEF(ReadNone,		readnone),
    ATTRDEF(ReadOnly,		readonly),
    ATTRDEF(Returned,		returned),
    ATTRDEF(ReturnsTwice,	returns_twice),
    ATTRDEF(SafeStack,		safestack),
    ATTRDEF(SanitizeAddress,	sanitize_address),
    ATTRDEF(SanitizeMemory,	sanitize_memory),
    ATTRDEF(SanitizeThread,	sanitize_thread),
    ATTRDEF(SExt,		signext),
    ATTRDEF(StructRet,		sret),
    ATTRDEF(StackProtect,	ssp),
    ATTRDEF(StackProtectReq,	sspreq),
    ATTRDEF(StackProtectStrong,	sspstrong),
    ATTRDEF(UWTable,		uwtable),
    ATTRDEF(ZExt,		zeroext),
    {NULL,Attribute::NonNull}
    //Any value will do, so long as the NULL is there
};
#undef ATTRDEF

/*
 * ----------------------------------------------------------------------
 *
 * GetAttrFromObj --
 *
 *	Gets the kind of attribute that is described by a particular
 *	Tcl_Obj.
 *
 * ----------------------------------------------------------------------
 */

static int
GetAttrFromObj(
    Tcl_Interp *interp,
    Tcl_Obj *obj,
    Attribute::AttrKind &attr)
{
    int index;
    if (Tcl_GetIndexFromObjStruct(interp, obj, (const void *) attrMap,
	    sizeof(AttributeMapper), "attribute", TCL_EXACT,
	    &index) != TCL_OK)
	return TCL_ERROR;
    attr = attrMap[index].kind;
    return TCL_OK;
}

/*
 * ----------------------------------------------------------------------
 *
 * DescribeAttributes --
 *
 *	Converts a set of attributes into the list of descriptions of
 *	attributes that apply to a particular slot.
 *
 * ----------------------------------------------------------------------
 */

template<typename T>
static Tcl_Obj *
DescribeAttributes(
    T attrs,
    unsigned slot)
{
    auto list = Tcl_NewObj();
    for (auto map = &attrMap[0] ; map->name ; map++)
	if (attrs.hasAttribute(
#ifndef API_5
		slot,
#endif // !API_5
		map->kind))
	    Tcl_ListObjAppendElement(NULL, list, NewObj(map->name));
    return list;
}

/*
 * ----------------------------------------------------------------------
 *
 * AddFunctionAttr --
 *
 *	Command implementation for [llvm::AddFunctionAttr].
 *
 * ----------------------------------------------------------------------
 */

int
AddFunctionAttr(
    ClientData clientData,
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const objv[])
{
    if (objc != 3) {
	Tcl_WrongNumArgs(interp, 1, objv, "Fn PA");
	return TCL_ERROR;
    }
    Function *func;
    if (GetValueFromObj(interp, objv[1], "expected function",
	    func) != TCL_OK)
        return TCL_ERROR;
    Attribute::AttrKind attr;
    if (GetAttrFromObj(interp, objv[2], attr) != TCL_OK)
	return TCL_ERROR;
    func->addFnAttr(attr);
    return TCL_OK;
}

/*
 * ----------------------------------------------------------------------
 *
 * RemoveFunctionAttr --
 *
 *	Command implementation for [llvm::RemoveFunctionAttr].
 *
 * ----------------------------------------------------------------------
 */

int
RemoveFunctionAttr(
    ClientData clientData,
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const objv[])
{
    if (objc != 3) {
	Tcl_WrongNumArgs(interp, 1, objv, "Fn PA");
	return TCL_ERROR;
    }
    Function *func;
    if (GetValueFromObj(interp, objv[1], "expected function",
	    func) != TCL_OK)
        return TCL_ERROR;
    Attribute::AttrKind attr;
    if (GetAttrFromObj(interp, objv[2], attr) != TCL_OK)
	return TCL_ERROR;
    func->removeFnAttr(attr);
    return TCL_OK;
}

/*
 * ----------------------------------------------------------------------
 *
 * GetFunctionAttr --
 *
 *	Command implementation for [llvm::GetFunctionAttr].
 *
 * ----------------------------------------------------------------------
 */

int
GetFunctionAttr(
    ClientData clientData,
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const objv[])
{
    if (objc != 2) {
	Tcl_WrongNumArgs(interp, 1, objv, "Fn");
	return TCL_ERROR;
    }
    Function *func;
    if (GetValueFromObj(interp, objv[1], "expected function",
	    func) != TCL_OK)
        return TCL_ERROR;
    auto attrs = func->getAttributes();
#ifdef API_5
    auto fa = attrs.getAttributes(AttributeList::FunctionIndex);
    Tcl_Obj *result = DescribeAttributes(fa, 0);
#else //!API_5
    Tcl_Obj *result = DescribeAttributes(attrs, AttributeSet::FunctionIndex);
#endif
    Tcl_SetObjResult(interp, result);
    return TCL_OK;
}

/*
 * ----------------------------------------------------------------------
 *
 * AddAttribute --
 *
 *	Command implementation for [llvm::AddArgumentAttribute].
 *
 * ----------------------------------------------------------------------
 */

int
AddAttribute(
    ClientData clientData,
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const objv[])
{
    Argument *arg;
    Attribute::AttrKind attr;

#ifdef API_5
    if (objc != 3 && objc != 4) {
	Tcl_WrongNumArgs(interp, 1, objv, "FnArg PA ?Int?");
	return TCL_ERROR;
    }
    if (GetValueFromObj(interp, objv[1], "expected function argument",
	    arg) != TCL_OK)
        return TCL_ERROR;
    if (GetAttrFromObj(interp, objv[2], attr) != TCL_OK)
	return TCL_ERROR;
    if (AttributeFuncs::typeIncompatible(arg->getType()).contains(attr)) {
	SetStringResult(interp,
		"attribute cannot be applied to arguments of that type");
	return TCL_ERROR;
    }

    if (objc == 4) {
	// TODO: parsing of value should depend on kind of attribute
	int payload;
	if (Tcl_GetIntFromObj(interp, objv[3], &payload) != TCL_OK)
	    return TCL_ERROR;
	arg->addAttr(Attribute::get(arg->getContext(), attr, payload));
    } else {
	arg->addAttr(attr);
    }
#else // !API_5
    // The API for handling values before LLVM 5 was really nasty!
    if (objc != 3) {
	Tcl_WrongNumArgs(interp, 1, objv, "FnArg PA");
	return TCL_ERROR;
    }
    if (GetValueFromObj(interp, objv[1], "expected function argument",
	    arg) != TCL_OK)
	return TCL_ERROR;
    if (GetAttrFromObj(interp, objv[2], attr) != TCL_OK)
	return TCL_ERROR;

    arg->addAttr(AttributeSet::get(arg->getContext(), arg->getArgNo(),
	    attr));
#endif // API_5
    return TCL_OK;
}

/*
 * ----------------------------------------------------------------------
 *
 * RemoveAttribute --
 *
 *	Command implementation for [llvm::RemoveArgumentAttribute].
 *
 * ----------------------------------------------------------------------
 */

int
RemoveAttribute(
    ClientData clientData,
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const objv[])
{
    if (objc != 3) {
	Tcl_WrongNumArgs(interp, 1, objv, "FnArg PA");
	return TCL_ERROR;
    }
    Argument *arg;
    if (GetValueFromObj(interp, objv[1], "expected function argument",
	    arg) != TCL_OK)
        return TCL_ERROR;
    Attribute::AttrKind attr;
    if (GetAttrFromObj(interp, objv[2], attr) != TCL_OK)
	return TCL_ERROR;
#ifdef API_5
    arg->removeAttr(attr);
#else // !API_5
    arg->removeAttr(AttributeSet::get(arg->getContext(),
	    arg->getArgNo(), attr));
#endif // API5
    return TCL_OK;
}

/*
 * ----------------------------------------------------------------------
 *
 * GetAttribute --
 *
 *	Command implementation for [llvm::GetArgumentAttribute].
 *
 * ----------------------------------------------------------------------
 */

int
GetAttribute(
    ClientData clientData,
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const objv[])
{
    if (objc != 2) {
	Tcl_WrongNumArgs(interp, 1, objv, "FnArg");
	return TCL_ERROR;
    }
    Argument *arg;
    if (GetValueFromObj(interp, objv[1], "expected function argument",
	    arg) != TCL_OK)
        return TCL_ERROR;
    auto func = arg->getParent();
    auto attrs = func->getAttributes();
#ifdef API_5
    auto pa = attrs.getParamAttributes(arg->getArgNo());
    Tcl_SetObjResult(interp, DescribeAttributes(pa, 0));
#else // !API_5
    Tcl_SetObjResult(interp,
	    DescribeAttributes(attrs, arg->getArgNo()));
#endif // API_5
    return TCL_OK;
}

/*
 * ----------------------------------------------------------------------
 *
 * AddInstrAttribute --
 *
 *	Command implementation for [llvm::AddCallAttribute].
 *
 * ----------------------------------------------------------------------
 */

int
AddInstrAttribute(
    ClientData clientData,
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const objv[])
{
    if (objc != 4) {
	Tcl_WrongNumArgs(interp, 1, objv, "Instr index PA");
	return TCL_ERROR;
    }
    Instruction *instr;
    if (GetValueFromObj(interp, objv[1], "expected function argument",
	    instr) != TCL_OK)
        return TCL_ERROR;
    CallSite call(instr);
    if (!instr) {
	SetStringResult(interp, "expected call or invoke instruction");
	return TCL_ERROR;
    }
    int iarg2 = 0;
    if (Tcl_GetIntFromObj(interp, objv[2], &iarg2) != TCL_OK)
        return TCL_ERROR;
    unsigned index = (unsigned)iarg2;
    Attribute::AttrKind attr;
    if (GetAttrFromObj(interp, objv[3], attr) != TCL_OK)
	return TCL_ERROR;
    call.setAttributes(call.getAttributes().addAttribute(
	    instr->getContext(), index, attr));
    return TCL_OK;
}

/*
 * ----------------------------------------------------------------------
 *
 * RemoveInstrAttribute --
 *
 *	Command implementation for [llvm::RemoveCallAttribute].
 *
 * ----------------------------------------------------------------------
 */

int
RemoveInstrAttribute(
    ClientData clientData,
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const objv[])
{
    if (objc != 4) {
	Tcl_WrongNumArgs(interp, 1, objv, "Instr index PA");
	return TCL_ERROR;
    }
    Instruction *instr;
    if (GetValueFromObj(interp, objv[1], "expected function argument",
	    instr) != TCL_OK)
        return TCL_ERROR;
    CallSite call(instr);
    if (!instr) {
	SetStringResult(interp, "expected call or invoke instruction");
	return TCL_ERROR;
    }
    int iarg2 = 0;
    if (Tcl_GetIntFromObj(interp, objv[2], &iarg2) != TCL_OK)
        return TCL_ERROR;
    unsigned index = unsigned(iarg2);
    Attribute::AttrKind attr;
    if (GetAttrFromObj(interp, objv[3], attr) != TCL_OK)
	return TCL_ERROR;
    call.setAttributes(call.getAttributes().removeAttribute(
	    instr->getContext(), index, attr));
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
