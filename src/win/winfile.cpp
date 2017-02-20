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

class IFileOpBase : public ITaskNotify {
public:
    void
    start(File * file, void * buf, size_t bufLen, int64_t off, int64_t len);
    void run();
    virtual void onNotify() = 0;
    virtual bool onRun() = 0;
    virtual bool async() = 0;

    // ITaskNotify
    void onTask() override;

protected:
    WinOverlappedEvent m_iocpEvt;
    int64_t m_offset{0};
    int64_t m_length{0};
    bool m_started{false};
    bool m_trigger{false};

    File * m_file{nullptr};
    char * m_buf{nullptr};
    int m_bufLen{0};

    WinError m_err{0};
    DWORD m_bytes{0};
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
*   IFileOpBase
*
***/

//===========================================================================
void IFileOpBase::start(
    File * file,
    void * buf,
    size_t bufLen,
    int64_t off,
    int64_t len) {
    assert(bufLen && bufLen <= numeric_limits<int>::max());
    m_file = file;
    m_buf = (char *)buf;
    m_bufLen = (int)bufLen;
    m_offset = off;
    m_length = len;

    m_iocpEvt.notify = this;
    m_iocpEvt.overlapped = {};

    if (async())
        return taskPush(s_hq, *this);

    m_iocpEvt.hq = s_hq;
    if (m_file->m_mode & IFile::kBlocking)
        return run();

    m_trigger = true;
    bool waiting = true;
    taskPush(s_hq, *this);
    while (m_trigger == waiting) {
        WaitOnAddress(&m_trigger, &waiting, sizeof(m_trigger), INFINITE);
    }
    if (m_err)
        setErrno(m_err);
}

//===========================================================================
void IFileOpBase::run() {
    m_started = true;
    winSetOverlapped(m_iocpEvt, m_offset);

    m_err = ERROR_SUCCESS;
    if (!onRun()) {
        WinError err;
        if (err == ERROR_IO_PENDING)
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
        if (m_err != ERROR_OPERATION_ABORTED)
            logMsgError() << "ReadFile(" << m_file->m_path << "): " << m_err;
        setErrno(m_err);
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
    FileReader(IFileReadNotify * notify)
        : m_notify{notify} {}
    bool onRun() override;
    void onNotify() override;
    bool async() override { return m_notify != nullptr; }

private:
    IFileReadNotify * m_notify;
};
}

//===========================================================================
bool FileReader::onRun() {
    auto len = m_length;
    if (!len || len > m_bufLen)
        len = m_bufLen;

    return ReadFile(
        m_file->m_handle, m_buf, (DWORD)len, &m_bytes, &m_iocpEvt.overlapped);
}

//===========================================================================
void FileReader::onNotify() {
    if (m_bytes) {
        m_notify->onFileRead(m_buf, m_bytes, m_offset, m_file);
        m_offset += m_bytes;

        if (m_length > m_bytes)
            m_length -= m_bytes;

        return run();
    }

    m_notify->onFileEnd(m_offset, m_file);
    delete this;
}


/****************************************************************************
*
*   FileWriter
*
***/

namespace {
class FileWriter : public IFileOpBase {
public:
    FileWriter(IFileWriteNotify * notify)
        : m_notify{notify} {}
    bool onRun() override;
    void onNotify() override;
    bool async() override { return m_notify != nullptr; }

private:
    IFileWriteNotify * m_notify;
};
}

//===========================================================================
bool FileWriter::onRun() {
    return WriteFile(
        m_file->m_handle, m_buf, m_bufLen, &m_bytes, &m_iocpEvt.overlapped);
}

//===========================================================================
void FileWriter::onNotify() {
    m_notify->onFileWrite(m_bytes, m_buf, m_bufLen, m_offset, m_file);
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

    s_hq = taskCreateQueue("File IO", 2);
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

    int flags = (mode & om::kBlocking) ? 0 : FILE_FLAG_OVERLAPPED;

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

    if (~mode & om::kBlocking) {
        if (!winIocpBindHandle(file->m_handle)) {
            setErrno(WinError{});
            return nullptr;
        }

        if (!SetFileCompletionNotificationModes(
                file->m_handle,
                FILE_SKIP_COMPLETION_PORT_ON_SUCCESS
                    | FILE_SKIP_SET_EVENT_ON_HANDLE)) {
            setErrno(WinError{});
            return nullptr;
        }
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
    auto ptr = new FileReader(notify);
    ptr->start(file, outBuf, outBufLen, off, len);
}

//===========================================================================
void Dim::fileReadSync(
    void * outBuf,
    size_t outBufLen,
    IFile * ifile,
    int64_t off) {
    File * file = static_cast<File *>(ifile);
    FileReader op(nullptr);
    op.start(file, outBuf, outBufLen, off, outBufLen);
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
    auto ptr = new FileWriter(notify);
    ptr->start(file, const_cast<void *>(buf), bufLen, off, bufLen);
}

//===========================================================================
void Dim::fileWriteSync(
    IFile * ifile,
    int64_t off,
    const void * buf,
    size_t bufLen) {
    File * file = static_cast<File *>(ifile);
    FileWriter op(nullptr);
    op.start(file, const_cast<void *>(buf), bufLen, off, bufLen);
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