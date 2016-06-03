// file.h - dim core
#pragma once

#include "dim/config.h"

#include "dim/types.h"

#include <experimental/filesystem>
#include <memory>

namespace Dim {

class IFile {
public:
    enum OpenMode {
        kReadOnly   = 0x01,
        kReadWrite  = 0x02,
        kCreat      = 0x04, // create if not exist
        kExcl       = 0x08, // fail if already exists
        kTrunc      = 0x10, // truncate if already exists
        kDenyWrite  = 0x20,
        kDenyNone   = 0x40,
    };

public:
    virtual ~IFile () {}
};

bool fileOpen (
    std::unique_ptr<IFile> & file,
    const std::experimental::filesystem::path & path,
    IFile::OpenMode mode
);

class IFileReadNotify {
public:
    virtual ~IFileReadNotify () {}

    // return false to prevent more reads, otherwise reads continue until the
    // requested length has been received.
    virtual bool onFileRead (
    char data[],
    int bytes,
    int64_t offset,
    IFile * file
    ) = 0;

    virtual void onFileEnd (
    int64_t offset,
    IFile * file
    ) = 0;
};
void fileRead (
    IFileReadNotify * notify,
    void * outBuf,
    size_t outBufLen,
    IFile * file,
    int64_t offset = 0,
    int64_t length = 0     // 0 to read until the end
);

class IFileWriteNotify {
public:
    virtual ~IFileWriteNotify () {}

    virtual void onFileWrite (
    int written,
    const char data[],
    int bytes,
    int64_t offset,
    IFile * file
    ) = 0;
};
void fileWrite (
    IFile * file,
    const void * buf,
    size_t bufLen,
    int64_t offset = 0,
    IFileWriteNotify * notify = nullptr
);
void fileAppend (
    IFile * file,
    const void * buf,
    size_t bufLen,
    IFileWriteNotify * notify = nullptr
);

} // namespace
