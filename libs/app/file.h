// file.h - dim core
#pragma once

#include "config/config.h"

#include "core/types.h"

#include <experimental/filesystem>
#include <memory>
#include <string>
#include <string_view>

namespace Dim {

class IFile {
public:
    enum OpenMode {
        kReadOnly = 0x01,
        kReadWrite = 0x02,
        kCreat = 0x04, // create if not exist
        kExcl = 0x08,  // fail if already exists
        kTrunc = 0x10, // truncate if already exists
        kDenyWrite = 0x20,
        kDenyNone = 0x40,

        // Optimize for file*Sync family of functions. Opens file without
        // FILE_FLAG_OVERLAPPED and does async by posting the requests
        // to a small taskqueue whos thread use blocking calls.
        kBlocking = 0x80,
    };

public:
    virtual ~IFile() {}
};

// on error returns an empty pointer and sets errno to one of:
//  EEXIST, ENOENT, EBUSY, EACCES, or EIO
std::unique_ptr<IFile> fileOpen(
    std::string_view path,
    unsigned modeFlags // IFile::OpenMode::*
    );
size_t fileSize(IFile * file);
TimePoint fileLastWriteTime(IFile * file);
std::experimental::filesystem::path filePath(IFile * file);
unsigned fileMode(IFile * file);

// Closing the file is normally handled as part of destroying the IFile
// object, but fileClose() can be used to release the file to the system
// when the IFile can't be deleted, such as in a callback.
//
// filePath still works, but most operations on a closed IFile will fail.
void fileClose(IFile * file);


/****************************************************************************
*
*   Read
*
***/

class IFileReadNotify {
public:
    virtual ~IFileReadNotify() {}

    // return false to prevent more reads, otherwise reads continue until the
    // requested length has been received.
    virtual bool
    onFileRead(char data[], int bytes, int64_t offset, IFile * file) {
        return true;
    }

    // guaranteed to be called exactly once, when the read is completed
    // (or failed)
    virtual void onFileEnd(int64_t offset, IFile * file) = 0;
};
void fileRead(
    IFileReadNotify * notify,
    void * outBuf,
    size_t outBufLen,
    IFile * file,
    int64_t offset = 0,
    int64_t length = 0 // 0 to read until the end
    );
void fileReadSync(
    void * outBuf,
    size_t outBufLen,
    IFile * file,
    int64_t offset);
void fileReadBinary(
    IFileReadNotify * notify,
    std::string & out,
    std::string_view path,
    size_t maxSize = 10'000'000);
void fileReadSyncBinary(
    std::string & out,
    std::string_view path,
    size_t maxSize = 10'000'000);

// page size is determined by the operating system but is always a power of 2
size_t filePageSize();

// The maxLen is the maximum offset into the file that view can be extended
// to cover. A value less than or equal to the size of the file (such as 0)
// makes a view of the entire file that can't be extended. The value is
// rounded up to a multiple of page size.
bool fileOpenView(const char *& base, IFile * file, int64_t maxLen = 0);

// Extend the view up to maxLen that was set when the view was opened. A
// view can only be extended if the file (which is also extended) was opened
// for writing. "Extending" with a length less than the current view has
// no effect and extending beyond maxLen is an error.
void fileExtendView(IFile * file, int64_t length);


/****************************************************************************
*
*   Write
*
***/

class IFileWriteNotify {
public:
    virtual ~IFileWriteNotify() {}

    virtual void onFileWrite(
        int written,
        const char data[],
        int bytes,
        int64_t offset,
        IFile * file) = 0;
};
void fileWrite(
    IFileWriteNotify * notify,
    IFile * file,
    int64_t offset,
    const void * buf,
    size_t bufLen);
void fileWriteSync(
    IFile * file,
    int64_t offset,
    const void * buf,
    size_t bufLen);
void fileAppend(
    IFileWriteNotify * notify,
    IFile * file,
    const void * buf,
    size_t bufLen);
void fileAppendSync(IFile * file, const void * buf, size_t bufLen);

} // namespace
