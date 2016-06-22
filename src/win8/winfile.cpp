// winfile.cpp - dim core - windows platform
#include "pch.h"
#pragma hdrstop

using namespace std;
using namespace std::experimental::filesystem;

namespace Dim {

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
    HANDLE m_handle;
    OpenMode m_mode;

    virtual ~File();
};
}

/****************************************************************************
*
*   Variables
*
***/

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
        IFileReadNotify *notify, File *file, void *outBuf, size_t outBufLen);
    void read(int64_t off, int64_t len);

    // ITaskNotify
    void onTask() override;

  private:
    WinOverlappedEvent m_iocpEvt;
    int64_t m_offset{0};
    int64_t m_length{0};

    IFileReadNotify *m_notify;
    File *m_file;
    char *m_outBuf;
    int m_outBufLen;
};
}

//===========================================================================
FileReader::FileReader(
    IFileReadNotify *notify, File *file, void *outBuf, size_t outBufLen)
    : m_file(file)
    , m_notify(notify)
    , m_outBuf((char *)outBuf)
    , m_outBufLen((int)outBufLen) {
    m_iocpEvt.notify = this;
    m_iocpEvt.overlapped = {};
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
        if (err != ERROR_IO_PENDING) {
            logMsgError() << "ReadFile (" << m_file->m_path << "): " << err;
            m_notify->onFileEnd(m_offset, m_file);
            delete this;
        }
    }
}

//===========================================================================
void FileReader::onTask() {
    DWORD bytes;
    if (!GetOverlappedResult(NULL, &m_iocpEvt.overlapped, &bytes, false)) {
        WinError err;
        if (err != ERROR_OPERATION_ABORTED)
            logMsgError() << "ReadFile (" << m_file->m_path << "): " << err;
        m_notify->onFileEnd(m_offset, m_file);
        delete this;
        return;
    }

    bool more =
        bytes ? m_notify->onFileRead(m_outBuf, bytes, m_offset, m_file) : false;

    if (!more || m_length && m_length <= bytes) {
        m_notify->onFileEnd(m_offset + bytes, m_file);
        delete this;
    } else {
        read(m_offset + bytes, m_length ? m_length - bytes : 0);
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
        IFileWriteNotify *notify, File *file, void *buf, size_t bufLen);
    void write(int64_t off);

    // ITaskNotify
    void onTask() override;

  private:
    WinOverlappedEvent m_iocpEvt;
    int64_t m_offset{0};

    IFileWriteNotify *m_notify;
    File *m_file;
    char *m_buf;
    int m_bufLen;

    WinError m_err{0};
    mutex m_mut;
    condition_variable m_cv;
    bool m_complete{false};
    DWORD m_written{0};
};
}

//===========================================================================
FileWriteBuf::FileWriteBuf(
    IFileWriteNotify *notify, File *file, void *buf, size_t bufLen)
    : m_notify(notify)
    , m_file(file)
    , m_buf((char *)buf)
    , m_bufLen((int)bufLen)
    , m_complete{false} {
    assert(bufLen <= numeric_limits<int>::max());
    m_iocpEvt.notify = this;
    m_iocpEvt.overlapped = {};
}

//===========================================================================
void FileWriteBuf::write(int64_t off) {
    m_iocpEvt.overlapped = {};
    m_iocpEvt.overlapped.Offset = (DWORD)off;
    m_iocpEvt.overlapped.OffsetHigh = (DWORD)(off >> 32);

    if (!WriteFile(
            m_file->m_handle,
            m_buf,
            m_bufLen,
            &m_written,
            &m_iocpEvt.overlapped)) {
        m_err = WinError{};
        if (!m_notify && m_err == ERROR_IO_PENDING) {
            unique_lock<mutex> lk{m_mut};
            while (!m_complete)
                m_cv.wait(lk);
        }

        if (m_err && m_err != ERROR_IO_PENDING) {
            logMsgError() << "WriteFile (" << m_file->m_path << "): " << m_err;
            setErrno(m_err);
        }
    }

    if (m_notify) {
        m_notify->onFileWrite(m_written, m_buf, m_bufLen, m_offset, m_file);
    }
    delete this;
}

//===========================================================================
void FileWriteBuf::onTask() {
    DWORD bytes;
    bool result =
        !!GetOverlappedResult(NULL, &m_iocpEvt.overlapped, &bytes, false);

    if (m_notify) {
        if (!result) {
            m_err = WinError{};
            if (m_err != ERROR_OPERATION_ABORTED) {
                logMsgError() << "WriteFile (" << m_file->m_path
                              << "): " << m_err;
            }
            setErrno(m_err);
        }
        m_notify->onFileWrite(bytes, m_buf, m_bufLen, m_offset, m_file);
        delete this;
        return;
    }

    unique_lock<mutex> lk{m_mut};
    m_err = WinError{};
    m_written = bytes;
    m_complete = true;
    m_cv.notify_all();
}

/****************************************************************************
*
*   File
*
***/

//===========================================================================
File::~File() {
    if (m_handle != INVALID_HANDLE_VALUE)
        CloseHandle(m_handle);
}

/****************************************************************************
*
*   Internal API
*
***/

//===========================================================================
void iFileInitialize() {
    winIocpInitialize();
}

/****************************************************************************
*
*   Public API
*
***/

//===========================================================================
bool fileOpen(unique_ptr<IFile> &out, const path &path, IFile::OpenMode mode) {
    using om = IFile::OpenMode;

    out.reset();
    auto file = make_unique<File>();
    file->m_mode = mode;
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
    if (file->m_handle == INVALID_HANDLE_VALUE)
        return setErrno(WinError{});

    if (!winIocpBindHandle(file->m_handle))
        return setErrno(WinError{});

    out = move(file);
    return true;
}

//===========================================================================
void fileRead(
    IFileReadNotify *notify,
    void *outBuf,
    size_t outBufLen,
    IFile *ifile,
    int64_t off,
    int64_t len) {
    assert(notify);
    File *file = static_cast<File *>(ifile);
    auto ptr = new FileReader(notify, file, outBuf, outBufLen);
    ptr->read(off, len);
}

//===========================================================================
void fileWrite(
    IFile *ifile,
    void *buf,
    size_t bufLen,
    int64_t off,
    IFileWriteNotify *notify) {
    File *file = static_cast<File *>(ifile);
    auto ptr = new FileWriteBuf(notify, file, buf, bufLen);
    ptr->write(off);
}

//===========================================================================
void fileAppend(
    IFile *file, void *buf, size_t bufLen, IFileWriteNotify *notify) {
    // file writes to offset 2^64-1 are interpreted as appends to the end
    // of the file.
    fileWrite(file, buf, bufLen, 0xffff'ffff'ffff'ffff, notify);
}

} // namespace
