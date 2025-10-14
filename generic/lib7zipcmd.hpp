#ifndef LIB7ZIPCMD_H
#define LIB7ZIPCMD_H

#include <lib7zip.h>
#include <tcl.h>

#include "tclcmd.hpp"

class Lib7ZipCmd : public TclCmd {

public:

    // TODO: lib destruction?
    Lib7ZipCmd (Tcl_Interp * interp, const char * name): TclCmd(interp, name), lib() {};

private:

    C7ZipLibrary lib;

    int Initialized ();
    int SupportedExts (Tcl_Obj * exts);
    int LastError ();

    virtual int Command (int objc, Tcl_Obj * const objv[]);
};

#endif
