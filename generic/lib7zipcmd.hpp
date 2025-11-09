#ifndef LIB7ZIPCMD_H
#define LIB7ZIPCMD_H

#include <locale>
#include <codecvt>
#include <lib7zip.h>
#include <tcl.h>

#include "tclcmd.hpp"

class Lib7ZipCmd : public TclCmd {

public:

    Lib7ZipCmd (Tcl_Interp * interp, const char * name): TclCmd(interp, name), lib(), convert() {};

    virtual ~Lib7ZipCmd () {};

private:

    C7ZipLibrary lib;
    std::wstring_convert<std::codecvt_utf8<wchar_t>> convert;

    int Initialize (Tcl_Obj * dll);
    int SupportedExts (Tcl_Obj * exts);
    int LastError ();

    virtual int Command (int objc, Tcl_Obj * const objv[]);
};

#endif
