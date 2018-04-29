#include <tcl.h>
#include <llvm/IR/Verifier.h>
#include "llvmtcl.h"
#include "version.h"

/*
 * ------------------------------------------------------------------------
 *
 * VerifyFunction --
 *	Implements a Tcl command for checking the correctness of a function.
 *
 *	This function is only needed because the C wrapper for the underlying
 *	llvm::verifyFunction() call sucks.
 *
 * ------------------------------------------------------------------------
 */

int VerifyFunction(
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

/*
 * ------------------------------------------------------------------------
 *
 * VerifyModule --
 *	Implements a Tcl command for checking the correctness of a module.
 *
 *	This function is only needed because the C wrapper for the underlying
 *	llvm::verifyModule() call sucks.
 *
 * ------------------------------------------------------------------------
 */

int VerifyModule(
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
    std::vector<tcl::value> result {
	NewObj(failedVerification),
	NewObj(Messages)
    };
    Tcl_SetObjResult(interp, NewObj(result));
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
