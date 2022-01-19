// Copyright Glen Knowles 2015 - 2022.
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

const int64_t kNpos = -1;

struct WinFileInfo : public HandleContent {
    FileHandle m_f;
    string m_path;
    HANDLE m_handle{INVALID_HANDLE_VALUE};
    EnumFlags<File::OpenMode> m_mode{File::fReadOnly};
    unordered_map<const void *, File::View> m_views;
};

class IFileOpBase : protected IWinOverlappedNotify {
public:
    IFileOpBase(TaskQueueHandle hq);

    size_t start(
        WinFileInfo * file,
        void * buf,
        size_t bufLen,
        int64_t off,
        int64_t len
    );

protected:
    void run();

    WinFileInfo * m_file{};
    char * m_buf{};
    int m_bufLen{};
    int m_bufUnused{};
    int64_t m_offset{};
    int64_t m_length{};
    DWORD m_bytes{};

private:
    virtual void onNotify() = 0;
    virtual bool onRun() = 0;
    virtual bool asyncOp() = 0;
    virtual const char * logOnError() = 0;

    // IWinOverlappedNotify
    void onTask() override;

    enum State { kUnstarted, kRunning };
    State m_state{kUnstarted};
    bool m_trigger{false};

    WinError m_err{0};
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

static TaskQueueHandle s_hq;

static shared_mutex s_fileMut;
static HandleMap<FileHandle, WinFileInfo> s_files;


/****************************************************************************
*
*   IFileOpBase
*
*   Completions only apply to non-blocking handles and are posted to a queue
*   depending on the type of request, so completions go:
*   Async:  to the selected (usually event) thread and makes the callback
*           from there.
*   Sync:   to an IO thread which then signals the waiting thread, which might
*           be the event thread.
*
***/

//===========================================================================
IFileOpBase::IFileOpBase(TaskQueueHandle hq)
    : IWinOverlappedNotify{hq}
{}

//===========================================================================
size_t IFileOpBase::start(
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

    overlapped() = {};

    if (asyncOp()) {
        taskPush(s_hq, this);
        return m_bytes;
    }

    if (m_file->m_mode.any(File::fBlocking)) {
        run();
        return m_bytes;
    }

    m_trigger = true;
    bool waiting = true;
    taskPush(s_hq, this);
    this_thread::yield();
    while (m_trigger == waiting) {
        WaitOnAddress(&m_trigger, &waiting, sizeof m_trigger, INFINITE);
    }
    if (m_err)
        winFileSetErrno(m_err);
    return m_bytes;
}

//===========================================================================
void IFileOpBase::run() {
    // Thread this is running on:
    // sync + blocking - request thread
    // sync + non-blocking - IO thread, request thread waiting
    // async + blocking - IO thread
    // async + non-blocking - IO thread
    m_state = kRunning;
    winSetOverlapped(overlapped(), m_offset);

    // Set m_err to pending now, because if onRun() launches the operation
    // asynchronously there's no chance to change it. If it's not async, m_err
    // will be set to something appropriate after onRun() returns.
    m_err = ERROR_IO_PENDING;
    if (onRun()) {
        m_err = ERROR_SUCCESS;
    } else {
        WinError err;
        if (err == ERROR_IO_PENDING)
            return;

        // Only now that we know "this" is not being processed (and perhaps
        // already deleted!) asynchronously is it safe to modify it.
        m_err = err;
    }

    // Operation completed synchronously
    if (asyncOp()) {
        // async operation on blocking or non-blocking handle
        //
        // post to task queue, which is where the callback must come from,
        // then onTask() will:
        //   set result, make callback, delete this
        pushOverlappedTask();
    } else {
        // sync operation on blocking or non-blocking handle
        //
        // set result, signal event (for non-blocking handle), no callback
        onTask();
    }
}

//===========================================================================
void IFileOpBase::onTask() {
    if (m_state == kUnstarted)
        return run();

    //-----------------------------------------------------------------------
    // IO complete, report result
    //
    // threadpool this thread is in:
    // sync + blocking - request thread
    // sync + non-blocking - IO thread, request thread waiting
    // async + blocking - event thread
    // async + non-blocking - event thread

    if (m_err == ERROR_IO_PENDING) {
        auto [err, bytes] = decodeOverlappedResult();
        m_err = err;
        m_bytes = bytes;
    }

    if (m_err) {
        if (m_err == ERROR_OPERATION_ABORTED) {
            // explicitly canceled
        } else if (m_err == ERROR_HANDLE_EOF && m_length == kNpos) {
            // hit EOF, expected when explicitly reading until the end
        } else if (auto log = logOnError()) {
            logMsgError() << log << '('
                << (m_file ? m_file->m_path : "(null)")
                << "): " << m_err;
        }
    }

    if (!asyncOp()) {
        if (m_trigger) {
            m_trigger = false;
            WakeByAddressSingle(&m_trigger);
        }
        return;
    }

    winFileSetErrno(m_err);
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
    explicit FileReader(IFileReadNotify * notify, TaskQueueHandle hq);
    bool onRun() override;
    void onNotify() override;
    bool asyncOp() override { return m_notify != nullptr; }
    const char * logOnError() override { return "ReadFile"; }

private:
    IFileReadNotify * m_notify;
};

} // namespace

//===========================================================================
FileReader::FileReader(IFileReadNotify * notify, TaskQueueHandle hq)
    : IFileOpBase{hq}
    , m_notify{notify}
{}

//===========================================================================
bool FileReader::onRun() {
    auto reqLen = (int64_t) m_bufLen - m_bufUnused;
    if (m_length != kNpos && reqLen > m_length)
        reqLen = m_length;

    return ReadFile(
        m_file->m_handle,
        m_buf + m_bufUnused,
        (DWORD)reqLen,
        &m_bytes,
        &overlapped()
    );
}

//===========================================================================
void FileReader::onNotify() {
    bool more = true;
    assert(m_length == kNpos || m_length >= m_bytes);
    auto reqLen = (int64_t) m_bufLen - m_bufUnused;
    if (m_length != kNpos) {
        if (reqLen > m_length)
            reqLen = m_length;
        m_length -= m_bytes;
    }
    more = m_length && m_bytes == reqLen;
    auto avail = m_bytes + m_bufUnused;
    size_t bytesUsed = 0;
    if (!m_notify->onFileRead(
        &bytesUsed,
        string_view(m_buf, avail),
        more,
        m_offset,
        m_file->m_f
    )) {
        more = false;
    }
    m_offset += m_bytes;
    if (!more) {
        delete this;
        return;
    }

    // Unless aborting (i.e. return false) calls to onFileRead must consume
    // at least some bytes.
    assert(bytesUsed && bytesUsed <= avail);
    m_bufUnused = int(avail - bytesUsed);
    if (m_bufUnused)
        memmove(m_buf, m_buf + bytesUsed, m_bufUnused);

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
    explicit FileWriter(IFileWriteNotify * notify, TaskQueueHandle hq);
    bool onRun() override;
    void onNotify() override;
    bool asyncOp() override { return m_notify != nullptr; }
    const char * logOnError() override { return "WriteFile"; }

private:
    IFileWriteNotify * m_notify;
};

} // namespace

//===========================================================================
FileWriter::FileWriter(IFileWriteNotify * notify, TaskQueueHandle hq)
    : IFileOpBase{hq}
    , m_notify{notify}
{}

//===========================================================================
bool FileWriter::onRun() {
    return WriteFile(
        m_file->m_handle,
        m_buf,
        m_bufLen,
        &m_bytes,
        &overlapped()
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
    winLoadProc(&s_NtCreateSection, "ntdll", "NtCreateSection");
    winLoadProc(&s_NtMapViewOfSection, "ntdll", "NtMapViewOfSection");
    winLoadProc(&s_NtUnmapViewOfSection, "ntdll", "NtUnmapViewOfSection");
    winLoadProc(&s_NtClose, "ntdll", "NtClose");

    s_hq = taskCreateQueue("File IO", 2);

    winFileMonitorInitialize();
}

//===========================================================================
bool Dim::winFileSetErrno(int error) {
    _set_doserrno(error);

    switch (error) {
    case NO_ERROR: errno = 0; break;
    case ERROR_ALREADY_EXISTS:
    case ERROR_FILE_EXISTS: errno = EEXIST; break;
    case ERROR_FILE_NOT_FOUND:
    case ERROR_PATH_NOT_FOUND: errno = ENOENT; break;
    case ERROR_SHARING_VIOLATION: errno = EBUSY; break;
    case ERROR_ACCESS_DENIED: errno = EACCES; break;
    case ERROR_INVALID_PARAMETER: errno = EINVAL; break;
    default: errno = EIO; break;
    }

    return !error;
}


/****************************************************************************
*
*   Public Path API
*
***/

//===========================================================================
TimePoint Dim::fileLastWriteTime(string_view path) {
    WIN32_FILE_ATTRIBUTE_DATA attrs;
    if (!GetFileAttributesExW(
        toWstring(path).c_str(),
        GetFileExInfoStandard,
        &attrs
    )) {
        return {};
    }
    return TimePoint{duration(attrs.ftLastWriteTime)};
}

//===========================================================================
EnumFlags<File::Attrs> Dim::fileAttrs(std::string_view path) {
    WIN32_FILE_ATTRIBUTE_DATA attrs;
    if (!GetFileAttributesExW(
        toWstring(path).c_str(),
        GetFileExInfoStandard,
        &attrs
    )) {
        return {};
    }
    return static_cast<File::Attrs>(attrs.dwFileAttributes);
}

//===========================================================================
bool Dim::fileAttrs(std::string_view path, EnumFlags<File::Attrs> attrs) {
    if (!SetFileAttributesW(
        toWstring(path).c_str(), 
        (DWORD) attrs.underlying()
    )) {
        winFileSetErrno(WinError{});
        return false;
    }
    return true;
}

//===========================================================================
string Dim::fileTempDir() {
    wstring tmpDir(MAX_PATH, '\0');
    for (;;) {
        auto bufLen = size(tmpDir);
        DWORD count = GetTempPathW((DWORD) bufLen, tmpDir.data());
        if (!count)
            logMsgFatal() << "GetTempPathW: " << WinError{};
        tmpDir.resize(count);
        if (count <= bufLen)
            break;
    }
    auto path = Path(toString(tmpDir));
    if (!fileCreateDirs(path)) {
        winFileSetErrno(WinError{});
        return {};
    }
    return path.str();
}

//===========================================================================
string Dim::fileTempName(string_view suffix) {
    auto path = Path(fileTempDir());
    if (!path)
        return {};

    auto fname = toString(newGuid());
    fname += suffix;
    path /= fname;
    return path.str();
}


/****************************************************************************
*
*   Public Handle API
*
***/

//===========================================================================
static FileHandle allocHandle(
    HANDLE handle,
    string_view path,
    EnumFlags<File::OpenMode> mode
) {
    assert(handle && handle != INVALID_HANDLE_VALUE);

    auto file = make_unique<WinFileInfo>();
    file->m_handle = handle;
    file->m_mode = mode;
    file->m_path = path;

    if (mode.none(File::fBlocking)) {
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

    unique_lock lk{s_fileMut};
    auto f = s_files.insert(file.get());
    file.release()->m_f = f;
    return f;
}

//===========================================================================
FileHandle Dim::fileOpen(string_view path, EnumFlags<File::OpenMode> mode) {
    using enum File::OpenMode;
    assert(mode.none(fInternalFlags));
    // There must be exactly one IO mode.
    assert(mode.count(fNoContent | fReadOnly | fReadWrite) == 1);

    int access = 0;
    if (mode.any(fNoContent)) {
        access = 0;
    } else if (mode.any(fReadOnly)) {
        access = GENERIC_READ;
    } else {
        assert(mode.any(fReadWrite));
        assert(mode.none(fNoContent | fReadOnly));
        access = GENERIC_READ | GENERIC_WRITE;
    }

    int share = 0;
    if (mode.any(fDenyWrite)) {
        assert(mode.none(fDenyNone));
        share |= FILE_SHARE_READ;
    } else if (mode.any(fDenyNone)) {
        share |= FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE;
    }

    int creation = 0;
    if (mode.any(fCreat)) {
        if (mode.any(fExcl)) {
            assert(mode.none(fTrunc));
            creation = CREATE_NEW;
        } else if (mode.any(fTrunc)) {
            creation = CREATE_ALWAYS;
        } else {
            creation = OPEN_ALWAYS;
        }
    } else {
        assert(mode.none(fExcl));
        if (mode.any(fTrunc)) {
            creation = TRUNCATE_EXISTING;
        } else {
            creation = OPEN_EXISTING;
        }
    }

    // Unless impersonation is explicitly desired it should be restricted by
    // including the SECURITY_SQOS_PRESENT and SECURITY_IDENTIFICATION access
    // flags when a client opens a named pipe.
    int flagsAndAttrs = 0;
    if (mode.none(fBlocking))
        flagsAndAttrs |= FILE_FLAG_OVERLAPPED;
    if (mode.any(fAligned))
        flagsAndAttrs |= FILE_FLAG_NO_BUFFERING;
    if (mode.any(fRandom))
        flagsAndAttrs |= FILE_FLAG_RANDOM_ACCESS;
    if (mode.any(fSequential))
        flagsAndAttrs |= FILE_FLAG_SEQUENTIAL_SCAN;
    if (mode.any(fTemp)) {
        assert(mode.any(fReadWrite));
        flagsAndAttrs |= FILE_ATTRIBUTE_TEMPORARY | FILE_FLAG_DELETE_ON_CLOSE;
    }

    auto handle = CreateFileW(
        toWstring(path).c_str(),
        access,
        share,
        NULL, // security attributes
        creation,
        flagsAndAttrs,
        NULL // template file
    );
    if (handle == INVALID_HANDLE_VALUE) {
        winFileSetErrno(WinError{});
        return {};
    }

    return allocHandle(handle, path, mode);
}

//===========================================================================
static string getName(HANDLE handle) {
    char tbuf[256];
    unique_ptr<char[]> buf;
    auto fni = (FILE_NAME_INFO *) tbuf;
    DWORD bufLen = sizeof tbuf;
    for (;;) {
        if (GetFileInformationByHandleEx(
            handle,
            FileNameInfo,
            fni,
            bufLen
        )) {
            break;
        }

        WinError err;
        if (err != ERROR_INSUFFICIENT_BUFFER || buf) {
            winFileSetErrno(err);
            return {};
        }

        bufLen = sizeof *fni + 2 * fni->FileNameLength;
        buf = make_unique<char[]>(bufLen);
        fni = (FILE_NAME_INFO *) buf.get();
    }

    return toString(wstring_view(fni->FileName, fni->FileNameLength));
}

//===========================================================================
FileHandle Dim::fileOpen(intptr_t osfhandle, EnumFlags<File::OpenMode> mode) {
    auto handle = (HANDLE) osfhandle;
    auto path = getName(handle);
    return allocHandle(handle, path, mode);
}

//===========================================================================
FileHandle Dim::fileCreateTemp(
    EnumFlags<File::OpenMode> mode, 
    string_view suffix
) {
    auto fname = fileTempName(suffix);
    if (fname.empty())
        return {};
    mode |= File::fCreat | File::fExcl | File::fReadWrite;
    return fileOpen(fname, mode);
}

//===========================================================================
static FileHandle attachStdHandle(
    int fd,
    string_view path,
    EnumFlags<File::OpenMode> mode // must be either fReadOnly or fReadWrite
) {
    assert(mode.value() == File::fReadOnly 
        || mode.value() == File::fReadWrite);
    auto file = make_unique<WinFileInfo>();
    file->m_mode = mode | File::fBlocking;
    file->m_path = path;
    file->m_handle = GetStdHandle(fd);
    if (file->m_handle == NULL) {
        // Process doesn't have a console or otherwise redirected standard
        // input/output.
        file->m_handle = INVALID_HANDLE_VALUE;
        winFileSetErrno(ERROR_FILE_NOT_FOUND);
        return {};
    } else if (file->m_handle == INVALID_HANDLE_VALUE) {
        winFileSetErrno(WinError{});
        return {};
    }
    // Duplicate the handle so that fileClose works just like for any other
    // file handle.
    auto proc = GetCurrentProcess();
    if (!DuplicateHandle(
        proc,   // source process
        file->m_handle,
        proc,   // target process
        &file->m_handle,
        0,      // access (ignored)
        FALSE,  // inheritable
        DUPLICATE_SAME_ACCESS
    )) {
        winFileSetErrno(WinError{});
        return {};
    }

    unique_lock lk{s_fileMut};
    auto f = s_files.insert(file.get());
    file.release()->m_f = f;
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
        unique_lock lk{s_fileMut};
        return s_files.release(f);
    } else {
        shared_lock lk{s_fileMut};
        return s_files.find(f);
    }
}

//===========================================================================
bool Dim::fileResize(FileHandle f, size_t size) {
    auto file = getInfo(f);
    if (!file)
        return winFileSetErrno(ERROR_INVALID_PARAMETER);
    LARGE_INTEGER tmp;
    tmp.QuadPart = size;
    if (!SetFilePointerEx(file->m_handle, tmp, nullptr, FILE_BEGIN)) {
        WinError err;
        logMsgError() << "SetFilePointerEx(" << file->m_path << "): " << err;
        return winFileSetErrno(err);
    }
    if (!SetEndOfFile(file->m_handle)) {
        WinError err;
        logMsgError() << "SetEndOfFile(" << file->m_path << "): " << err;
        return winFileSetErrno(err);
    }
    return true;
}

//===========================================================================
bool Dim::fileFlush(FileHandle f) {
    auto file = getInfo(f);
    if (!file)
        return winFileSetErrno(ERROR_INVALID_PARAMETER);
    if (!FlushFileBuffers(file->m_handle)) {
        WinError err;
        logMsgError() << "FlushFileBuffers(" << file->m_path << "): " << err;
        return winFileSetErrno(err);
    }
    return true;
}

//===========================================================================
bool Dim::fileFlushViews(FileHandle f) {
    auto file = getInfo(f);
    if (!file) {
        winFileSetErrno(ERROR_INVALID_PARAMETER);
        return false;
    }
    WinError err{0};
    for (auto && kv : file->m_views) {
        if (!FlushViewOfFile(kv.first, 0)) {
            err.set();
            logMsgError() << "FlushViewOfFile(" << file->m_path << "): "
                << err;
        }
    }
    return winFileSetErrno(err);
}

//===========================================================================
void Dim::fileClose(FileHandle f) {
    auto file = static_cast<unique_ptr<WinFileInfo>>(getInfo(f, true));
    if (!file)
        return;
    if (file->m_handle != INVALID_HANDLE_VALUE) {
        if (!file->m_views.empty()) {
            logMsgFatal() << "fileClose(" << file->m_path
                << "): has views that are still open";
        }
        if (file->m_mode.none(File::fNonOwning)) {
            if (file->m_mode.any(File::fBlocking))
                CancelIoEx(file->m_handle, NULL);
            CloseHandle(file->m_handle);
        }
        file->m_handle = INVALID_HANDLE_VALUE;
    }
}

//===========================================================================
uint64_t Dim::fileSize(FileHandle f) {
    auto file = getInfo(f);
    if (!file) {
        winFileSetErrno(ERROR_INVALID_PARAMETER);
        return 0;
    }
    LARGE_INTEGER size;
    if (!GetFileSizeEx(file->m_handle, &size)) {
        WinError err;
        logMsgError() << "GetFileSizeEx(" << file->m_path << "): " << err;
        winFileSetErrno(err);
        return 0;
    }
    if (!size.QuadPart)
        winFileSetErrno(NO_ERROR);
    return size.QuadPart;
}

//===========================================================================
TimePoint Dim::fileLastWriteTime(FileHandle f) {
    auto file = getInfo(f);
    if (!file) {
        winFileSetErrno(ERROR_INVALID_PARAMETER);
        return TimePoint::min();
    }
    FILETIME ctime, atime, wtime;
    if (!GetFileTime(file->m_handle, &ctime, &atime, &wtime)) {
        WinError err;
        logMsgError() << "GetFileTime(" << file->m_path << "): " << err;
        winFileSetErrno(err);
        return TimePoint::min();
    }
    auto dur = duration(wtime);
    if (!dur.count())
        return TimePoint::min();

    winFileSetErrno(NO_ERROR);
    return TimePoint(dur);
}

//===========================================================================
string_view Dim::filePath(FileHandle f) {
    if (auto file = getInfo(f))
        return file->m_path;
    winFileSetErrno(ERROR_INVALID_PARAMETER);
    return {};
}

//===========================================================================
EnumFlags<File::OpenMode> Dim::fileMode(FileHandle f) {
    if (auto file = getInfo(f))
        return file->m_mode;
    winFileSetErrno(ERROR_INVALID_PARAMETER);
    return {};
}

//===========================================================================
File::Type Dim::fileType(FileHandle f) {
    auto file = getInfo(f);
    DWORD type = GetFileType(file->m_handle);
    switch (type) {
    case FILE_TYPE_CHAR: return File::Type::kCharacter;
    case FILE_TYPE_DISK: return File::Type::kRegular;
    case FILE_TYPE_UNKNOWN:
        winFileSetErrno(WinError{});
        return File::Type::kUnknown;
    default:
        winFileSetErrno(NO_ERROR);
        return File::Type::kUnknown;
    }
}

//===========================================================================
FileAlignment Dim::fileAlignment(FileHandle f) {
    FileAlignment out = {};
    auto file = getInfo(f);
    if (!file) {
        winFileSetErrno(ERROR_INVALID_PARAMETER);
        return out;
    }

    FILE_STORAGE_INFO fi;
    if (!GetFileInformationByHandleEx(
        file->m_handle,
        FileStorageInfo,
        &fi,
        (DWORD) sizeof fi
    )) {
        winFileSetErrno(WinError{});
        return out;
    }

    winFileSetErrno(NO_ERROR);
    out.logicalSector = fi.LogicalBytesPerSector;
    out.physicalSector =
        fi.FileSystemEffectivePhysicalBytesPerSectorForAtomicity;
    return out;
}

//===========================================================================
Path Dim::fileGetCurrentDir(string_view drive) {
    wchar_t wdrive[3] = {};
    switch (drive.size()) {
    default:
        if (drive[1] != ':')
            return {};
        [[fallthrough]];
    case 1:
        if (drive[0] < 'A'
            || drive[0] > 'Z' && drive[0] < 'a'
            || drive[0] > 'z'
        ) {
            return {};
        }
        wdrive[0] = drive[0];
        wdrive[1] = ':';
        break;
    case 0:
        wdrive[0] = '.';
        break;
    }

    wstring wstr;
    wchar_t wbuf[MAX_PATH];
    auto wpath = wbuf;
    auto len = GetFullPathNameW(wdrive, (DWORD) size(wbuf), wpath, nullptr);
    if (len > size(wbuf)) {
        wstr = wstring(len, 0);
        wpath = wstr.data();
        len = GetFullPathNameW(wdrive, len, wpath, nullptr);
    }
    if (!len) {
        logMsgError() << "GetFullPathNameW(" << drive << "): " << WinError{};
        return {};
    }
    return Path{toString(wpath)};
}

//===========================================================================
Path Dim::fileSetCurrentDir(string_view path) {
    if (!SetCurrentDirectoryW(toWstring(path).c_str())) {
        WinError err;
        logMsgError() << "SetCurrentDirectoryW(" << path << "): " << err;
        return {};
    }
    return fileGetCurrentDir(path);
}

//===========================================================================
void Dim::fileRead(
    IFileReadNotify * notify,
    void * outBuf,
    size_t outBufLen,
    FileHandle f,
    int64_t off,
    int64_t len,
    TaskQueueHandle hq
) {
    assert(notify);
    auto file = getInfo(f);
    assert(file);
    auto ptr = new FileReader{notify, hq};
    ptr->start(file, outBuf, outBufLen, off, len ? len : kNpos);
}

//===========================================================================
size_t Dim::fileReadWait(
    void * outBuf,
    size_t outBufLen,
    FileHandle f,
    int64_t off
) {
    auto file = getInfo(f);
    assert(file);
    FileReader op{nullptr, s_hq};
    return op.start(file, outBuf, outBufLen, off, outBufLen);
}

//===========================================================================
void Dim::fileWrite(
    IFileWriteNotify * notify,
    FileHandle f,
    int64_t off,
    const void * buf,
    size_t bufLen,
    TaskQueueHandle hq
) {
    assert(notify);
    auto file = getInfo(f);
    assert(file);
    auto ptr = new FileWriter(notify, hq);
    ptr->start(file, const_cast<void *>(buf), bufLen, off, bufLen);
}

//===========================================================================
void Dim::fileWrite(
    IFileWriteNotify * notify,
    FileHandle f,
    int64_t offset,
    string_view data,
    TaskQueueHandle hq
) {
    fileWrite(notify, f, offset, data.data(), data.size(), hq);
}

//===========================================================================
size_t Dim::fileWriteWait(
    FileHandle f,
    int64_t off,
    const void * buf,
    size_t bufLen
) {
    auto file = getInfo(f);
    assert(file);
    FileWriter op(nullptr, s_hq);
    return op.start(file, const_cast<void *>(buf), bufLen, off, bufLen);
}

//===========================================================================
size_t Dim::fileWriteWait(FileHandle f, int64_t offset, string_view data) {
    return fileWriteWait(f, offset, data.data(), data.size());
}

//===========================================================================
void Dim::fileAppend(
    IFileWriteNotify * notify,
    FileHandle f,
    const void * buf,
    size_t bufLen,
    TaskQueueHandle hq
) {
    assert(notify);
    // file writes to offset 2^64-1 are interpreted as appends to the end
    // of the file.
    fileWrite(notify, f, 0xffff'ffff'ffff'ffff, buf, bufLen, hq);
}

//===========================================================================
void Dim::fileAppend(
    IFileWriteNotify * notify,
    FileHandle f,
    string_view data,
    TaskQueueHandle hq
) {
    return fileAppend(notify, f, data.data(), data.size(), hq);
}

//===========================================================================
size_t Dim::fileAppendWait(FileHandle f, const void * buf, size_t bufLen) {
    return fileWriteWait(f, 0xffff'ffff'ffff'ffff, buf, bufLen);
}

//===========================================================================
size_t Dim::fileAppendWait(FileHandle f, string_view data) {
    return fileAppendWait(f, data.data(), data.size());
}


/****************************************************************************
*
*   Access control
*
***/

//===========================================================================
static unsigned getWindowsPerms(File::Access::Right right) {
    switch (right) {
    case File::Access::Right::kNone: return 0;
    case File::Access::Right::kFull: return FILE_ALL_ACCESS;
    case File::Access::Right::kModify:
        return FILE_GENERIC_READ | FILE_GENERIC_WRITE | FILE_GENERIC_EXECUTE
            | DELETE;
    case File::Access::Right::kReadAndExecute:
        return FILE_GENERIC_READ | FILE_GENERIC_EXECUTE;
    case File::Access::Right::kReadOnly: return FILE_GENERIC_READ;
    case File::Access::Right::kWriteOnly: return FILE_GENERIC_WRITE;
    case File::Access::Right::kDelete: return DELETE;
    default:
        break;
    }
    assert(!"Unknown file access right");
    return 0;
}

//===========================================================================
static bool updateNamedAccess(
    string_view path,
    string_view trustee, // name of account or group
    ACCESS_MODE mode,
    File::Access::Right allow,
    File::Access::Inherit inherit
) {
    WinError err = 0;
    SECURITY_DESCRIPTOR * sd = nullptr;
    ACL * aclNew = nullptr;

    for (;;) {
        ACL * aclOld;
        auto wpath = toWstring(path);
        err = GetNamedSecurityInfoW(
            wpath.c_str(),
            SE_FILE_OBJECT,
            DACL_SECURITY_INFORMATION,
            NULL, // owner
            NULL, // group
            &aclOld,
            NULL, // sacl
            (PSECURITY_DESCRIPTOR *) &sd
        );
        if (err)
            break;

        auto wtrustee = toWstring(trustee);
        EXPLICIT_ACCESSW access = {};
        access.grfAccessMode = mode;
        access.grfAccessPermissions = getWindowsPerms(allow);
        switch (inherit) {
        case File::Access::Inherit::kNone:
        default:
            access.grfInheritance = NO_INHERITANCE;
            break;
        case File::Access::Inherit::kOnly:
            access.grfInheritance =
                SUB_CONTAINERS_AND_OBJECTS_INHERIT | INHERIT_ONLY;
            break;
        case File::Access::Inherit::kAll:
            access.grfInheritance = SUB_CONTAINERS_AND_OBJECTS_INHERIT;
            break;
        }
        access.Trustee.MultipleTrusteeOperation = NO_MULTIPLE_TRUSTEE;
        access.Trustee.TrusteeForm = TRUSTEE_IS_NAME;
        access.Trustee.TrusteeType = TRUSTEE_IS_UNKNOWN;
        access.Trustee.ptstrName = wtrustee.data();
        ACL * aclNew;
        err = SetEntriesInAclW(1, &access, aclOld, &aclNew);
        if (err || !aclNew)
            break;

        err = SetNamedSecurityInfoW(
            wpath.data(),
            SE_FILE_OBJECT,
            DACL_SECURITY_INFORMATION,
            NULL, // owner
            NULL, // group
            aclNew,
            NULL // sacl
        );
        break;
    }

    if (sd)
        LocalFree(sd);
    if (aclNew)
        LocalFree(aclNew);
    return winFileSetErrno(err);
}

//===========================================================================
bool Dim::fileAddAccess(
    string_view path,
    string_view trustee, // name or Sid of account or group
    File::Access::Right allow,
    File::Access::Inherit inherit
) {
    return updateNamedAccess(path, trustee, GRANT_ACCESS, allow, inherit);
}

//===========================================================================
bool Dim::fileSetAccess(
    string_view path,
    string_view trustee, // name or Sid of account or group
    File::Access::Right allow,
    File::Access::Inherit inherit
) {
    return updateNamedAccess(path, trustee, SET_ACCESS, allow, inherit);
}


/****************************************************************************
*
*   Copy
*
***/

namespace {

struct ProgressTask : ITaskNotify {
    IFileCopyNotify * m_notify{};
    wstring m_wdst;
    wstring m_wsrc;
    TaskQueueHandle m_hq;
    int64_t m_offset{};
    int64_t m_copied{};
    int64_t m_total{};
    BOOL m_cancel{};
    RunMode m_mode{kRunStarting};

    void onTask() override;
};

} // namespace

//===========================================================================
static DWORD CALLBACK copyProgress(
    LARGE_INTEGER totalBytes,
    LARGE_INTEGER totalBytesXfer,
    LARGE_INTEGER streamBytes,
    LARGE_INTEGER streamBytesXfer,
    DWORD stream,
    DWORD reason,
    HANDLE src,
    HANDLE dst,
    void * param
) {
    if (reason == CALLBACK_CHUNK_FINISHED) {
        auto task = (ProgressTask *) param;
        task->m_copied = totalBytesXfer.QuadPart - task->m_offset;
        task->m_total = totalBytes.QuadPart;
        taskPush(task->m_hq, task);
    }
    return PROGRESS_CONTINUE;
}

//===========================================================================
void ProgressTask::onTask() {
    if (m_mode == kRunStarting) {
        m_mode = kRunRunning;
        if (!CopyFileExW(
            m_wsrc.c_str(),
            m_wdst.c_str(),
            copyProgress,
            this,
            &m_cancel,
            COPY_FILE_OPEN_SOURCE_FOR_WRITE
        )) {
            WinError err;
            m_cancel = true;
        }

        m_mode = kRunStopped;
        taskPush(m_hq, this);
        return;
    }

    if (m_mode == kRunRunning) {
        if (m_copied) {
            if (!m_notify->onFileCopy(m_offset, m_copied, m_total))
                m_cancel = true;
            m_offset += m_copied;
            m_copied = 0;
        }
        return;
    }

    assert(m_mode == kRunStopped);
    m_notify->onFileEnd(m_offset, m_total);
    delete this;
}

//===========================================================================
void Dim::fileCopy(
    IFileCopyNotify * notify,
    string_view dst,
    string_view src,
    TaskQueueHandle hq
) {
    assert(notify);
    auto task = new ProgressTask;
    task->m_notify = notify;
    task->m_wdst = toWstring(dst);
    task->m_wsrc = toWstring(src);
    task->m_hq = hq ? hq : taskEventQueue();
    taskPush(s_hq, task);
}


/****************************************************************************
*
*   Public View API
*
***/

//===========================================================================
size_t Dim::filePageSize(FileHandle f) {
    // Use the page size of the system memory manager as a first approximation
    // as it will always be a multiple of the physical sector size of any
    // filesystem that can support a page file.
    //
    // TODO: Get the physical sector size of the underlying device
    auto pageSize = envMemoryConfig().pageSize;

    // must be a power of 2
    assert(popcount(pageSize) == 1);

    return pageSize;
}

//===========================================================================
size_t Dim::fileViewAlignment(FileHandle f) {
    auto & mem = envMemoryConfig();

    // must be a multiple of the page size
    assert(mem.allocAlign % mem.pageSize == 0);
    // must be a power of 2
    assert(popcount(mem.allocAlign) == 1);

    return mem.allocAlign;
}

//===========================================================================
static bool openView(
    char *& base,
    FileHandle f,
    File::View mode,
    int64_t offset,
    int64_t length,
    int64_t maxLength
) {
    auto pageSize = filePageSize(f);
    assert(length % pageSize == 0);
    assert(maxLength % pageSize == 0);
    assert(offset % fileViewAlignment(f) == 0);

    base = nullptr;
    auto file = getInfo(f);

    WinError err{0};
    HANDLE sec;
    LARGE_INTEGER secSize;
    SIZE_T viewSize;
    ULONG access, secProt, allocType, pageProt;

    if (mode == File::View::kReadOnly) {
        if (!maxLength || file->m_mode.any(File::fReadOnly)) {
            assert(!maxLength);
            // read only view
            access = SECTION_MAP_READ;
            secProt = PAGE_READONLY;
            secSize.QuadPart = 0;
            viewSize = (SIZE_T) length;
            assert((uint64_t) viewSize == (uint64_t) length);
            allocType = 0;
            pageProt = PAGE_READONLY;
        } else {
            assert(file->m_mode.any(File::fReadWrite));
            assert(maxLength >= length);
            // read only but extendable
            access = SECTION_MAP_READ;
            secProt = PAGE_READWRITE;
            secSize.QuadPart = offset + max(pageSize, (size_t) length);
            viewSize = (SIZE_T) maxLength;
            assert((uint64_t) viewSize == (uint64_t) maxLength);
            allocType = MEM_RESERVE;
            pageProt = PAGE_READONLY;
        }
    } else {
        assert(mode == File::View::kReadWrite);
        assert(file->m_mode.any(File::fReadWrite));
        if (!maxLength) {
            // writable view
            access = SECTION_MAP_READ | SECTION_MAP_WRITE;
            secProt = PAGE_READWRITE;
            secSize.QuadPart = 0;
            viewSize = (SIZE_T) length;
            assert((uint64_t) viewSize == (uint64_t) length);
            allocType = 0;
            pageProt = PAGE_READWRITE;
        } else {
            assert(maxLength >= length);
            // fully writable and extendable view
            access = SECTION_MAP_READ | SECTION_MAP_WRITE;
            secProt = PAGE_READWRITE;
            secSize.QuadPart = offset + max(pageSize, (size_t) length);
            viewSize = (SIZE_T) maxLength;
            assert((uint64_t) viewSize == (uint64_t) maxLength);
            allocType = MEM_RESERVE;
            pageProt = PAGE_READWRITE;
        }
    }

    err = (WinError::NtStatus) s_NtCreateSection(
        &sec,
        access,
        nullptr,        // object attributes
        &secSize,
        secProt,
        SEC_RESERVE,
        file->m_handle
    );
    if (err) {
        logMsgError() << "NtCreateSection(" << file->m_path << "): " << err;
        winFileSetErrno(err);
        return false;
    }

    LARGE_INTEGER viewOffset;
    viewOffset.QuadPart = offset;
    err = (WinError::NtStatus) s_NtMapViewOfSection(
        sec,
        GetCurrentProcess(),
        (const void **)&base,
        0,  // number of high order address bits that must be zero
        0,  // commit size
        &viewOffset,
        &viewSize,
        ViewUnmap,
        allocType,
        pageProt
    );
    if (err) {
        logMsgError() << "NtMapViewOfSection(" << file->m_path << "): " << err;
        winFileSetErrno(err);
    } else {
        file->m_views[base] = mode;
    }

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
bool Dim::fileOpenView(
    const char *& base,
    FileHandle f,
    File::View mode,
    int64_t offset,
    int64_t length,
    int64_t maxLength
) {
    assert(mode == File::View::kReadOnly);
    return openView((char *&) base, f, mode, offset, length, maxLength);
}

//===========================================================================
bool Dim::fileOpenView(
    char *& base,
    FileHandle f,
    File::View mode,
    int64_t offset,
    int64_t length,
    int64_t maxLength
) {
    assert(mode == File::View::kReadWrite);
    return openView(base, f, mode, offset, length, maxLength);
}

//===========================================================================
void Dim::fileCloseView(FileHandle f, const void * view) {
    auto file = getInfo(f);
    auto found = file->m_views.erase(view);
    if (!found) {
        logMsgError() << "fileCloseView(" << file->m_path
            << "): unknown view, " << (void *) view;
    }
    WinError err = (WinError::NtStatus) s_NtUnmapViewOfSection(
        GetCurrentProcess(),
        view
    );
    if (err) {
        logMsgError() << "NtUnmapViewOfSection(" << file->m_path
            << "): " << err;
    }
}

//===========================================================================
void Dim::fileExtendView(FileHandle f, const void * view, int64_t length) {
    auto file = getInfo(f);
    auto i = file->m_views.find(view);
    if (i == file->m_views.end()) {
        logMsgFatal() << "fileExtendView(" << file->m_path
            << "): unknown view, " << (void *) view;
    }
    ULONG pageProt;
    if (i->second == File::View::kReadOnly) {
        pageProt = PAGE_READONLY;
    } else {
        assert(i->second == File::View::kReadWrite);
        pageProt = PAGE_READWRITE;
    }
    assert((uint64_t) length == (uint64_t) (SIZE_T) length);
    void * ptr = VirtualAlloc(
        (void *) view,
        (SIZE_T) length,
        MEM_COMMIT,
        pageProt
    );
    if (!ptr) {
        logMsgFatal() << "VirtualAlloc(" << file->m_path
            << "): " << WinError{};
    }
    if (ptr != view) {
        logMsgDebug() << "VirtualAlloc(" << file->m_path << "): " << ptr
            << " (expected " << view << ")";
    }
}
