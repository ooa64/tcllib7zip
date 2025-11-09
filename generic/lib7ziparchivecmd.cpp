#include <string.h>
#include "lib7ziparchivecmd.hpp"
#include "lib7zipstream.hpp"

#if defined(LIB7ZIPARCHIVECMD_DEBUG)
#   include <iostream>
#   define DEBUGLOG(_x_) (std::cerr << "DEBUG: " << _x_ << "\n")
#else
#   define DEBUGLOG(_x_)
#endif

#define DESTROY_ARCHIVE_BUG

#define LIST_MATCH_NOCASE TCL_MATCH_NOCASE
#define LIST_MATCH_EXACT (1 << 16)

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

static int Tcl_StringCaseEqual(const char *str1, const char *str2, int nocase);
static Int64 Time_FileTimeToUnixTime64(UInt64 filetime);
#ifdef _WIN32
static std::string Path_WindowsPathToUnixPath(std::string path);
#endif

Lib7ZipArchiveCmd::Lib7ZipArchiveCmd (Tcl_Interp *interp, const char *name, TclCmd *parent,
        C7ZipArchive *archive, Lib7ZipInStream *stream):
        TclCmd(interp, name, parent), archive(archive), stream(stream), volumes(NULL), convert() {
    DEBUGLOG("Lib7ZipArchiveCmd, archive " << archive << ", stream " << stream);
};

Lib7ZipArchiveCmd::Lib7ZipArchiveCmd (Tcl_Interp *interp, const char *name, TclCmd *parent,
        C7ZipArchive *archive, Lib7ZipMultiVolumes *volumes):
        TclCmd(interp, name, parent), archive(archive), stream(NULL), volumes(volumes), convert() {
    DEBUGLOG("Lib7ZipArchiveCmd, archive " << archive << ", volumes " << volumes);
};

Lib7ZipArchiveCmd::~Lib7ZipArchiveCmd () {
    DEBUGLOG("~Lib7ZipArchiveCmd, archive " << archive << ", stream " << stream << ", volumes " << volumes);
#ifndef DESTROY_ARCHIVE_BUG
    // FIXME: tcl script crashes on 'rename sevenzip ""'
    // WORKAROUND: to reduce memory leaks
    // - call archive->Close in Cleanup for 'rename sevenzipXXX ""  
    // - call archive->Close in cmClose handler for 'sevenzipXXX close'
    //
    DEBUGLOG("~Lib7ZipArchiveCmd, archive destroyer call");
    // NOTE: crashes in C7ZipArchiveImpl::Close when called from parent destructor
    // if (archive)
    //     archive->Close();
    //    
    // NOTE: crashes in CMyComPtr<IInArchive>::Release when called the parent destructor
    if (archive)
        delete archive;
#endif    
    if (stream)
        delete stream;
    if (volumes)
        delete volumes;
}

void Lib7ZipArchiveCmd::Cleanup() {
#ifdef DESTROY_ARCHIVE_BUG
    DEBUGLOG("Lib7ZipArchiveCmd::Cleanup, close archive " << archive);
    // NOTE: see notes for destructor
    if (archive)
        archive->Close();
#endif    
};

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
            if (!Valid())
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
            if (!Valid())
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
                "-info", "-nocase", "-exact", "-type", "--", 0L
            };
            enum options {
                opInfo, opNocase, opExact, opType, opEnd
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
                switch ((enum options)(index)) {
                case opInfo:
                    info = true;
                    continue;
                case opNocase:
                    flags |= LIST_MATCH_NOCASE;
                    continue;
                case opExact:
                    flags |= LIST_MATCH_EXACT;
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
                switch ((enum options)(index)) {
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
#ifdef DESTROY_ARCHIVE_BUG
            // NOTE: see notes for destructor
            DEBUGLOG("Lib7ZipArchiveCmd::Command cmClose, close archive " << archive);
            if (archive)
                archive->Close();
#endif
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
            Tcl_ListObjAppendElement(NULL, info, Tcl_NewStringObj(convert.to_bytes(wstrval).c_str(), -1));
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

#ifdef _WIN32
        std::string path = Path_WindowsPathToUnixPath(convert.to_bytes(item->GetFullPath()).c_str());
#else
        std::string path = convert.to_bytes(item->GetFullPath()).c_str();
#endif
        if (pattern) {
            if (flags & LIST_MATCH_EXACT) {
                if (!Tcl_StringCaseEqual(path.c_str(), Tcl_GetString(pattern), flags & TCL_MATCH_NOCASE))
                    continue;
            } else { 
                if (!Tcl_StringCaseMatch(path.c_str(), Tcl_GetString(pattern), flags & TCL_MATCH_NOCASE))
                    continue;
            }
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
                    std::string strval = convert.to_bytes(wstrval);
#ifdef _WIN32
                    if (prop == lib7zip::kpidPath)
                        strval = Path_WindowsPathToUnixPath(strval);
#endif
                    Tcl_ListObjAppendElement(NULL, propObj, Tcl_NewStringObj(Lib7ZipProperties[prop], -1));
                    Tcl_ListObjAppendElement(NULL, propObj, Tcl_NewStringObj(strval.c_str(), -1));
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
            Tcl_ListObjAppendElement(NULL, list, Tcl_NewStringObj(path.c_str(), -1));
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

#ifdef _WIN32
        std::string path = Path_WindowsPathToUnixPath(convert.to_bytes(item->GetFullPath()).c_str());
#else
        std::string path = convert.to_bytes(item->GetFullPath()).c_str();
#endif
        if (strcmp(path.c_str(), Tcl_GetString(source)) == 0) {
            Lib7ZipOutStream *out = new Lib7ZipOutStream(tclInterp, destination, usechannel);
            if (!out->Valid())
                result = TCL_ERROR;
            else
                if (password) {  
                    if (!archive->Extract(i, out, convert.from_bytes(Tcl_GetString(password))))
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
        return true;
    Tcl_SetObjResult(tclInterp, Tcl_NewStringObj("error opening archive", -1));
    return false;
}

static int Tcl_StringCaseEqual(const char *str1, const char *str2, int nocase) {
    Tcl_Size len1 = Tcl_NumUtfChars(str1, -1);
    Tcl_Size len2 = Tcl_NumUtfChars(str2, -1);
    if (len1 != len2)
        return 0;
    return 0 == (nocase ? Tcl_UtfNcasecmp(str1, str2, len1) : Tcl_UtfNcmp(str1, str2, len1));
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

#ifdef _WIN32
static std::string Path_WindowsPathToUnixPath(std::string path) {
    for (int i = 0; i < path.size(); i++)
    if (path[i] == '\\')
        if (i < (path.size()-1) && path[i+1] == '\\')
            i++;
        else
            path[i] = '/';
    return path;
}
#endif
