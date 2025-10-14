#ifndef LIB7ZIPARCHIVECMD_H
#define LIB7ZIPARCHIVECMD_H

#include <lib7zip.h>
#include <tcl.h>

#include "tclcmd.hpp"

class Lib7ZipArchiveCmd : public TclCmd {

public:

    Lib7ZipArchiveCmd (Tcl_Interp *interp, const char *name,
        C7ZipArchive *archive, C7ZipInStream *stream);

    Lib7ZipArchiveCmd (Tcl_Interp *interp, const char *name,
        C7ZipArchive *archive, C7ZipMultiVolumes *volumes);

    virtual ~Lib7ZipArchiveCmd();

private:

    C7ZipArchive *archive;
    C7ZipInStream *stream;
    C7ZipMultiVolumes *volumes;

    int Info(Tcl_Obj *info);
    int List(Tcl_Obj *list, Tcl_Obj *pattern, char type, int flags, bool info);
    int Extract(Tcl_Obj *source, Tcl_Obj *destination, Tcl_Obj *password, bool usechannel);

    bool Valid ();

    virtual int Command (int objc, Tcl_Obj * const objv[]);
};

#endif