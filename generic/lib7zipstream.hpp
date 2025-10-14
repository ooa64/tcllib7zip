#ifndef LIB7ZIPSTREAM_H
#define LIB7ZIPSTREAM_H

#include <vector>
#include <lib7zip.h>
#include <tcl.h>

class Lib7ZipInStream:  public C7ZipInStream {

public:

    Lib7ZipInStream(Tcl_Interp *interp, Tcl_Obj *file, Tcl_Obj *type, bool usechannel);

    virtual ~Lib7ZipInStream();

    virtual std::wstring GetExt() const {return ext;};
    virtual int Read(void *data, unsigned int size, unsigned int *processedSize);
    virtual int Seek(__int64 offset, unsigned int seekOrigin, UInt64 *newPosition);
    virtual int GetSize(UInt64 *size);

    std::wstring GetName() {return name;};
    bool Valid() {return !!tclChannel;};

private:

    bool closechannel;
    std::wstring ext;
    std::wstring name;
    Tcl_Channel tclChannel;
    Tcl_Interp *tclInterp;
};

class Lib7ZipOutStream:  public C7ZipOutStream {

public:

    Lib7ZipOutStream(Tcl_Interp *interp, Tcl_Obj *file, bool usechannel);

    virtual ~Lib7ZipOutStream();

    virtual int Write(const void *data, unsigned int size, unsigned int *processedSize);
    virtual int Seek(__int64 offset, unsigned int seekOrigin, UInt64 *newPosition);
    virtual int SetSize(UInt64 size);

    bool Valid() {return !!tclChannel;};

private:

    bool closechannel;
    Tcl_Channel tclChannel;
    Tcl_Interp *tclInterp;
};

class Lib7ZipMultiVolumes:  public C7ZipMultiVolumes {

public:

    Lib7ZipMultiVolumes(Tcl_Interp *interp, Tcl_Obj *path, Tcl_Obj *type);

    virtual ~Lib7ZipMultiVolumes();

    virtual wstring GetFirstVolumeName();
    virtual bool MoveToVolume(const wstring &volumeName);
    virtual UInt64 GetCurrentVolumeSize();
    virtual C7ZipInStream *OpenCurrentVolumeStream();

    bool Valid() {return current && current->Valid();};

private:

    std::vector<Lib7ZipInStream *> streams;
    Lib7ZipInStream *current;
    Tcl_Obj *type;
    Tcl_Interp *tclInterp;
};

#endif