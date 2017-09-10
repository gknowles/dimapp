// Copyright Glen Knowles 2015 - 2017.
// Distributed under the Boost Software License, Version 1.0.
//
// winfile.cpp - dim windows platform
#include "pch.h"
#pragma hdrstop

using namespace std;
using namespace Dim;


/****************************************************************************
*
*   Private declarations
*
***/

namespace {

struct WinFileInfo : public HandleContent {
    FileHandle m_f;
    string m_path;
    HANDLE m_handle{INVALID_HANDLE_VALUE};
    File::OpenMode m_mode{File::fReadOnly};
    const char * m_view{nullptr};
    size_t m_viewSize{0};
};

class IFileOpBase : public ITaskNotify {
public:
    void start(
        WinFileInfo * file, 
        void * buf, 
        size_t bufLen, 
        int64_t off, 
        int64_t len
    );
    void run();
    virtual void onNotify() = 0;
    virtual bool onRun() = 0;
    virtual bool async() = 0;
    virtual const char * logOnError() = 0;

    // ITaskNotify
    void onTask() override;

protected:
    WinOverlappedEvent m_iocpEvt;
    int64_t m_offset{0};
    int64_t m_length{0};
    bool m_started{false};
    bool m_trigger{false};

    WinFileInfo * m_file{nullptr};
    char * m_buf{nullptr};
    int m_bufLen{0};
    int m_bufUnused{0};

    WinError m_err{0};
    DWORD m_bytes{0};
};

} // namespace

//---------------------------------------------------------------------------
// Windows API declarations not available from Windows.h
//---------------------------------------------------------------------------
using NtCreateSectionFn = DWORD(WINAPI *)(
    HANDLE * hSec,
    ACCESS_MASK access,
    void * objAttrs,
    LARGE_INTEGER * maxSize,
    ULONG secProt,
    ULONG allocAttrs,
    HANDLE hFile
);
enum SECTION_INHERIT {
    ViewShare = 1,
    ViewUnmap = 2,
};
using NtMapViewOfSectionFn = DWORD(WINAPI *)(
    HANDLE hSec,
    HANDLE hProc,
    const void ** base,
    ULONG_PTR zeroBits,
    SIZE_T commitSize,
    LARGE_INTEGER * secOffset,
    SIZE_T * viewSize,
    SECTION_INHERIT ih,
    ULONG allocType,
    ULONG pageProt
);
using NtUnmapViewOfSectionFn = DWORD(WINAPI *)(
    HANDLE hProc, 
    const void * base
);
using NtCloseFn = DWORD(WINAPI *)(HANDLE h);


/****************************************************************************
*
*   Variables
*
***/

static NtCreateSectionFn s_NtCreateSection;
static NtMapViewOfSectionFn s_NtMapViewOfSection;
static NtUnmapViewOfSectionFn s_NtUnmapViewOfSection;
static NtCloseFn s_NtClose;

static size_t s_pageSize;
static TaskQueueHandle s_hq;

static shared_mutex s_fileMut;
static HandleMap<FileHandle, WinFileInfo> s_files;


/****************************************************************************
*
*   IFileOpBase
*
***/

//===========================================================================
void IFileOpBase::start(
    WinFileInfo * file,
    void * buf,
    size_t bufLen,
    int64_t off,
    int64_t len
) {
    assert(bufLen && bufLen <= (size_t) numeric_limits<int>::max());
    m_file = file;
    m_buf = (char *)buf;
    m_bufLen = (int)bufLen;
    m_bufUnused = 0;
    m_offset = off;
    m_length = len;

    m_iocpEvt.notify = this;
    m_iocpEvt.overlapped = {};

    if (async())
        return taskPush(s_hq, *this);

    m_iocpEvt.hq = s_hq;
    if (m_file->m_mode & File::fBlocking)
        return run();

    m_trigger = true;
    bool waiting = true;
    taskPush(s_hq, *this);
    while (m_trigger == waiting) {
        WaitOnAddress(&m_trigger, &waiting, sizeof(m_trigger), INFINITE);
    }
    if (m_err)
        winFileSetErrno(m_err);
}

//===========================================================================
void IFileOpBase::run() {
    m_started = true;
    winSetOverlapped(m_iocpEvt, m_offset);

    m_err = ERROR_SUCCESS;
    if (!onRun()) {
        m_err = WinError{};
        if (m_err == ERROR_IO_PENDING)
            return;
    }

    if (async()) {
        // async operation on blocking or non-blocking handle
        //
        // post to task queue, which is where the callback must come from,
        // then onTask() will:
        // set result, make callback, delete this
        taskPushEvent(*this);
    } else {
        // sync operation on blocking or non-blocking handle
        //
        // set result, signal event (for non-blocking), no callback
        onTask();
    }
}

//===========================================================================
void IFileOpBase::onTask() {
    if (!m_started)
        return run();

    if (m_err == ERROR_IO_PENDING) {
        m_err = 0;
        if (!GetOverlappedResult(NULL, &m_iocpEvt.overlapped, &m_bytes, false))
            m_err = WinError{};
    }

    if (m_err) {
        if (m_err == ERROR_OPERATION_ABORTED) {
            // explicitly canceled
        } else if (m_err == ERROR_HANDLE_EOF && !m_length) {
            // hit eof, when explicitly reading until the end
        } else if (auto log = logOnError()) {
            logMsgError() << log << '(' << m_file->m_path << "): " << m_err;
        }
        winFileSetErrno(m_err);
    }

    if (!async()) {
        if (m_trigger) {
            m_trigger = false;
            WakeByAddressSingle(&m_trigger);
        }
        return;
    }

    onNotify();
}


/****************************************************************************
*
*   FileReader
*
***/

namespace {

class FileReader : public IFileOpBase {
public:
    FileReader(IFileReadNotify * notify) : m_notify{notify} {}
    bool onRun() override;
    void onNotify() override;
    bool async() override { return m_notify != nullptr; }
    const char * logOnError() override { return "ReadFile"; }

private:
    IFileReadNotify * m_notify;
};

} // namespace

//===========================================================================
bool FileReader::onRun() {
    auto bufLen = m_bufLen - m_bufUnused;
    auto len = m_length;
    if (!len || len > bufLen)
        len = bufLen;

    return ReadFile(
        m_file->m_handle, 
        m_buf + m_bufUnused, 
        (DWORD)len, 
        &m_bytes, 
        &m_iocpEvt.overlapped
    );
}

//===========================================================================
void FileReader::onNotify() {
    auto avail = m_bytes + m_bufUnused;
    size_t bytesUsed = 0;
    bool again = m_bytes && m_notify->onFileRead(
        &bytesUsed,
        string_view(m_buf, avail), 
        m_offset, 
        m_file->m_f
    );
    m_offset += m_bytes;
    if (!again) {
        m_notify->onFileEnd(m_offset, m_file->m_f);
        delete this;
        return;
    }

    // Unless aborting (i.e. returned false) calls to onFileRead must consume
    // at least some bytes.
    assert(bytesUsed && bytesUsed <= avail);
    m_bufUnused = int(avail - bytesUsed);
    if (m_bufUnused)
        memmove(m_buf, m_buf + bytesUsed, m_bufUnused);

    if (m_length > m_bytes)
        m_length -= m_bytes;

    return run();
}


/****************************************************************************
*
*   FileWriter
*
***/

namespace {

class FileWriter : public IFileOpBase {
public:
    FileWriter(IFileWriteNotify * notify) : m_notify{notify} {}
    bool onRun() override;
    void onNotify() override;
    bool async() override { return m_notify != nullptr; }
    const char * logOnError() override { return "ReadFile"; }

private:
    IFileWriteNotify * m_notify;
};

} // namespace

//===========================================================================
bool FileWriter::onRun() {
    return WriteFile(
        m_file->m_handle, 
        m_buf, 
        m_bufLen, 
        &m_bytes, 
        &m_iocpEvt.overlapped
    );
}

//===========================================================================
void FileWriter::onNotify() {
    m_notify->onFileWrite(
        m_bytes, 
        string_view(m_buf, m_bufLen), 
        m_offset, 
        m_file->m_f
    );
    delete this;
}


/****************************************************************************
*
*   Internal API
*
***/

//===========================================================================
void Dim::iFileInitialize() {
    SYSTEM_INFO si;
    GetSystemInfo(&si);
    s_pageSize = si.dwPageSize;

    loadProc(s_NtCreateSection, "ntdll", "NtCreateSection");
    loadProc(s_NtMapViewOfSection, "ntdll", "NtMapViewOfSection");
    loadProc(s_NtUnmapViewOfSection, "ntdll", "NtUnmapViewOfSection");
    loadProc(s_NtClose, "ntdll", "NtClose");

    s_hq = taskCreateQueue("File IO", 2);

    winFileMonitorInitialize();
}

//===========================================================================
bool Dim::winFileSetErrno(int error) {
    _set_doserrno(error);

    switch (error) {
    case ERROR_ALREADY_EXISTS:
    case ERROR_FILE_EXISTS: _set_errno(EEXIST); break;
    case ERROR_FILE_NOT_FOUND: _set_errno(ENOENT); break;
    case ERROR_SHARING_VIOLATION: _set_errno(EBUSY); break;
    case ERROR_ACCESS_DENIED: _set_errno(EACCES); break;
    default: _set_errno(EIO); break;
    }

    return false;
}


/****************************************************************************
*
*   Public Handle API
*
***/

//===========================================================================
FileHandle Dim::fileOpen(string_view path, File::OpenMode mode) {
    using om = File::OpenMode;
    assert(~mode & om::fInternalFlags);

    auto file = make_unique<WinFileInfo>();
    file->m_mode = (om)mode;
    file->m_path = path;

    int access = 0;
    if (mode & om::fNoContent) {
        assert((~mode & om::fReadOnly) && (~mode & om::fReadWrite));
    } else if (mode & om::fReadOnly) {
        assert((~mode & om::fNoContent) && (~mode & om::fReadWrite));
        access = GENERIC_READ;
    } else {
        assert(mode & om::fReadWrite);
        assert((~mode & om::fNoContent) && (~mode & om::fReadOnly));
        access = GENERIC_READ | GENERIC_WRITE;
    }

    int share = 0;
    if (mode & om::fDenyWrite) {
        assert(~mode & om::fDenyNone);
        share |= FILE_SHARE_READ;
    } else if (mode & om::fDenyNone) {
        share |= FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE;
    }

    int creation = 0;
    if (mode & om::fCreat) {
        if (mode & om::fExcl) {
            assert(~mode & om::fTrunc);
            creation = CREATE_NEW;
        } else if (mode & om::fTrunc) {
            creation = CREATE_ALWAYS;
        } else {
            creation = OPEN_ALWAYS;
        }
    } else {
        assert(~mode & om::fExcl);
        if (mode & om::fTrunc) {
            creation = TRUNCATE_EXISTING;
        } else {
            creation = OPEN_EXISTING;
        }
    }

    int flagsAndAttrs = (mode & om::fBlocking) ? 0 : FILE_FLAG_OVERLAPPED;

    file->m_handle = CreateFileW(
        toWstring(file->m_path).c_str(),
        access,
        share,
        NULL, // security attributes
        creation,
        flagsAndAttrs,
        NULL // template file
    );
    if (file->m_handle == INVALID_HANDLE_VALUE) {
        winFileSetErrno(WinError{});
        return {};
    }

    if (~mode & om::fBlocking) {
        if (!winIocpBindHandle(file->m_handle)) {
            winFileSetErrno(WinError{});
            return {};
        }

        if (!SetFileCompletionNotificationModes(
            file->m_handle,
            FILE_SKIP_COMPLETION_PORT_ON_SUCCESS 
                | FILE_SKIP_SET_EVENT_ON_HANDLE
        )) {
            winFileSetErrno(WinError{});
            return {};
        }
    }

    unique_lock<shared_mutex> lk{s_fileMut};
    auto f = file->m_f = s_files.insert(file.release());
    return f;
}

//===========================================================================
static FileHandle attachStdHandle(
    int fd, 
    string_view path, 
    File::OpenMode mode // must be either fReadOnly or fReadWrite
) {
    assert(mode == File::fReadOnly || mode == File::fReadWrite);
    auto file = make_unique<WinFileInfo>();
    file->m_mode = mode | File::fBlocking;
    file->m_path = path;
    file->m_handle = GetStdHandle(fd);
    if (file->m_handle == NULL) {
        // process doesn't have a console or otherwise redirected stdin
        file->m_handle = INVALID_HANDLE_VALUE;
        winFileSetErrno(ERROR_FILE_NOT_FOUND);
        return {};
    } else if (file->m_handle == INVALID_HANDLE_VALUE) {
        winFileSetErrno(WinError{});
        return {};
    }
    auto proc = GetCurrentProcess();
    if (!DuplicateHandle(
        proc,   // source proc
        file->m_handle,
        proc,   // target proc
        &file->m_handle,
        0,      // access (ignored)
        FALSE,  // inheritable
        DUPLICATE_SAME_ACCESS
    )) {
        winFileSetErrno(WinError{});
        return {};
    }

    unique_lock<shared_mutex> lk{s_fileMut};
    auto f = file->m_f = s_files.insert(file.release());
    return f;
}

//===========================================================================
FileHandle Dim::fileAttachStdin() {
    return attachStdHandle(STD_INPUT_HANDLE, "STDIN", File::fReadOnly);
}

//===========================================================================
FileHandle Dim::fileAttachStdout() {
    return attachStdHandle(STD_OUTPUT_HANDLE, "STDOUT", File::fReadWrite);
}

//===========================================================================
FileHandle Dim::fileAttachStderr() {
    return attachStdHandle(STD_ERROR_HANDLE, "STDERR", File::fReadWrite);
}

//===========================================================================
static WinFileInfo * getInfo(FileHandle f, bool release = false) {
    if (release) {
        unique_lock<shared_mutex> lk{s_fileMut};
        return s_files.release(f);
    } else {
        shared_lock<shared_mutex> lk{s_fileMut};
        return s_files.find(f);
    }
}

//===========================================================================
void Dim::fileClose(FileHandle f) {
    auto file = static_cast<unique_ptr<WinFileInfo>>(getInfo(f, true));
    if (!file)
        return;
    if (file->m_handle != INVALID_HANDLE_VALUE) {
        if (file->m_view) {
            WinError err = (WinError::NtStatus) s_NtUnmapViewOfSection(
                GetCurrentProcess(), 
                file->m_view
            );
            if (err) {
                logMsgError() << "NtUnmapViewOfSection(" << file->m_path
                    << "): " << err;
            }
        }
        if (~file->m_mode & File::fNonOwning) {
            if (file->m_mode & File::fBlocking)
                CancelIoEx(file->m_handle, NULL);
            CloseHandle(file->m_handle);
        }
        file->m_handle = INVALID_HANDLE_VALUE;
    }
}

//===========================================================================
uint64_t Dim::fileSize(FileHandle f) {
    auto file = getInfo(f);
    if (!file)
        return 0;
    LARGE_INTEGER size;
    if (!GetFileSizeEx(file->m_handle, &size)) {
        WinError err;
        logMsgError() << "GetFileSizeEx(" << file->m_path << "): " << err;
        winFileSetErrno(err);
        return 0;
    }
    return size.QuadPart;
}

//===========================================================================
TimePoint Dim::fileLastWriteTime(FileHandle f) {
    auto file = getInfo(f);
    if (!file)
        return TimePoint::min();
    FILETIME ctime, atime, wtime;
    if (!GetFileTime(file->m_handle, &ctime, &atime, &wtime)) {
        WinError err;
        logMsgError() << "GetFileTime(" << file->m_path << "): " << err;
        winFileSetErrno(err);
        return TimePoint::min();
    }
    ULARGE_INTEGER q;
    q.LowPart = wtime.dwLowDateTime;
    q.HighPart = wtime.dwHighDateTime;
    return TimePoint(Duration(q.QuadPart));
}

//===========================================================================
string_view Dim::filePath(FileHandle f) {
    auto file = getInfo(f);
    return file->m_path;
}

//===========================================================================
unsigned Dim::fileMode(FileHandle f) {
    auto file = getInfo(f);
    return file->m_mode;
}

//===========================================================================
File::FileType Dim::fileType(FileHandle f) {
    auto file = getInfo(f);
    DWORD type = GetFileType(file->m_handle);
    switch (type) {
    case FILE_TYPE_CHAR: return File::kCharacter;
    case FILE_TYPE_DISK: return File::kRegular;
    case FILE_TYPE_UNKNOWN:
        winFileSetErrno(WinError{});
        return File::kUnknown;
    default:
        winFileSetErrno(0);
        return File::kUnknown;
    }
}

//===========================================================================
void Dim::fileRead(
    IFileReadNotify * notify,
    void * outBuf,
    size_t outBufLen,
    FileHandle f,
    int64_t off,
    int64_t len
) {
    assert(notify);
    auto file = getInfo(f);
    auto ptr = new FileReader(notify);
    ptr->start(file, outBuf, outBufLen, off, len);
}

//===========================================================================
void Dim::fileReadWait(
    void * outBuf,
    size_t outBufLen,
    FileHandle f,
    int64_t off
) {
    auto file = getInfo(f);
    FileReader op(nullptr);
    op.start(file, outBuf, outBufLen, off, outBufLen);
}

//===========================================================================
void Dim::fileWrite(
    IFileWriteNotify * notify,
    FileHandle f,
    int64_t off,
    const void * buf,
    size_t bufLen
) {
    assert(notify);
    auto file = getInfo(f);
    auto ptr = new FileWriter(notify);
    ptr->start(file, const_cast<void *>(buf), bufLen, off, bufLen);
}

//===========================================================================
void Dim::fileWriteWait(
    FileHandle f,
    int64_t off,
    const void * buf,
    size_t bufLen
) {
    auto file = getInfo(f);
    FileWriter op(nullptr);
    op.start(file, const_cast<void *>(buf), bufLen, off, bufLen);
}

//===========================================================================
void Dim::fileAppend(
    IFileWriteNotify * notify,
    FileHandle f,
    const void * buf,
    size_t bufLen
) {
    assert(notify);
    // file writes to offset 2^64-1 are interpreted as appends to the end
    // of the file.
    fileWrite(notify, f, 0xffff'ffff'ffff'ffff, buf, bufLen);
}

//===========================================================================
void Dim::fileAppendWait(FileHandle f, const void * buf, size_t bufLen) {
    fileWriteWait(f, 0xffff'ffff'ffff'ffff, buf, bufLen);
}


/****************************************************************************
*
*   Public View API
*
***/

//===========================================================================
size_t Dim::filePageSize() {
    return s_pageSize;
}

//===========================================================================
bool Dim::fileOpenView(
    const char *& base,
    FileHandle f,
    int64_t maxLen // maximum viewable offset, rounded up to page size
) {
    auto file = getInfo(f);
    if (file->m_view) {
        base = file->m_view;
        return true;
    }
    base = nullptr;
    WinError err{0};
    HANDLE sec;

    SIZE_T viewSize;
    ULONG access, secProt, allocType, pageProt;

    if (file->m_mode & File::fReadOnly) {
        // readonly view
        access = SECTION_MAP_READ;
        secProt = PAGE_READONLY;
        viewSize = 0;
        allocType = 0;
        pageProt = PAGE_READONLY;
    } else if (file->m_mode & File::fReadWrite) {
        // readonly but extendable
        access = SECTION_MAP_READ;
        secProt = PAGE_READWRITE;
        viewSize = maxLen;
        allocType = MEM_RESERVE;
        pageProt = PAGE_READONLY;
    } else {
        // fully writable and extendable view
        assert(0 && "fully writable view");
        access = SECTION_MAP_READ | SECTION_MAP_WRITE;
        secProt = PAGE_READWRITE;
        viewSize = maxLen;
        allocType = MEM_RESERVE;
        pageProt = PAGE_READWRITE;
    }
    
    err = (WinError::NtStatus) s_NtCreateSection(
        &sec,
        access,
        nullptr, // object attributes
        nullptr, // maximum size
        secProt,
        SEC_RESERVE,
        file->m_handle
    );
    if (err) {
        logMsgError() << "NtCreateSection(" << file->m_path << "): " << err;
        winFileSetErrno(err);
        return false;
    }

    err = (WinError::NtStatus) s_NtMapViewOfSection(
        sec,
        GetCurrentProcess(),
        (const void **)&base,
        0,       // zero bits
        0,       // commit size
        nullptr, // section offset
        &viewSize,
        ViewUnmap,
        allocType,
        pageProt
    );
    if (err) {
        logMsgError() << "NtMapViewOfSection(" << file->m_path << "): " << err;
        winFileSetErrno(err);
    }
    file->m_view = base;
    file->m_viewSize = viewSize;

    // always close the section whether the map view worked or not
    WinError tmp = (WinError::NtStatus) s_NtClose(sec);
    if (tmp) {
        logMsgError() << "NtClose(" << file->m_path << "): " << tmp;
        if (!err) {
            winFileSetErrno(tmp);
            err = tmp;
        }
    }

    return !err;
}

//===========================================================================
void Dim::fileExtendView(FileHandle f, int64_t length) {
    auto file = getInfo(f);
    void * ptr =
        VirtualAlloc((void *)file->m_view, length, MEM_COMMIT, PAGE_READONLY);
    if (!ptr) {
        logMsgCrash() << "VirtualAlloc(" << file->m_path
                      << "): " << WinError{};
    }
    if (ptr != file->m_view) {
        logMsgDebug() << "VirtualAlloc(" << file->m_path << "): " << ptr
                      << " (expected " << file->m_view << ")";
    }
}


/****************************************************************************
*
*   Public Overlapped API
*
***/

//===========================================================================
void Dim::winSetOverlapped(
    WinOverlappedEvent & evt,
    int64_t off,
    HANDLE event) {
    evt.overlapped = {};
    evt.overlapped.Offset = (DWORD)off;
    evt.overlapped.OffsetHigh = (DWORD)(off >> 32);
    if (event != INVALID_HANDLE_VALUE)
        evt.overlapped.hEvent = event;
}
