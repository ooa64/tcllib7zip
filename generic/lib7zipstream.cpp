#include "lib7zipstream.hpp"

#if defined(LIB7ZIPSTREAM_DEBUG)
#   include <iostream>
#   define DEBUGLOG(_x_) (std::cerr << "DEBUG: " << _x_ << "\n")
#else
#   define DEBUGLOG(_x_)
#endif

// tclDecl.h
EXTERN const char * TclGetExtension(const char *name);

Lib7ZipInStream::Lib7ZipInStream(Tcl_Interp *interp, Tcl_Obj *file, Tcl_Obj *type, bool usechannel):
        tclInterp(interp), tclChannel(NULL), name(L""), ext(L""), closechannel(!usechannel) {
    DEBUGLOG("Lib7ZipInStream to open " << Tcl_GetString(file) << " as chan " << usechannel);
    if (usechannel) {
        int mode;
        tclChannel = Tcl_GetChannel(interp, Tcl_GetString(file), &mode);
        if (tclChannel == NULL) {
            return;
        }
        if ((mode & TCL_READABLE) != TCL_READABLE) {
            Tcl_SetObjResult(tclInterp, Tcl_ObjPrintf(
                "channel \"%s\" wasn't opened for reading", Tcl_GetString(file)));
            tclChannel = NULL;
            return;
        }
        if (Tcl_SetChannelOption(tclInterp, tclChannel, "-translation", "binary") != TCL_OK ||
                Tcl_SetChannelOption(tclInterp, tclChannel, "-blocking", "0") != TCL_OK) {
            tclChannel = NULL;
            return;
        }
    } else {
        tclChannel = Tcl_FSOpenFileChannel(tclInterp, file, "rb", 0644);
        if (tclChannel == NULL) {
            Tcl_SetObjResult(tclInterp, Tcl_ObjPrintf(
                "couldn't read file \"%s\": %s", Tcl_GetString(file), Tcl_PosixError(tclInterp)));
            return;
        }
    }
    name = (wchar_t *)Tcl_GetUnicode(file);
    if (type) {
        ext = (wchar_t *)Tcl_GetUnicode(type);
    } else if (!usechannel) {
        const char *e = TclGetExtension(Tcl_GetString(file));
        if (e && *e == '.') {
            Tcl_DString ds;
            Tcl_DStringInit(&ds);
            ext = (wchar_t *)Tcl_UtfToUniCharDString(e + 1, -1, &ds);
            Tcl_DStringFree(&ds);
        }
    }
    DEBUGLOG("Lib7ZipInStream was open "
        << Tcl_GetString(Tcl_NewUnicodeObj((const Tcl_UniChar *)name.c_str(), -1)) << " as "
        << Tcl_GetString(Tcl_NewUnicodeObj((const Tcl_UniChar *)ext.c_str(), -1)));
}

Lib7ZipInStream::~Lib7ZipInStream() {
    DEBUGLOG("~Lib7ZipInStream");
    if (tclChannel && closechannel)
        Tcl_Close(tclInterp, tclChannel);
}

int Lib7ZipInStream::Read(void *data, unsigned int size, unsigned int *processedSize) {
    DEBUGLOG("Lib7ZipInStream::Read " << size);
    if (tclChannel) {
        unsigned int read = Tcl_Read(tclChannel, (char *)data, size);
        if (read >= 0) {
            if (processedSize)
                *processedSize = read;
            return 0;
        }
    }
    return 1;
}

int Lib7ZipInStream::Seek(__int64 offset, unsigned int seekOrigin, UInt64 *newPosition) {
    DEBUGLOG("Lib7ZipInStream::Seek " << offset << " as " << seekOrigin);
    if (tclChannel) {
        Tcl_WideInt pos = Tcl_Seek(tclChannel, offset, seekOrigin);
        if (pos >= 0) {
            if (newPosition)
                *newPosition = pos;
            return 0;
        }
    }
    return 1;
}

int Lib7ZipInStream::GetSize(UInt64 *size) {
    Tcl_Panic("Lib7ZipInStream::GetSize");
    // NOTE: dummy, not used by lib7zip
    return -1;
}


Lib7ZipOutStream::Lib7ZipOutStream(Tcl_Interp *interp, Tcl_Obj *file, bool usechannel):
        tclInterp(interp), tclChannel(NULL), closechannel(!usechannel) {
    DEBUGLOG("Lib7ZipOutStream to open " << Tcl_GetString(file) << " as chan " << usechannel);
    if (usechannel) {
        int mode;
        tclChannel = Tcl_GetChannel(interp, Tcl_GetString(file), &mode);
        if (tclChannel == NULL) {
            return;
        }
        if ((mode & TCL_WRITABLE) != TCL_WRITABLE) {
            Tcl_SetObjResult(tclInterp, Tcl_ObjPrintf(
                "channel \"%s\" wasn't opened for writing", Tcl_GetString(file)));
            tclChannel = NULL;
            return;
        }
        if (Tcl_SetChannelOption(tclInterp, tclChannel, "-translation", "binary") != TCL_OK ||
                Tcl_SetChannelOption(tclInterp, tclChannel, "-blocking", "0") != TCL_OK) {
            tclChannel = NULL;
            return;
        }
    } else {
        tclChannel = Tcl_FSOpenFileChannel(tclInterp, file, "wb", 0644);
        if (tclChannel == NULL) {
            Tcl_SetObjResult(tclInterp, Tcl_ObjPrintf(
                "couldn't create file \"%s\": %s", Tcl_GetString(file), Tcl_PosixError(tclInterp)));
            return;
        }
    }
    DEBUGLOG("Lib7ZipOutStream was open " << Tcl_GetString(file));
}

Lib7ZipOutStream::~Lib7ZipOutStream() {
    DEBUGLOG("~Lib7ZipOutStream");
    if (tclChannel && closechannel)
        Tcl_Close(tclInterp, tclChannel);
}

int Lib7ZipOutStream::Write(const void *data, unsigned int size, unsigned int *processedSize) {
    DEBUGLOG("Lib7ZipOutStream::Write " << size);
    if (tclChannel) {
        unsigned int wrote = Tcl_Write(tclChannel, (char *)data, size);
        if (wrote >= 0) {
            if (processedSize)
                *processedSize = wrote;
            return 0;
        }
    }
    return 1;
}

int Lib7ZipOutStream::Seek(__int64 offset, unsigned int seekOrigin, UInt64 *newPosition) {
    DEBUGLOG("Lib7ZipOutStream::Seek " << offset << " as " << seekOrigin);
    if (tclChannel) {
        Tcl_WideInt pos = Tcl_Seek(tclChannel, offset, seekOrigin);
        if (pos >= 0) {
            if (newPosition)
                *newPosition = pos;
            return 0;
        }
    }
    return 1;
}

int Lib7ZipOutStream::SetSize(UInt64 size) {
    Tcl_Panic("Lib7ZipOutStream::SetSize!");
    return -1;
}


Lib7ZipMultiVolumes::Lib7ZipMultiVolumes(Tcl_Interp *interp, Tcl_Obj *path, Tcl_Obj *type):
        tclInterp(interp), type(type), streams(), current(NULL) {
    DEBUGLOG("Lib7ZipMultiVolumes");
    if (type)
        Tcl_IncrRefCount(type);
    MoveToVolume((wchar_t *)Tcl_GetUnicode(path));
}

Lib7ZipMultiVolumes::~Lib7ZipMultiVolumes() {
    DEBUGLOG("~Lib7ZipMultiVolumes");
    if (type)
        Tcl_DecrRefCount(type);
    for (std::vector<Lib7ZipInStream *>::iterator i = streams.begin(); i != streams.end(); i++)
        delete *i;
}

std::wstring Lib7ZipMultiVolumes::GetFirstVolumeName() {
    DEBUGLOG("Lib7ZipMultiVolumes::GetFirstVolumeName");
    return streams.size() ? streams[0]->GetName() : L"";
};

bool Lib7ZipMultiVolumes::MoveToVolume(const wstring &volumeName) {
    current = NULL;
    for (std::vector<Lib7ZipInStream *>::iterator i = streams.begin(); i != streams.end(); i++) {
        if ((*i)->GetName() == volumeName) {
            DEBUGLOG("Lib7ZipMultiVolumes::MoveToVolume found " << *i);
            current = *i;
            break;
        }
    }
    if (current == NULL) {
        Tcl_Obj *filename = Tcl_NewUnicodeObj((Tcl_UniChar *)volumeName.c_str(), -1);
        Tcl_IncrRefCount(filename);
        current = new Lib7ZipInStream(tclInterp, filename, type, false);
        Tcl_DecrRefCount(filename);
        streams.push_back(current);
        DEBUGLOG("Lib7ZipMultiVolumes::MoveToVolume add " << current);
    }
    return current->Valid();
}

UInt64 Lib7ZipMultiVolumes::GetCurrentVolumeSize() {
    Tcl_Panic("Lib7ZipMultiVolumes::GetCurrentVolumeSize!");
    return -1;
};

C7ZipInStream *Lib7ZipMultiVolumes::OpenCurrentVolumeStream() {
    DEBUGLOG("Lib7ZipMultiVolumes::OpenCurrentVolumeStream " << current);
    return current && current->Valid() ? current : NULL;
};
