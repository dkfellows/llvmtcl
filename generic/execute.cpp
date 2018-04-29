#include <tcl.h>
#include "llvmtcl.h"
#include "version.h"

/*
 * ------------------------------------------------------------------------
 *
 * CreateGenericValueOfTclInterp --
 *	Implements a Tcl command for mapping the current Tcl interpreter as an
 *	LLVM GenericValue.
 *
 * ------------------------------------------------------------------------
 */

int
CreateGenericValueOfTclInterp(
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
/*
 * ------------------------------------------------------------------------
 *
 * CreateGenericValueOfTclObj --
 *	Implements a Tcl command for mapping a Tcl value as an LLVM
 *	GenericValue.
 *
 * ------------------------------------------------------------------------
 */


int
CreateGenericValueOfTclObj(
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

/*
 * ------------------------------------------------------------------------
 *
 * GenericValueToTclObj --
 *	Implements a Tcl command for reinterpreting an LLVM GenericValue (of a
 *	Tcl_Obj*) as a Tcl value in the result.
 *
 * ------------------------------------------------------------------------
 */

int
GenericValueToTclObj(
    ClientData clientData,
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const objv[])
{
    if (objc != 2) {
        Tcl_WrongNumArgs(interp, 1, objv, "GenVal");
        return TCL_ERROR;
    }

    LLVMGenericValueRef arg = nullptr;
    if (GetLLVMGenericValueRefFromObj(interp, objv[1], arg) != TCL_OK)
        return TCL_ERROR;

    Tcl_Obj *rt = (Tcl_Obj *) LLVMGenericValueToPointer(arg);
    Tcl_SetObjResult(interp, rt);
    return TCL_OK;
}

/*
 * ------------------------------------------------------------------------
 *
 * CallInitialisePackageFunction --
 *	Implements a Tcl command for calling a function (assumed to be a
 *	standard Tcl package initialisation function) within the given
 *	execution engine.
 *
 *	This command may cause the interpreter to crash; it is up to the
 *	caller to ensure that the function will leave the system in a defined
 *	state.
 *
 * ------------------------------------------------------------------------
 */

int
CallInitialisePackageFunction(
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

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 4
 * fill-column: 78
 * tab-width: 8
 * End:
 */
