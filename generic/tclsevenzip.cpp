#include <tcl.h>

#include "tclsevenzipUuid.h"
#include "lib7zipcmd.hpp"

extern "C" {
DLLEXPORT int Sevenzip_Init (Tcl_Interp *interp);
}

int Sevenzip_Init (Tcl_Interp *interp) {
    Tcl_CmdInfo info;

#ifdef USE_TCL_STUBS    
    if (Tcl_InitStubs(interp, "8.5-10", 0) == NULL) {
        return TCL_ERROR;
    }
#endif

    if (Tcl_GetCommandInfo(interp, "::tcl::build-info", &info)) {
        Tcl_CreateObjCommand(interp, "::sevenzip::build-info",
            info.objProc, (void *)(
                PACKAGE_VERSION "+" STRINGIFY(SEVENZIP_VERSION_UUID)
#if defined(__clang__) && defined(__clang_major__)
                ".clang-" STRINGIFY(__clang_major__)
#if __clang_minor__ < 10
                "0"
#endif
                STRINGIFY(__clang_minor__)
#endif
#if defined(__cplusplus) && !defined(__OBJC__)
                ".cplusplus"
#endif
#ifndef NDEBUG
                ".debug"
#endif
#if !defined(__clang__) && !defined(__INTEL_COMPILER) && defined(__GNUC__)
                ".gcc-" STRINGIFY(__GNUC__)
#if __GNUC_MINOR__ < 10
                "0"
#endif
                STRINGIFY(__GNUC_MINOR__)
#endif
#ifdef __INTEL_COMPILER
                ".icc-" STRINGIFY(__INTEL_COMPILER)
#endif
#ifdef TCL_MEM_DEBUG
                ".memdebug"
#endif
#if defined(_MSC_VER)
                ".msvc-" STRINGIFY(_MSC_VER)
#endif
#ifdef USE_NMAKE
                ".nmake"
#endif
#ifndef TCL_CFG_OPTIMIZED
                ".no-optimize"
#endif
#ifdef __OBJC__
                ".objective-c"
#if defined(__cplusplus)
                "plusplus"
#endif
#endif
#ifdef TCL_CFG_PROFILED
                ".profile"
#endif
#ifdef PURIFY
                ".purify"
#endif
#ifdef STATIC_BUILD
                ".static"
#endif
                ), NULL);
    }

    if (Tcl_PkgProvideEx(interp, PACKAGE_NAME, PACKAGE_VERSION, NULL) != TCL_OK) {
        return TCL_ERROR;
    }

    new Lib7ZipCmd(interp, "sevenzip");

    return TCL_OK;
}
