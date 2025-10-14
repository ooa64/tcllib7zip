#include "lib7ziparchivecmd.hpp"
#include "lib7zipstream.hpp"

#if defined(LIB7ZIPARCHIVECMD_DEBUG)
#   include <iostream>
#   define DEBUGLOG(_x_) (std::cerr << "DEBUG: " << _x_ << "\n")
#else
#   define DEBUGLOG(_x_)
#endif

// NOTE: should match lib7zip::PropertyIndexEnum
static const char *const Lib7ZipProperties[] = {
    "packsize",
    "attrib",
    "ctime",
    "atime",
    "mtime",
    "solid",
    "encrypted",
    "user",
    "group",
    "comment",
    "physize",
    "headerssize",
    "checksum",
    "characts",
    "creatorapp",
    "totalsize",
    "freespace",
    "clustersize",
    "volumename",
    "path",
    "isdir",
    "size",
    0L
};

Int64 Time_FileTimeToUnixTime64(UInt64 filetime);

int WChar_StringMatch(std::wstring wstr, char *pattern, int flags);
int WChar_StringEqual(std::wstring wstr, char *str);

Lib7ZipArchiveCmd::Lib7ZipArchiveCmd (Tcl_Interp *interp, const char *name,
        C7ZipArchive *archive, C7ZipInStream *stream):
        TclCmd(interp, name), archive(archive), stream(stream), volumes(NULL) {
    DEBUGLOG("Lib7ZipArchiveCmd, archive " << archive << ", stream " << stream);
};

Lib7ZipArchiveCmd::Lib7ZipArchiveCmd (Tcl_Interp *interp, const char *name,
        C7ZipArchive *archive, C7ZipMultiVolumes *volumes):
        TclCmd(interp, name), archive(archive), stream(NULL), volumes(volumes) {
    DEBUGLOG("Lib7ZipArchiveCmd, archive " << archive << ", volumes " << volumes);
};

Lib7ZipArchiveCmd::~Lib7ZipArchiveCmd () {
    DEBUGLOG("~Lib7ZipArchiveCmd, archive " << archive << ", stream " << stream << ", volumes " << volumes);
    if (archive)
        delete archive;
    if (stream)
        delete stream;
    if (volumes)
        delete volumes;
}

int Lib7ZipArchiveCmd::Command (int objc, Tcl_Obj *const objv[]) {
    static const char *const commands[] = {
        "info", "count", "list", "extract", "close", 0L
    };
    enum commands {
        cmInfo, cmCount, cmList, cmExtract, cmClose
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

    case cmInfo:

        if (objc == 2) {
            if (Valid() != TCL_OK)
                return TCL_ERROR;
            if (Info(Tcl_GetObjResult(tclInterp)) != TCL_OK)
                return TCL_ERROR;
        } else {
            Tcl_WrongNumArgs(tclInterp, 2, objv, NULL);
            return TCL_ERROR;
        } 
        break;

    case cmCount:

        if (objc == 2) {
            unsigned int count;
            if (Valid() != TCL_OK)
                return TCL_ERROR;
            if (!archive->GetItemCount(&count)) // NOTE: always OK
                return TCL_ERROR;
            Tcl_SetObjResult(tclInterp, Tcl_NewIntObj(count));
        } else {
            Tcl_WrongNumArgs(tclInterp, 2, objv, NULL);
            return TCL_ERROR;
        } 
        break;

    case cmList:

        if (objc >= 2) {
            static const char * const options[] = {
                "-info", "-nocase", "-type", "--", 0L
            };
            enum options {
                opInfo, opNocase, opType, opEnd
            };
            int index;
            int flags = 0;
            char type = 'a';
            bool info = false;
            Tcl_Obj *patternObj = NULL;
            for (int i = 2; i < objc; i++) {
                if (Tcl_GetIndexFromObj(tclInterp, objv[i], options, "option", 0, &index) != TCL_OK) {
                    if (i < objc - 1)
                        return TCL_ERROR;
                    Tcl_ResetResult(tclInterp);
                    patternObj = objv[i];
                    break;
                }
                switch ((enum commands)(index)) {
                case opInfo:
                    info = true;
                    continue;
                case opNocase:
                    flags = TCL_MATCH_NOCASE;
                    continue;
                case opType:
                    i++;
                    if (i < objc && Tcl_GetCharLength(objv[i]) == 1) {
                        type = Tcl_GetString(objv[i])[0];
                        if (type == 'd' || type == 'f')
                            continue;
                    }
                    Tcl_SetObjResult(tclInterp, Tcl_NewStringObj(
                            "\"-type\" option must be followed by \"d\" or \"f\"", -1));
                    return TCL_ERROR;
                case opEnd:
                    if (i == objc - 2)
                        patternObj = objv[i+1];
                    if (i >= objc - 2)
                        break;
                    Tcl_SetObjResult(tclInterp, Tcl_NewStringObj(
                            "\"--\" option can be followed by pattern only", -1));
                    return TCL_ERROR;
                };
                break;
            };
            if (List(Tcl_GetObjResult(tclInterp), patternObj, type, flags, info) != TCL_OK)
                return TCL_ERROR;
        } else {
            Tcl_WrongNumArgs(tclInterp, 2, objv, "?options? ?pattern?");
            return TCL_ERROR;
        }
        break;

    case cmExtract:
        if (objc >= 4) {
            static const char * const options[] = {
                "-password", "-channel", 0L
            };
            enum options {
                opPassword, opChannel
            };
            int index;
            bool usechannel = false;
            Tcl_Obj *password = NULL;
            for (int i = 2; i < objc - 2; i++) {
                if (Tcl_GetIndexFromObj(tclInterp, objv[i], options, "option", 0, &index) != TCL_OK) {
                    return TCL_ERROR;
                }
                switch ((enum commands)(index)) {
                case opPassword:
                    if (i < objc - 3) {
                        password = objv[++i];
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
            if (Extract(objv[objc-2], objv[objc-1], password, usechannel) != TCL_OK)
                return TCL_ERROR;
        } else {
            Tcl_WrongNumArgs(tclInterp, 2, objv, "?options? item path");
            return TCL_ERROR;
        }
        break;

    case cmClose:
        if (objc > 2) {
            Tcl_WrongNumArgs(tclInterp, 2, objv, NULL);
            return TCL_ERROR;
        } else {
            delete this;
        }
        break;
    }

    return TCL_OK;
};

int Lib7ZipArchiveCmd::Info(Tcl_Obj *info) {
    for (int prop = lib7zip::PROP_INDEX_BEGIN; prop < lib7zip::PROP_INDEX_END; prop++) {
        UInt64 intval;
        if (archive->GetUInt64Property((lib7zip::PropertyIndexEnum)prop, intval)) {
            Tcl_ListObjAppendElement(NULL, info, Tcl_NewStringObj(Lib7ZipProperties[prop], -1));
            Tcl_ListObjAppendElement(NULL, info, Tcl_NewWideIntObj(intval));
            continue;
        }
        bool boolval;
        if (archive->GetBoolProperty((lib7zip::PropertyIndexEnum)prop, boolval)) {
            Tcl_ListObjAppendElement(NULL, info, Tcl_NewStringObj(Lib7ZipProperties[prop], -1));
            Tcl_ListObjAppendElement(NULL, info, Tcl_NewBooleanObj(boolval));
            continue;
        }
        std::wstring wstrval;
        if (archive->GetStringProperty((lib7zip::PropertyIndexEnum)prop, wstrval)) {
            Tcl_ListObjAppendElement(NULL, info, Tcl_NewStringObj(Lib7ZipProperties[prop], -1));
            Tcl_ListObjAppendElement(NULL, info, Tcl_NewUnicodeObj((const Tcl_UniChar *)wstrval.c_str(), -1));
            continue;
        }
        UInt64 timeval;
        if (archive->GetFileTimeProperty((lib7zip::PropertyIndexEnum)prop, timeval)) {
            Tcl_ListObjAppendElement(NULL, info, Tcl_NewStringObj(Lib7ZipProperties[prop], -1));
            Tcl_ListObjAppendElement(NULL, info, Tcl_NewWideIntObj(Time_FileTimeToUnixTime64(timeval)));
            continue;
        }
        DEBUGLOG("Lib7ZipArchiveCmd unhandled prop " << Lib7ZipProperties[prop]);
    }
    return TCL_OK;
}

int Lib7ZipArchiveCmd::List(Tcl_Obj *list, Tcl_Obj *pattern, char type, int flags, bool info) {
    unsigned int count;
    if (!archive->GetItemCount(&count)) // NOTE: always OK
        return TCL_ERROR;
    for (unsigned int i = 0; i < count; ++i) {
        C7ZipArchiveItem *item;
        if (!archive->GetItemInfo(i, &item)) // NOTE: always OK
            return TCL_ERROR;

        if (type == 'd' && !item->IsDir())
            continue;
        if (type == 'f' && item->IsDir())
            continue;
        if (pattern && !WChar_StringMatch(item->GetFullPath(), Tcl_GetString(pattern), flags)) {
            continue;
        }
        if (info) {
            Tcl_Obj *propObj = Tcl_NewObj();
            for (int prop = lib7zip::PROP_INDEX_BEGIN; prop < lib7zip::PROP_INDEX_END; prop++) {
                UInt64 intval;
                if (item->GetUInt64Property((lib7zip::PropertyIndexEnum)prop, intval)) {
                    Tcl_ListObjAppendElement(NULL, propObj, Tcl_NewStringObj(Lib7ZipProperties[prop], -1));
                    Tcl_ListObjAppendElement(NULL, propObj, Tcl_NewWideIntObj(intval));
                    continue;
                }
                bool boolval;
                if (item->GetBoolProperty((lib7zip::PropertyIndexEnum)prop, boolval)) {
                    Tcl_ListObjAppendElement(NULL, propObj, Tcl_NewStringObj(Lib7ZipProperties[prop], -1));
                    Tcl_ListObjAppendElement(NULL, propObj, Tcl_NewBooleanObj(boolval));
                    continue;
                }
                std::wstring wstrval;
                if (item->GetStringProperty((lib7zip::PropertyIndexEnum)prop, wstrval)) {
                    Tcl_ListObjAppendElement(NULL, propObj, Tcl_NewStringObj(Lib7ZipProperties[prop], -1));
                    Tcl_ListObjAppendElement(NULL, propObj, Tcl_NewUnicodeObj((const Tcl_UniChar *)wstrval.c_str(), -1));
                    continue;
                }
                UInt64 timeval;
                if (item->GetFileTimeProperty((lib7zip::PropertyIndexEnum)prop, timeval)) {
                    Tcl_ListObjAppendElement(NULL, propObj, Tcl_NewStringObj(Lib7ZipProperties[prop], -1));
                    Tcl_ListObjAppendElement(NULL, propObj, Tcl_NewWideIntObj(Time_FileTimeToUnixTime64(timeval)));
                    continue;
                }
                DEBUGLOG("Lib7ZipArchiveCmd unhandled item prop " << Lib7ZipProperties[prop]);
            }
            Tcl_ListObjAppendElement(NULL, list, propObj);
        } else {
            Tcl_ListObjAppendElement(NULL, list,
                    Tcl_NewUnicodeObj((const Tcl_UniChar *)item->GetFullPath().c_str(), -1));
        }
    }
    return TCL_OK;
}

int Lib7ZipArchiveCmd::Extract(Tcl_Obj *source, Tcl_Obj *destination, Tcl_Obj *password, bool usechannel) {
    int result = TCL_OK;
    unsigned int count;
    if (!archive->GetItemCount(&count)) // NOTE: always OK
        return TCL_ERROR;
    unsigned int i;
    for (i = 0; i < count; ++i) {
        C7ZipArchiveItem *item;
        if (!archive->GetItemInfo(i, &item)) // NOTE: always OK
            return TCL_ERROR;
        if (item->IsDir())
            continue;
        if (WChar_StringEqual(item->GetFullPath(), Tcl_GetString(source))) {
            Lib7ZipOutStream *out = new Lib7ZipOutStream(tclInterp, destination, usechannel);
            if (!out->Valid())
                result = TCL_ERROR;
            else
                if (password) {  
                    if (!archive->Extract(i, out, (wchar_t *)Tcl_GetUnicode(password)))
                        result = TCL_ERROR;
                } else {
                    if (!archive->Extract(i, out))
                        result = TCL_ERROR;
                }
            delete out;
            // TODO: restore attrs ???
            // item->GetUInt64Property(lib7zip::kpidAttrib, &attr);
            // item->GetUInt64Property(lib7zip::kpidMTime, &mtime);
            break;
        }
    }
    if (i >= count) {
        Tcl_SetObjResult(tclInterp, Tcl_ObjPrintf("no such item \"%s\" in the archive", Tcl_GetString(source)));
        result = TCL_ERROR;
    }
    return result;
}

bool Lib7ZipArchiveCmd::Valid () {
    if (archive)
        return TCL_OK;
    Tcl_SetObjResult(tclInterp, Tcl_NewStringObj("error opening archive", -1));
    return TCL_ERROR;
}

typedef struct
{
  uint32_t dwLowDateTime;
  uint32_t dwHighDateTime;
} FILETIME;

#define TICKS_PER_SEC 10000000
#define GET_TIME_64(ft) ((ft).dwLowDateTime | ((UInt64)(ft).dwHighDateTime << 32))
#define SET_FILETIME(ft, v64) \
   (ft).dwLowDateTime = (uint32_t)v64; \
   (ft).dwHighDateTime = (uint32_t)(v64 >> 32);

static const UInt32 kNumTimeQuantumsInSecond = 10000000;
static const UInt32 kFileTimeStartYear = 1601;
static const UInt32 kUnixTimeStartYear = 1970;

static Int64 Time_FileTimeToUnixTime64(UInt64 filetime)
{
    const UInt64 kUnixTimeOffset =
        (UInt64)60 * 60 * 24 * (89 + 365 * (kUnixTimeStartYear - kFileTimeStartYear));
    const UInt64 winTime = GET_TIME_64(*(FILETIME*)&filetime);
    return (Int64)(winTime / kNumTimeQuantumsInSecond) - (Int64)kUnixTimeOffset;
}

static int WChar_StringMatch(std::wstring wstr, char *pattern, int flags) {
    Tcl_DString ds;
    Tcl_DStringInit(&ds);
    char *s = Tcl_UniCharToUtfDString((const Tcl_UniChar *)wstr.c_str(), (int)wstr.size(), &ds);
    int match = Tcl_StringCaseMatch(s, pattern, flags);
    Tcl_DStringFree(&ds);
    return match;
}

static int WChar_StringEqual(std::wstring wstr, char *str) {
    Tcl_DString ds;
    Tcl_DStringInit(&ds);
    char *s = Tcl_UniCharToUtfDString((const Tcl_UniChar *)wstr.c_str(), (int)wstr.size(), &ds);
    int equal = (strcmp(str, s) == 0);
    Tcl_DStringFree(&ds);
    return equal;
}
