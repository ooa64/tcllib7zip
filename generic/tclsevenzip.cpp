#include <tcl.h>

// #include "tclsevenzipUuid.h"
#include "lib7zipcmd.hpp"

extern "C" {
DLLEXPORT int Sevenzip_Init (Tcl_Interp *interp);
}

int Sevenzip_Init (Tcl_Interp *interp) {

#ifdef USE_TCL_STUBS    
    if (Tcl_InitStubs(interp, "8.5-10", 0) == NULL) {
        return TCL_ERROR;
    }
#endif

    if (Tcl_PkgProvideEx(interp, PACKAGE_NAME, PACKAGE_VERSION, NULL) != TCL_OK) {
        return TCL_ERROR;
    }

    new Lib7ZipCmd(interp, "sevenzip");

    return TCL_OK;
}
