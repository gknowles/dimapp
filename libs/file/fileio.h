// Copyright Glen Knowles 2015 - 2024.
// Distributed under the Boost Software License, Version 1.0.
//
// file.h - dim file
#pragma once

#include "cppconf/cppconf.h"

#include "file/path.h"

#include "core/handle.h"
#include "core/task.h"
#include "core/time.h"
#include "core/types.h"

#include <cstdint>
#include <memory>
#include <string_view>
#include <system_error>

namespace Dim {

namespace File {
    enum class OpenMode : unsigned {
        // Content access, exactly one *must* be specified.
        fNoContent = 0x1, // When you only want metadata (time, size, etc).
        fReadOnly = 0x2,
        fReadWrite = 0x4,

        // Remove access, allows fileRemoveOnClose() to be used on file handle.
        fRemove = 0x1'0000,

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
        // and a multiple of page size as returned by filePageSize().
        fAligned = 0x2000,

        // Optimize for random or sequential access
        fRandom = 0x4000,
        fSequential = 0x8000,

        // Temporary file, will be deleted when closed, also a hint to file
        // caching.
        fTemp = 0x2'0000,

        // INTERNAL USE ONLY
        // Underlying native file handle is externally owned, and will be left
        // open when the file is closed.
        fNonOwning = 0x4'0000,

        fInternalFlags = fNonOwning,
    };

    enum class Type {
        kUnknown,   // bad handle, system error, or unknown file type
        kRegular,
        kCharacter,
    };
};

template<> struct is_enum_flags<File::OpenMode> : std::true_type {};


/****************************************************************************
*
*   Directory
*
***/

// Drive defaults to the current drive
std::error_code fileGetCurrentDir(Path * out, std::string_view drive = {});

// "path" is resolved relative to the current dir of the drive (which defaults
// to current drive) given in the path.
std::error_code fileSetCurrentDir(
    std::string_view path,
    Path * out = nullptr
);

// Resolve to an absolute path as if done by:
//  Path p{path}
//  p.resolve(fileGetCurrentDir(p.drive()))
std::error_code fileAbsolutePath(Path * out, std::string_view path);

// Returns errc::invalid_argument if file relative to root is not within the
// root path. This can happen if file breaks out via ".." or is an absolute
// path.
std::error_code fileChildPath(
    Path * out,
    const Path & root,
    std::string_view file,
    bool createDirIfNotExist
);


/****************************************************************************
*
*   Metadata
*
***/

//---------------------------------------------------------------------------
// With path
//---------------------------------------------------------------------------
std::error_code fileSize(uint64_t * out, std::string_view path);
std::error_code fileLastWriteTime(TimePoint * out, std::string_view path);
std::error_code fileExists(bool * out, std::string_view path);
std::error_code fileDirExists(bool * out, std::string_view path);
std::error_code fileReadOnly(bool * out, std::string_view path);
std::error_code fileReadOnly(std::string_view path, bool enable);

namespace File {
    enum class Attrs : uint32_t {
        fReadOnly   = 0x0001,   // FILE_ATTRIBUTE_READONLY
        fHidden     = 0x0002,   // FILE_ATTRIBUTE_HIDDEN
        fSystem     = 0x0004,   // FILE_ATTRIBUTE_SYSTEM
        fDir        = 0x0010,   // FILE_ATTRIBUTE_DIRECTORY
        fArchive    = 0x0020,   // FILE_ATTRIBUTE_ARCHIVE
        fDevice     = 0x0040,   // FILE_ATTRIBUTE_DEVICE
        fNormal     = 0x0080,   // FILE_ATTRIBUTE_NORMAL
        fTemp       = 0x0100,   // FILE_ATTRIBUTE_TEMPORARY
        fSparse     = 0x0200,   // FILE_ATTRIBUTE_SPARSE_FILE
        fReparse    = 0x0400,   // FILE_ATTRIBUTE_REPARSE_POINT
    };
}
template<> struct is_enum_flags<File::Attrs> : std::true_type {};

std::error_code fileAttrs(EnumFlags<File::Attrs> * out, std::string_view path);
std::error_code fileAttrs(std::string_view path, EnumFlags<File::Attrs> attrs);

std::error_code fileResize(std::string_view path, size_t size);

std::error_code fileRemove(std::string_view path, bool recurse = false);
std::error_code fileCreateDirs(std::string_view path);

std::error_code fileTempDir(Path * out);
std::error_code fileTempName(
    Path * out,
    std::string_view suffix = ".tmp"
);

namespace File::Access {
    enum class Right {
        kInvalid,
        kNone,
        kFull, // not normally needed, prefer using "Modify"
        kModify,
        kReadAndExecute,
        kReadOnly,
        kWriteOnly,
        kDelete,
    };
    enum class Inherit {
        kNone,
        kOnly, // inherited by children but doesn't apply to object
        kAll,
    };
}
std::error_code fileAddAccess(
    std::string_view path,
    std::string_view trustee, // name or Sid of account or group
    File::Access::Right allow,
    File::Access::Inherit inherit
);
std::error_code fileSetAccess(
    std::string_view path,
    std::string_view trustee, // name or Sid of account or group
    File::Access::Right allow,
    File::Access::Inherit inherit
);

//---------------------------------------------------------------------------
// With handle
//---------------------------------------------------------------------------
struct FileHandle : HandleBase {};

std::error_code fileOpen(
    FileHandle * out,
    std::string_view path,
    EnumFlags<File::OpenMode> modeFlags
);
//  EINVAL
std::error_code fileOpen(
    FileHandle * out,
    intptr_t osfhandle,
    EnumFlags<File::OpenMode> modeFlags
);

// The modeFlags are or'd with "fCreat | fExcl | fReadWrite" before the
// underlying call to fileOpen.
std::error_code fileCreateTemp(
    FileHandle * out,
    EnumFlags<File::OpenMode> modeFlags = {},
    std::string_view suffix = ".tmp"
);

// Create references to the standard in/out/err devices. The handle should be
// closed after use, which detaches from the underlying file descriptor.
// Accessing the same device from multiple handles will have unpredictable
// results.
std::error_code fileAttachStdin(FileHandle * out);
std::error_code fileAttachStdout(FileHandle * out);
std::error_code fileAttachStderr(FileHandle * out);

// Closing the file invalidates the handle and releases any internal resources
// associated with the file.
std::error_code fileClose(FileHandle f);

// Changes the files size by either growing or shrinking it. Returns false on
// errors. Requires write access and that there are no open views.
std::error_code fileResize(FileHandle f, size_t size);

// Must have delete access to file, either by opening it with fRemove or some
// other means.
std::error_code fileRemoveOnClose(FileHandle f, bool enable = true);

// Flushes pending writes from the file cache to disk.
std::error_code fileFlush(FileHandle f);
std::error_code fileFlushViews(FileHandle f);

// On error returns 0 and sets errno to a non-zero value. Otherwise the
// function returns the size and, if the size was 0, clears errno.
std::error_code fileSize(uint64_t * out, FileHandle f);

// On error returns TimePoint::min() and sets errno to a non-zero value. If
// the time was min(), errno is cleared.
std::error_code fileLastWriteTime(TimePoint * out, FileHandle f);

// On error returns empty and sets errno to a non-zero value.
std::string_view filePath(FileHandle f);

// Returns the open mode flags used to create the handle.
EnumFlags<File::OpenMode> fileMode(FileHandle f);

// kUnknown is returned for bad handle and system errors as well as when the
// type is unknown. If kUnknown was not returned due to error, errno is set
// to 0.
std::error_code fileType(File::Type * out, FileHandle f);

struct FileAlignment {
    // All values measured in bytes
    unsigned logicalSector;
    unsigned physicalSector;
};
std::error_code fileAlignment(FileAlignment * out, FileHandle f);


/****************************************************************************
*
*   Read
*
***/

struct FileReadData {
    std::string_view data;
    bool more;
    int64_t offset;
    FileHandle f;
    std::error_code ec;
};

class IFileReadNotify {
public:
    virtual ~IFileReadNotify() = default;

    // Returns false to prevent more reads, otherwise reads continue (with more
    // set to true) until the requested length has been received (at which
    // point more will be false). *bytesUsed must be set to how much data was
    // consumed, when it is less than data.size() the unused portion will be
    // included as part of the data in the subsequent call. *bytesUsed is zero
    // on entry and when returning true it must not be left that way.
    //
    // Guaranteed to be called at least once, on operation failure 'more' is
    // set to false and data.size() == 0.
    virtual bool onFileRead(size_t * bytesUsed, const FileReadData & data);
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
std::error_code fileReadWait(
    size_t * out,
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

// The notify will be called exactly once, when the load has finished, with
// bytesUsed set to nullptr.
void fileLoadBinary(
    IFileReadNotify * notify,
    std::string * out,
    std::string_view path,
    size_t maxSize = 10'000'000,
    TaskQueueHandle hq = {} // queue to notify
);
std::error_code fileLoadBinaryWait(
    std::string * out,
    std::string_view path,
    size_t maxSize = 10'000'000
);


/****************************************************************************
*
*   Write
*
***/

struct FileWriteData {
    int written;
    std::string_view data;
    int64_t offset;
    FileHandle f;
    std::error_code ec;
};

class IFileWriteNotify {
public:
    virtual ~IFileWriteNotify() = default;
    virtual void onFileWrite(const FileWriteData & data) = 0;
};

void fileWrite(
    IFileWriteNotify * notify,
    FileHandle f,
    int64_t offset,
    const void * buf,
    size_t bufLen,
    TaskQueueHandle hq = {} // queue to notify
);
void fileWrite(
    IFileWriteNotify * notify,
    FileHandle f,
    int64_t offset,
    std::string_view data,
    TaskQueueHandle hq = {} // queue to notify
);
std::error_code fileWriteWait(
    size_t * out,
    FileHandle f,
    int64_t offset,
    const void * buf,
    size_t bufLen
);
std::error_code fileWriteWait(
    size_t * out,
    FileHandle f,
    int64_t offset,
    std::string_view data
);

void fileAppend(
    IFileWriteNotify * notify,
    FileHandle f,
    const void * buf,
    size_t bufLen,
    TaskQueueHandle hq = {} // queue to notify
);
void fileAppend(
    IFileWriteNotify * notify,
    FileHandle f,
    std::string_view data,
    TaskQueueHandle hq = {} // queue to notify
);
std::error_code fileAppendWait(
    size_t * out,
    FileHandle f,
    const void * buf,
    size_t bufLen
);
std::error_code fileAppendWait(
    size_t * out,
    FileHandle f,
    std::string_view data
);

void fileSaveBinary(
    IFileWriteNotify * notify,
    std::string_view path,
    std::string_view data,
    TaskQueueHandle hq = {} // queue to notify
);
std::error_code fileSaveBinaryWait(
    std::string_view path,
    std::string_view data
);

void fileSaveTempFile(
    IFileWriteNotify * notify,
    std::string_view data,
    std::string_view suffix = ".tmp",
    TaskQueueHandle hq = {} // queue to notify
);


/****************************************************************************
*
*   FileAppendStream
*
***/

class FileAppendStream : Dim::IFileWriteNotify {
public:
    struct Impl;

public:
    FileAppendStream();
    FileAppendStream(int numBufs, int maxWrites, size_t pageSize);
    ~FileAppendStream();
    void init(int numBufs, int maxWrites, size_t pageSize);

    explicit operator bool() const;

    enum OpenExisting {
        kFail,
        kAppend,
        kTrunc,
    };
    bool open(std::string_view path, OpenExisting mode);
    bool attach(Dim::FileHandle f);
    void close();

    void append(std::string_view data);

private:
    void write_LK();

    void onFileWrite(const FileWriteData & data) override;

    std::unique_ptr<Impl> m_impl;
};


/****************************************************************************
*
*   Copy
*
***/

class IFileCopyNotify {
public:
    virtual ~IFileCopyNotify() = default;

    virtual bool onFileCopy(int64_t offset, int64_t copied, int64_t total) {
        return true;
    }
    // Guaranteed to be called exactly once, when the copy is completed
    // (or failed).
    virtual void onFileEnd(int64_t offset, int64_t total) = 0;
};

// !!! Never been used, never been tested (beyond knowing that it requires
//     exclusive file access to both the source and target... :P)!
void fileCopy(
    IFileCopyNotify * notify,
    std::string_view dstPath,
    std::string_view srcPath,
    TaskQueueHandle hq = {} // queue to notify
);


/****************************************************************************
*
*   Views
*
***/

namespace File {

enum class View {
    kReadOnly,
    kReadWrite,
};

} // namespace

// Offset to the start of a view must be a multiple of the view alignment, it
// is always a multiple of the page size.
size_t fileViewAlignment(FileHandle f);

// The maxLength is the maximum length into the file that view can be extended
// to cover. A value less than or equal to the length (such as 0) makes a view
// that can't be extended. The value is rounded up to a multiple of page size.
std::error_code fileOpenView(
    const char *& view,
    FileHandle f,
    File::View mode,    // must be kReadOnly
    int64_t offset = 0,
    int64_t length = 0,     // defaults to current length of file
    int64_t maxLength = 0   // defaults to not extendable
);
std::error_code fileOpenView(
    char *& view,
    FileHandle f,
    File::View mode,    // must be kReadWrite
    int64_t offset = 0,
    int64_t length = 0,     // defaults to current length of file
    int64_t maxLength = 0   // defaults to not extendable
);

std::error_code fileCloseView(FileHandle f, const void * view);

// Extend the view up to maxLen that was set when the view was opened. A view
// can only be extended if the file (which is also extended) was opened for
// writing. "Extending" with a length less than the current view has no effect
// and extending beyond maxLen is an error.
std::error_code fileExtendView(
    FileHandle f,
    const void * view,
    int64_t length
);

} // namespace
