// Copyright Glen Knowles 2015 - 2017.
// Distributed under the Boost Software License, Version 1.0.
//
// file.h - dim app
#pragma once

#include "cppconf/cppconf.h"

#include "core/handle.h"
#include "core/time.h"

#include <cstdint>
#include <experimental/filesystem>
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

        // Optimize for file*Wait family of functions. Opens file without
        // FILE_FLAG_OVERLAPPED and does async by posting the requests
        // to a small taskqueue whose threads use blocking calls.
        fBlocking = 0x1000,
    };
};


/****************************************************************************
*
*   Metadata
*
***/

//===========================================================================
// With path
//===========================================================================
uint64_t fileSize(std::string_view path);
TimePoint fileLastWriteTime(std::string_view path);

//===========================================================================
// With handle
//===========================================================================
struct FileHandle : HandleBase {};

// on error returns an empty pointer and sets errno to one of:
//  EEXIST, ENOENT, EBUSY, EACCES, or EIO
FileHandle fileOpen(
    std::string_view path,
    File::OpenMode modeFlags
);
uint64_t fileSize(FileHandle f);
TimePoint fileLastWriteTime(FileHandle f);
std::experimental::filesystem::path filePath(FileHandle f);
unsigned fileMode(FileHandle f);

// Closing the file is normally handled as part of destroying the File
// object, but fileClose() can be used to release the file to the system
// when the File can't be deleted, such as in a callback.
//
// filePath still works, but most operations on a closed File will fail.
void fileClose(FileHandle f);


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
    virtual bool onFileRead(
        std::string_view data, 
        int64_t offset, 
        FileHandle f
    ) {
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
    int64_t length = 0 // 0 to read until the end
);
void fileReadWait(
    void * outBuf,
    size_t outBufLen,
    FileHandle f,
    int64_t offset);

void fileStreamBinary(
    IFileReadNotify * notify,
    std::string_view path,
    size_t blkSize);

void fileLoadBinary(
    IFileReadNotify * notify,   // only onFileEnd() is called
    std::string & out,
    std::string_view path,
    size_t maxSize = 10'000'000);
void fileLoadBinaryWait(
    std::string & out,
    std::string_view path,
    size_t maxSize = 10'000'000);

// page size is determined by the operating system but is always a power of 2
size_t filePageSize();

// The maxLen is the maximum offset into the file that view can be extended
// to cover. A value less than or equal to the size of the file (such as 0)
// makes a view of the entire file that can't be extended. The value is
// rounded up to a multiple of page size.
bool fileOpenView(const char *& base, FileHandle f, int64_t maxLen = 0);

// Extend the view up to maxLen that was set when the view was opened. A
// view can only be extended if the file (which is also extended) was opened
// for writing. "Extending" with a length less than the current view has
// no effect and extending beyond maxLen is an error.
void fileExtendView(FileHandle f, int64_t length);


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
        std::string_view data,
        int64_t offset,
        FileHandle f) = 0;
};
void fileWrite(
    IFileWriteNotify * notify,
    FileHandle f,
    int64_t offset,
    const void * buf,
    size_t bufLen);
void fileWriteWait(
    FileHandle f,
    int64_t offset,
    const void * buf,
    size_t bufLen);
void fileAppend(
    IFileWriteNotify * notify,
    FileHandle f,
    const void * buf,
    size_t bufLen);
void fileAppendWait(FileHandle f, const void * buf, size_t bufLen);


/****************************************************************************
*
*   Monitor
*
***/

class IFileChangeNotify {
public:
    virtual ~IFileChangeNotify () {}

    virtual void onFileChange(std::string_view fullpath) = 0;
};

struct FileMonitorHandle : HandleBase {};

// The notifier, if present, is always called with the fullpath of the root
// directory and a nullptr for the File*. This happens for every rename or 
// change to last write time of any file within it's scope.
bool fileMonitorDir(
    FileMonitorHandle * handle,
    std::string_view dir,
    bool recurse,
    IFileChangeNotify * notify = nullptr
);
void fileMonitorCloseWait(FileMonitorHandle dir);

// Absolute path to directory being monitored
std::string_view fileMonitorPath(FileMonitorHandle dir);

void fileMonitor(
    FileMonitorHandle dir, 
    std::string_view file,
    IFileChangeNotify * notify
);
void fileMonitorCloseWait(
    FileMonitorHandle dir,
    std::string_view file,
    IFileChangeNotify * notify
);

// Normalized path that would be monitored for file, relative to base 
// directory. False for invalid parameters (bad dir, file outside of dir, 
// etc.)
bool fileMonitorPath(
    std::string & out, 
    FileMonitorHandle dir, 
    std::string_view file
);

} // namespace
