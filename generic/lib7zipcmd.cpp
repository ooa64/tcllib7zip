#include "lib7zipcmd.hpp"
#include "lib7ziparchivecmd.hpp"
#include "lib7zipstream.hpp"

#if defined(LIB7ZIPCMD_DEBUG)
#   include <iostream>
#   define DEBUGLOG(_x_) (std::cerr << "DEBUG: " << _x_ << "\n")
#else
#   define DEBUGLOG(_x_)
#endif

int Lib7ZipCmd::Command (int objc, Tcl_Obj *const objv[]) {
    static const char *const commands[] = {
        "initialize", "isinitialized", "extensions", "open", 0L
    };
    enum commands {
        cmInitialize, cmIsInitialized, cmExtensions, cmOpen
    };
    int index;

    if (objc < 2) {
        Tcl_WrongNumArgs(tclInterp, 1, objv, "subcommand");
        return TCL_ERROR;
    }
    if (Tcl_GetIndexFromObj(tclInterp, objv[1], commands, "subcommand", 0, &index) != TCL_OK) {
        return TCL_ERROR;
    }

    switch ((enum commands)(index)) {

    case cmInitialize:

        if (objc == 2 || objc == 3) {
            if (Initialize(objc == 3 ? objv[2] : NULL) != TCL_OK)
                return TCL_ERROR;
        } else {
            Tcl_WrongNumArgs(tclInterp, 2, objv, "?library?");
            return TCL_ERROR;
        }

        break;

    case cmIsInitialized:

        if (objc == 2) {
            Tcl_SetObjResult(tclInterp, Tcl_NewBooleanObj(lib.IsInitialized()));
        } else {
            Tcl_WrongNumArgs(tclInterp, 2, objv, NULL);
            return TCL_ERROR;
        }

        break;

    case cmExtensions:

        if (objc == 2) {
            if (!lib.IsInitialized() && Initialize(NULL) != TCL_OK)
                return TCL_ERROR;
            if (SupportedExts(Tcl_GetObjResult(tclInterp)) != TCL_OK) {
                return TCL_ERROR;
            }
        } else {
            Tcl_WrongNumArgs(tclInterp, 2, objv, NULL);
            return TCL_ERROR;
        } 

        break;

    case cmOpen:

        // open ?-multivolume? ?-detecttype|-forcetype? ?-password password? -channel -- chan | filename
        if (objc > 2) {
            static const char *const options[] = {
                "-multivolume", "-detecttype", "-forcetype", "-password", "-channel", 0L
            };
            enum options {
                opMultivolume, opDetecttype, opForcetype, opPassword, opChannel
            };
            int index;
            bool multivolume = false;
            bool detecttype = false;
            bool usechannel = false;
            std::wstring password = L"";
            Tcl_Obj *forcetype = NULL;
            for (int i = 2; i < objc - 1; i++) {
                if (Tcl_GetIndexFromObj(tclInterp, objv[i], options, "option", 0, &index) != TCL_OK) {
                    return TCL_ERROR;
                }
                switch ((enum options)(index)) {
                case opMultivolume:
                    multivolume = true;
                    break;
                case opDetecttype:
                    detecttype = true;
                    break;
                case opForcetype:
                    if (i < objc - 2) {
                        forcetype = objv[++i];
                    } else {
                        Tcl_SetObjResult(tclInterp, Tcl_NewStringObj(
                            "\"-forcetype\" option must be followed by type", -1));
                        return TCL_ERROR;
                    }
                    break;
                case opPassword:
                    if (i < objc - 2) {
                        password = convert.from_bytes(Tcl_GetString(objv[++i])).c_str();
                    } else {
                        Tcl_SetObjResult(tclInterp, Tcl_NewStringObj(
                            "\"-password\" option must be followed by password", -1));
                        return TCL_ERROR;
                    }
                    break;
                case opChannel:
                    usechannel = true;
                    break;
                }
            }
            if (detecttype && forcetype != NULL) {
                Tcl_SetObjResult(tclInterp, Tcl_NewStringObj(
                    "only one of options \"-detecttype\" or \"-forcetype\" must be specified", -1));
                return TCL_ERROR;
            }
            if (multivolume && usechannel) {
                Tcl_SetObjResult(tclInterp, Tcl_NewStringObj(
                    "only one of options \"-multivolume\" or \"-channel\" must be specified", -1));
                return TCL_ERROR;
            }

            if (!lib.IsInitialized() && Initialize(NULL) != TCL_OK)
                return TCL_ERROR;

            static unsigned long archiveCounter = 0;
            Tcl_Obj *archiveObj = NULL;
            C7ZipArchive *archive = NULL;
            if (multivolume) {
                Lib7ZipMultiVolumes *volumes = new Lib7ZipMultiVolumes(tclInterp, objv[objc-1], forcetype);
                if (!volumes->Valid()) {
                    delete volumes;
                    return TCL_ERROR;
                }
                if (!lib.OpenMultiVolumeArchive(volumes, &archive, password, detecttype)) {
                    LastError();
                    delete volumes;
                    return TCL_ERROR;
                }
                archiveObj = Tcl_ObjPrintf("sevenzip%lu", archiveCounter++);
                new Lib7ZipArchiveCmd(tclInterp, Tcl_GetString(archiveObj), this, archive, volumes);
            } else {
                Lib7ZipInStream *stream = new Lib7ZipInStream(tclInterp, objv[objc-1], forcetype, usechannel);
                if (!stream->Valid()) {
                    delete stream;
                    return TCL_ERROR;
                }
                if (!lib.OpenArchive(stream, &archive, password, detecttype)) {
                    LastError();
                    delete stream;
                    return TCL_ERROR;
                }
                archiveObj = Tcl_ObjPrintf("sevenzip%lu", archiveCounter++);
                new Lib7ZipArchiveCmd(tclInterp, Tcl_GetString(archiveObj), this, archive, stream);
            }
            Tcl_SetObjResult(tclInterp, archiveObj);
        } else {
            Tcl_WrongNumArgs(tclInterp, 2, objv, "?options? filename");
            return TCL_ERROR;
        } 

        break;

    }

    return TCL_OK;
};

int Lib7ZipCmd::Initialize (Tcl_Obj *dll) {
    if (lib.IsInitialized()) {
        Tcl_SetObjResult(tclInterp, Tcl_NewStringObj("already initialized", -1));
        return TCL_ERROR;
    }
    if (dll) {
        if (lib.Initialize(convert.from_bytes(Tcl_GetString(dll)).c_str())) {
            return TCL_OK;
        }
    } else if (lib.Initialize()) {
        return TCL_OK;
    }
    Tcl_SetObjResult(tclInterp, Tcl_NewStringObj("error loading 7z library", -1));
    return TCL_ERROR;
}

int Lib7ZipCmd::SupportedExts (Tcl_Obj *exts) {
    WStringArray a;
    if (lib.GetSupportedExts(a)) {
        for(size_t i = 0; i < a.size(); i++) {
            Tcl_ListObjAppendElement(tclInterp, exts,
                Tcl_NewStringObj(convert.to_bytes(a[i]).c_str(), -1));
        }
        return TCL_OK;
    }
    Tcl_SetObjResult(tclInterp, Tcl_NewStringObj("error loading 7z library", -1));
    return TCL_ERROR;
}

int Lib7ZipCmd::LastError () {
    lib7zip::ErrorCodeEnum errorcode = lib.GetLastError();
    switch (errorcode) {
    case lib7zip::LIB7ZIP_NO_ERROR:
        Tcl_SetObjResult(tclInterp, Tcl_NewStringObj("no error", -1));
        break;
    case lib7zip::LIB7ZIP_UNKNOWN_ERROR:
        Tcl_SetObjResult(tclInterp, Tcl_NewStringObj("unknown error", -1));
        break;
    case lib7zip::LIB7ZIP_NOT_INITIALIZE:
        Tcl_SetObjResult(tclInterp, Tcl_NewStringObj("not initialized", -1));
        break;
    case lib7zip::LIB7ZIP_NEED_PASSWORD:
        Tcl_SetObjResult(tclInterp, Tcl_NewStringObj("need password", -1));
        break;
    case lib7zip::LIB7ZIP_NOT_SUPPORTED_ARCHIVE:
        Tcl_SetObjResult(tclInterp, Tcl_NewStringObj("not supported", -1));
        break;
    default:
        Tcl_SetObjResult(tclInterp, Tcl_ObjPrintf("unknown error %d", errorcode));
    };  
    return TCL_ERROR;
}
