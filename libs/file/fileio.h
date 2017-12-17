// Copyright Glen Knowles 2015 - 2017.
// Distributed under the Boost Software License, Version 1.0.
//
// fileio.h - dim file
#pragma once

#include "cppconf/cppconf.h"

#include "core/handle.h"
#include "core/task.h"
#include "core/time.h"

#include <cstdint>
#include <memory>
#include <string>
#include <string_view>

namespace Dim {

namespace File {
    enum OpenMode : unsigned {
        // content access, one *must* be specified
        fNoContent = 0x1, // when you only want metadata (time, size, etc)
        fReadOnly = 0x2,
        fReadWrite = 0x4,

        // creation
        fCreat = 0x10, // create if not exist
        fExcl = 0x20,  // fail if already exists
        fTrunc = 0x40, // truncate if already exists

        // sharing, mutually exclusive, not more than one can be set
        fDenyWrite = 0x100, // others can read
        fDenyNone = 0x200,  // others can read, write, or delete

        // Optimize for the file*Wait family of functions. Opens file without
        // FILE_FLAG_OVERLAPPED and does asynchronous by posting the requests
        // to a small task queue whose threads use blocking calls.
        fBlocking = 0x1000,

        // Unbuffered but all application buffers must be system page aligned
        // and a multiple of page size.
        fAligned = 0x2000,

        // Optimize for random or sequential access
        fRandom = 0x4000,
        fSequential = 0x8000,

        // INTERNAL USE ONLY
        // Underlying native file handle is externally owned, and will be
        // left open when the file is closed.
        fNonOwning = 0x10000,

        fInternalFlags = fNonOwning,
    };

    enum FileType {
        kUnknown,   // bad handle, system error, or unknown file type
        kRegular,
        kCharacter,
    };
};


/****************************************************************************
*
*   Metadata
*
***/

//---------------------------------------------------------------------------
// With path
//---------------------------------------------------------------------------
uint64_t fileSize(std::string_view path);
TimePoint fileLastWriteTime(std::string_view path);
bool fileRemove(std::string_view path);

//---------------------------------------------------------------------------
// With handle
//---------------------------------------------------------------------------
struct FileHandle : HandleBase {};

// on error returns an empty pointer and sets errno to one of:
//  EEXIST, ENOENT, EBUSY, EACCES, or EIO
FileHandle fileOpen(
    std::string_view path,
    File::OpenMode modeFlags
);

// Create references to the standard in/out/err devices. The handle
// should be closed after use, which detaches from the underlying file
// descriptor. Accessing the same device from multiple handles will have
// unpredictable results.
FileHandle fileAttachStdin();
FileHandle fileAttachStdout();
FileHandle fileAttachStderr();

// Closing the file invalidates the handle and releases any internal resources
// associated with the file.
void fileClose(FileHandle f);

// Changes the files size by either growing or shrinking it. Returns false on
// errors.
bool fileResize(FileHandle f, size_t size);

// Flushes pending writes from the file cache to disk.
bool fileFlush(FileHandle f);
bool fileFlushViews(FileHandle f);

// On error returns 0 and sets errno to a non-zero value. Otherwise the
// function returns the size and, if the size was 0, clears errno.
uint64_t fileSize(FileHandle f);

// On error returns TimePoint::min() and sets errno to a non-zero value. If
// the time was min(), errno is cleared.
TimePoint fileLastWriteTime(FileHandle f);

// On error returns empty and sets errno to a non-zero value.
std::string_view filePath(FileHandle f);

unsigned fileMode(FileHandle f);

// kUnknown is returned for bad handle and system errors as well as when
// the type is unknown. If kUnknown was not returned due to error, errno will
// set to 0.
File::FileType fileType(FileHandle f);


/****************************************************************************
*
*   Read
*
***/

class IFileReadNotify {
public:
    virtual ~IFileReadNotify() = default;

    // return false to prevent more reads, otherwise reads continue until the
    // requested length has been received. *bytesUsed must be set to how much
    // data was consumed, when it is less than data.size() the unused portion
    // will be included as part of the data in the subsequent call. bytesUsed
    // is initialized to zero and when returning true it must not be left that
    // way.
    virtual bool onFileRead(
        size_t * bytesUsed,
        std::string_view data,
        int64_t offset,
        FileHandle f
    ) {
        *bytesUsed = data.size();
        return true;
    }

    // guaranteed to be called exactly once, when the read is completed
    // (or failed)
    virtual void onFileEnd(int64_t offset, FileHandle f) = 0;
};

void fileRead(
    IFileReadNotify * notify,
    void * outBuf,
    size_t outBufLen,
    FileHandle f,
    int64_t offset = 0,
    int64_t length = 0, // 0 to read until the end
    TaskQueueHandle hq = {} // queue to notify
);
size_t fileReadWait(
    void * outBuf,
    size_t outBufLen,
    FileHandle f,
    int64_t offset
);

void fileStreamBinary(
    IFileReadNotify * notify,
    std::string_view path,
    size_t blkSize,
    TaskQueueHandle hq = {} // queue to notify
);

void fileLoadBinary(
    IFileReadNotify * notify,   // only onFileEnd() is called
    std::string & out,
    std::string_view path,
    size_t maxSize = 10'000'000,
    TaskQueueHandle hq = {} // queue to notify
);
void fileLoadBinaryWait(
    std::string & out,
    std::string_view path,
    size_t maxSize = 10'000'000
);


/****************************************************************************
*
*   Write
*
***/

class IFileWriteNotify {
public:
    virtual ~IFileWriteNotify() = default;

    virtual void onFileWrite(
        int written,
        std::string_view data,
        int64_t offset,
        FileHandle f
    ) = 0;
};

void fileWrite(
    IFileWriteNotify * notify,
    FileHandle f,
    int64_t offset,
    const void * buf,
    size_t bufLen,
    TaskQueueHandle hq = {} // queue to notify

);
size_t fileWriteWait(
    FileHandle f,
    int64_t offset,
    const void * buf,
    size_t bufLen
);
void fileAppend(
    IFileWriteNotify * notify,
    FileHandle f,
    const void * buf,
    size_t bufLen,
    TaskQueueHandle hq = {} // queue to notify

);
size_t fileAppendWait(FileHandle f, const void * buf, size_t bufLen);


/****************************************************************************
*
*   Views
*
***/

namespace File {

enum ViewMode {
    kViewReadOnly,
    kViewReadWrite,
};

} // namespace

// page size is determined by the operating system but is always a power of 2
size_t filePageSize();

// Offset to the start of a view must be a multiple of the view alignment, it
// is always a multiple of the page size.
size_t fileViewAlignment();

// The maxLength is the maximum length into the file that view can be extended
// to cover. A value less than or equal to the length (such as 0) makes a view
// that can't be extended. The value is rounded up to a multiple of page size.
bool fileOpenView(
    const char *& view,
    FileHandle f,
    File::ViewMode mode,    // must be kViewReadOnly
    int64_t offset = 0,
    int64_t length = 0,     // defaults to current length of file
    int64_t maxLength = 0   // defaults to not extendable
);
bool fileOpenView(
    char *& view,
    FileHandle f,
    File::ViewMode mode,    // must be kViewReadWrite
    int64_t offset = 0,
    int64_t length = 0,     // defaults to current length of file
    int64_t maxLength = 0   // defaults to not extendable
);

void fileCloseView(FileHandle f, const void * view);

// Extend the view up to maxLen that was set when the view was opened. A
// view can only be extended if the file (which is also extended) was opened
// for writing. "Extending" with a length less than the current view has
// no effect and extending beyond maxLen is an error.
void fileExtendView(FileHandle f, const void * view, int64_t length);

} // namespace
