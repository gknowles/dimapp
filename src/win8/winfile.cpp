// winfile.cpp - dim core - windows platform
#include "pch.h"
#pragma hdrstop

using namespace std;
using namespace std::experimental::filesystem;
using namespace Dim;


/****************************************************************************
*
*   Incomplete public types
*
***/


/****************************************************************************
*
*   Private declarations
*
***/

namespace {
class File : public IFile {
public:
    path m_path;
    HANDLE m_handle{INVALID_HANDLE_VALUE};
    OpenMode m_mode{kReadOnly};
    const char * m_view{nullptr};
    size_t m_viewSize{0};

    virtual ~File();
};
}

using NtCreateSectionFn = DWORD(WINAPI *)(
    HANDLE * hSec,
    ACCESS_MASK access,
    void * objAttrs,
    LARGE_INTEGER * maxSize,
    ULONG secProt,
    ULONG allocAttrs,
    HANDLE hFile);
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
    ULONG pageProt);
using NtUnmapViewOfSectionFn =
    DWORD(WINAPI *)(HANDLE hProc, const void * base);
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


/****************************************************************************
*
*   Helpers
*
***/

//===========================================================================
static bool setErrno(int error) {
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
*   FileReader
*
***/

namespace {
class FileReader : public ITaskNotify {
public:
    FileReader(
        IFileReadNotify * notify,
        File * file,
        void * outBuf,
        size_t outBufLen);
    void queue(int64_t off, int64_t len, bool sync);

    // ITaskNotify
    void onTask() override;

private:
    void read(int64_t off, int64_t len);
    void destroy();

    WinOverlappedEvent m_iocpEvt;
    int64_t m_offset{0};
    int64_t m_length{0};
    bool m_started{false};
    HANDLE m_event{INVALID_HANDLE_VALUE};

    IFileReadNotify * m_notify;
    File * m_file;
    char * m_outBuf;
    int m_outBufLen;
};
}

//===========================================================================
FileReader::FileReader(
    IFileReadNotify * notify,
    File * file,
    void * outBuf,
    size_t outBufLen)
    : m_file(file)
    , m_notify(notify)
    , m_outBuf((char *)outBuf)
    , m_outBufLen((int)outBufLen) {
    m_iocpEvt.notify = this;
    m_iocpEvt.overlapped = {};
    if (!m_notify)
        m_iocpEvt.hq = s_hq;
}

//===========================================================================
void FileReader::queue(int64_t off, int64_t len, bool sync) {
    m_offset = off;
    m_length = len;
    if (sync) {
        WinEvent evt;
        m_event = evt.nativeHandle();
        taskPush(s_hq, *this);
        evt.wait();
    } else {
        taskPush(s_hq, *this);
    }
}

//===========================================================================
void FileReader::read(int64_t off, int64_t len) {
    m_offset = off;
    m_length = len;
    m_iocpEvt.overlapped = {};
    m_iocpEvt.overlapped.Offset = (DWORD)off;
    m_iocpEvt.overlapped.OffsetHigh = (DWORD)(off >> 32);

    if (!len || len > m_outBufLen)
        len = m_outBufLen;

    if (!ReadFile(
            m_file->m_handle,
            m_outBuf,
            (DWORD)len,
            NULL,
            &m_iocpEvt.overlapped)) {
        WinError err;
        if (err == ERROR_IO_PENDING)
            return;
        logMsgError() << "ReadFile(" << m_file->m_path << "): " << err;
    }

    destroy();
}

//===========================================================================
void FileReader::destroy() {
    if (m_notify)
        m_notify->onFileEnd(m_offset, m_file);
    if (m_event == INVALID_HANDLE_VALUE) {
        delete this;
    } else {
        WinEvent evt(m_event);
        delete this;
        evt.signal();
        evt.release();
    }
}

//===========================================================================
void FileReader::onTask() {
    if (!m_started) {
        m_started = true;
        return read(m_offset, m_length);
    }

    DWORD bytes;
    if (!GetOverlappedResult(NULL, &m_iocpEvt.overlapped, &bytes, false)) {
        WinError err;
        if (err != ERROR_OPERATION_ABORTED)
            logMsgError() << "ReadFile(" << m_file->m_path << "): " << err;
        return destroy();
    }

    bool more = false;
    if (bytes) {
        if (m_notify)
            m_notify->onFileRead(m_outBuf, bytes, m_offset, m_file);
        m_offset += bytes;
        more = true;
    }

    if (!more || m_length && m_length <= bytes) {
        destroy();
    } else {
        read(m_offset, m_length ? m_length - bytes : 0);
    }
}


/****************************************************************************
*
*   FileWriteBuf
*
***/

namespace {
class FileWriteBuf : public ITaskNotify {
public:
    FileWriteBuf(
        IFileWriteNotify * notify,
        File * file,
        const void * buf,
        size_t bufLen);
    void queue(int64_t off, bool sync);

    // ITaskNotify
    void onTask() override;

private:
    void write(int64_t off);

    WinOverlappedEvent m_iocpEvt;
    int64_t m_offset{0};
    bool m_started{false};
    HANDLE m_event{INVALID_HANDLE_VALUE};

    IFileWriteNotify * m_notify;
    File * m_file;
    const char * m_buf;
    int m_bufLen;

    WinError m_err{0};
    DWORD m_written{0};
};
}

//===========================================================================
FileWriteBuf::FileWriteBuf(
    IFileWriteNotify * notify,
    File * file,
    const void * buf,
    size_t bufLen)
    : m_notify(notify)
    , m_file(file)
    , m_buf((const char *)buf)
    , m_bufLen((int)bufLen) {
    assert(bufLen <= numeric_limits<int>::max());
    m_iocpEvt.notify = this;
    m_iocpEvt.overlapped = {};
    if (!m_notify)
        m_iocpEvt.hq = s_hq;
}

//===========================================================================
void FileWriteBuf::queue(int64_t off, bool sync) {
    m_offset = off;
    if (sync) {
        WinEvent evt;
        m_event = evt.nativeHandle();
        taskPush(s_hq, *this);
        evt.wait();
    } else {
        taskPush(s_hq, *this);
    }
}

//===========================================================================
void FileWriteBuf::write(int64_t off) {
    m_offset = off;
    m_iocpEvt.overlapped = {};
    m_iocpEvt.overlapped.Offset = (DWORD)off;
    m_iocpEvt.overlapped.OffsetHigh = (DWORD)(off >> 32);
    if (m_event != INVALID_HANDLE_VALUE)
        m_iocpEvt.overlapped.hEvent = m_event;

    if (!WriteFile(
            m_file->m_handle,
            m_buf,
            m_bufLen,
            &m_written,
            &m_iocpEvt.overlapped)) {
        m_err = WinError{};
        if (m_err == ERROR_IO_PENDING)
            return;
        if (m_err) {
            logMsgError() << "WriteFile(" << m_file->m_path << "): " << m_err;
        }
        setErrno(m_err);
    }

    if (m_notify)
        m_notify->onFileWrite(m_written, m_buf, m_bufLen, m_offset, m_file);
    delete this;
}

//===========================================================================
void FileWriteBuf::onTask() {
    if (!m_started) {
        m_started = true;
        return write(m_offset);
    }

    DWORD bytes;
    bool result =
        !!GetOverlappedResult(NULL, &m_iocpEvt.overlapped, &bytes, false);
    m_written = bytes;

    if (!result) {
        m_err = WinError{};
        if (m_err != ERROR_OPERATION_ABORTED) {
            logMsgError() << "WriteFile(" << m_file->m_path << "): " << m_err;
        }
        setErrno(m_err);
    }

    if (m_notify)
        m_notify->onFileWrite(bytes, m_buf, m_bufLen, m_offset, m_file);
    delete this;
}


/****************************************************************************
*
*   File
*
***/

//===========================================================================
File::~File() {
    fileClose(this);
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

    s_hq = taskCreateQueue("File Sync", 2);
}


/****************************************************************************
*
*   Public API
*
***/

//===========================================================================
unique_ptr<IFile> Dim::fileOpen(const path & path, unsigned mode) {
    using om = IFile::OpenMode;

    auto file = make_unique<File>();
    file->m_mode = (om)mode;
    file->m_path = path;

    int access = 0;
    if (mode & om::kReadOnly) {
        assert(~mode & om::kReadWrite);
        access = GENERIC_READ;
    } else {
        assert(mode & om::kReadWrite);
        access = GENERIC_READ | GENERIC_WRITE;
    }

    int share = 0;
    if (mode & om::kDenyWrite) {
        assert(~mode & om::kDenyNone);
        share = FILE_SHARE_READ;
    } else if (mode & om::kDenyNone) {
        share = FILE_SHARE_READ | FILE_SHARE_WRITE;
    }

    int creation = 0;
    if (mode & om::kCreat) {
        if (mode & om::kExcl) {
            assert(~mode & om::kTrunc);
            creation = CREATE_NEW;
        } else if (mode & om::kTrunc) {
            creation = CREATE_ALWAYS;
        } else {
            creation = OPEN_ALWAYS;
        }
    } else {
        assert(~mode & om::kExcl);
        if (mode & om::kTrunc) {
            creation = TRUNCATE_EXISTING;
        } else {
            creation = OPEN_EXISTING;
        }
    }

    int flags = FILE_FLAG_OVERLAPPED;

    file->m_handle = CreateFileW(
        path.c_str(),
        access,
        share,
        NULL, // security attributes
        creation,
        flags,
        NULL // template file
        );
    if (file->m_handle == INVALID_HANDLE_VALUE) {
        setErrno(WinError{});
        return nullptr;
    }

    if (!winIocpBindHandle(file->m_handle)) {
        setErrno(WinError{});
        return nullptr;
    }

    if (!SetFileCompletionNotificationModes(file->m_handle,
        FILE_SKIP_COMPLETION_PORT_ON_SUCCESS
            | FILE_SKIP_SET_EVENT_ON_HANDLE)) {
        setErrno(WinError{});
        return nullptr;
    }

    return file;
}

//===========================================================================
void Dim::fileClose(IFile * ifile) {
    File * file = static_cast<File *>(ifile);
    if (file->m_handle != INVALID_HANDLE_VALUE) {
        if (file->m_view) {
            if (DWORD status = s_NtUnmapViewOfSection(
                    GetCurrentProcess(), file->m_view)) {
                WinError err{WinError::NtStatus(status)};
                logMsgError() << "NtUnmapViewOfSection(" << file->m_path
                              << "): " << err;
            }
        }
        CloseHandle(file->m_handle);
        file->m_handle = INVALID_HANDLE_VALUE;
    }
}

//===========================================================================
size_t Dim::fileSize(IFile * ifile) {
    File * file = static_cast<File *>(ifile);
    LARGE_INTEGER size;
    if (!GetFileSizeEx(file->m_handle, &size)) {
        WinError err;
        logMsgError() << "WriteFile(" << file->m_path << "): " << err;
        setErrno(err);
        return 0;
    }
    return size.QuadPart;
}

//===========================================================================
TimePoint Dim::fileLastWriteTime(IFile * ifile) {
    File * file = static_cast<File *>(ifile);
    uint64_t ctime, atime, wtime;
    if (!GetFileTime(
            file->m_handle,
            (FILETIME *)&ctime,
            (FILETIME *)&atime,
            (FILETIME *)&wtime)) {
        WinError err;
        logMsgError() << "GetFileTime(" << file->m_path << "): " << err;
        setErrno(err);
        return TimePoint::min();
    }
    return TimePoint(Duration(wtime));
}

//===========================================================================
std::experimental::filesystem::path Dim::filePath(IFile * ifile) {
    File * file = static_cast<File *>(ifile);
    return file->m_path;
}

//===========================================================================
void Dim::fileRead(
    IFileReadNotify * notify,
    void * outBuf,
    size_t outBufLen,
    IFile * ifile,
    int64_t off,
    int64_t len) {
    assert(notify);
    File * file = static_cast<File *>(ifile);
    auto ptr = new FileReader(notify, file, outBuf, outBufLen);
    ptr->queue(off, len, false);
}

//===========================================================================
void Dim::fileReadSync(
    void * outBuf,
    size_t outBufLen,
    IFile * ifile,
    int64_t off) {
    File * file = static_cast<File *>(ifile);
    auto ptr = new FileReader(nullptr, file, outBuf, outBufLen);
    ptr->queue(off, outBufLen, true);
}

//===========================================================================
void Dim::fileWrite(
    IFileWriteNotify * notify,
    IFile * ifile,
    int64_t off,
    const void * buf,
    size_t bufLen) {
    assert(notify);
    File * file = static_cast<File *>(ifile);
    auto ptr = new FileWriteBuf(notify, file, buf, bufLen);
    ptr->queue(off, false);
}

//===========================================================================
void Dim::fileWriteSync(
    IFile * ifile,
    int64_t off,
    const void * buf,
    size_t bufLen) {
    File * file = static_cast<File *>(ifile);
    auto ptr = new FileWriteBuf(nullptr, file, buf, bufLen);
    ptr->queue(off, true);
}

//===========================================================================
void Dim::fileAppend(
    IFileWriteNotify * notify,
    IFile * file,
    const void * buf,
    size_t bufLen) {
    assert(notify);
    // file writes to offset 2^64-1 are interpreted as appends to the end
    // of the file.
    fileWrite(notify, file, 0xffff'ffff'ffff'ffff, buf, bufLen);
}

//===========================================================================
void Dim::fileAppendSync(IFile * file, const void * buf, size_t bufLen) {
    fileWriteSync(file, 0xffff'ffff'ffff'ffff, buf, bufLen);
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
    IFile * ifile,
    int64_t maxLen // maximum viewable offset, rounded up to page size
    ) {
    File * file = static_cast<File *>(ifile);
    if (file->m_view) {
        base = file->m_view;
        return true;
    }
    base = nullptr;
    WinError err{0};
    HANDLE sec;
    DWORD status = s_NtCreateSection(
        &sec,
        SECTION_MAP_READ,
        nullptr, // object attributes
        nullptr, // maximum size
        (file->m_mode & IFile::kReadOnly) ? PAGE_READONLY : PAGE_READWRITE,
        SEC_RESERVE,
        file->m_handle);
    if (status) {
        err = (WinError::NtStatus)status;
        logMsgError() << "NtCreateSection(" << file->m_path << "): " << err;
        setErrno(err);
        return false;
    }

    SIZE_T viewSize = (file->m_mode & IFile::kReadOnly) ? 0 : maxLen;
    status = s_NtMapViewOfSection(
        sec,
        GetCurrentProcess(),
        (const void **)&base,
        0,       // zero bits
        0,       // commit size
        nullptr, // section offset
        &viewSize,
        ViewUnmap,
        (file->m_mode & IFile::kReadOnly) ? 0 : MEM_RESERVE,
        PAGE_READONLY);
    if (status) {
        err = (WinError::NtStatus)status;
        logMsgError() << "NtMapViewOfSection(" << file->m_path << "): " << err;
        setErrno(err);
    }
    file->m_view = base;
    file->m_viewSize = viewSize;

    // always close the section whether the map view worked or not
    status = s_NtClose(sec);
    if (status) {
        WinError tmp{(WinError::NtStatus)status};
        logMsgError() << "NtClose(" << file->m_path << "): " << tmp;
        if (!err) {
            setErrno(tmp);
            err = tmp;
        }
    }

    return !err;
}

//===========================================================================
void Dim::fileExtendView(IFile * ifile, int64_t length) {
    File * file = static_cast<File *>(ifile);
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
