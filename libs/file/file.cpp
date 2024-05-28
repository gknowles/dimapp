// Copyright Glen Knowles 2016 - 2023.
// Distributed under the Boost Software License, Version 1.0.
//
// file.cpp - dim file
#include "pch.h"
#pragma hdrstop

using namespace std;
using namespace Dim;
namespace fs = std::filesystem;
using fm = Dim::File::OpenMode;


/****************************************************************************
*
*   FileAppendStream
*
***/

struct FileAppendStream::Impl {
    std::mutex mut;
    std::condition_variable cv;
    int fullBufs = 0;     // ready to be written
    int lockedBufs = 0;   // being written
    int numBufs = 0;
    int maxWrites = 0;
    int numWrites = 0;
    char * buffers = nullptr;  // aligned to page boundary
    size_t bufLen = 0;

    Dim::FileHandle file;
    std::string_view buf;
    size_t filePos = 0;
};

//===========================================================================
FileAppendStream::FileAppendStream()
    : m_impl(new Impl)
{}

//===========================================================================
FileAppendStream::FileAppendStream(
    int numBufs,
    int maxWrites,
    size_t pageSize
)
    : FileAppendStream()
{
    init(numBufs, maxWrites, pageSize);
}

//===========================================================================
FileAppendStream::~FileAppendStream() {
    close();
    if (m_impl->buffers)
        freeAligned(m_impl->buffers);
}

//===========================================================================
FileAppendStream::operator bool() const {
    return (bool) m_impl->file;
}

//===========================================================================
void FileAppendStream::init(int numBufs, int maxWrites, size_t pageSize) {
    assert(!m_impl->numBufs && pageSize);
    m_impl->numBufs = numBufs;
    m_impl->maxWrites = maxWrites;
    m_impl->bufLen = pageSize;
    assert(m_impl->numBufs
        && m_impl->maxWrites
        && m_impl->maxWrites <= m_impl->numBufs);
}

//===========================================================================
bool FileAppendStream::open(string_view path, OpenExisting mode) {
    auto flags = fm::fReadWrite | fm::fAligned | fm::fDenyWrite;
    switch (mode) {
    case kFail: flags |= fm::fCreat | fm::fExcl; break;
    case kAppend: flags |= fm::fCreat; break;
    case kTrunc: flags |= fm::fCreat | fm::fTrunc; break;
    }
    FileHandle f;
    if (auto ec = fileOpen(&f, path, flags); !ec) {
        if (!attach(f))
            fileClose(f);
    }
    return (bool) *this;
}

//===========================================================================
bool FileAppendStream::attach(Dim::FileHandle f) {
    assert(m_impl->bufLen && "file append stream has no buffers assigned");
    close();
    if (auto ec = fileSize(&m_impl->filePos, f); ec)
        return false;

    m_impl->file = f;
    if (!m_impl->buffers) {
        m_impl->buffers = (char *) mallocAligned(
            m_impl->bufLen,
            m_impl->numBufs * m_impl->bufLen
        );
        __assume(m_impl->buffers);
    }

    auto used = m_impl->filePos % m_impl->bufLen;
    m_impl->buf = string_view{m_impl->buffers + used, m_impl->bufLen - used};
    m_impl->filePos -= used;
    if (m_impl->filePos) {
        if (auto ec = fileReadWait(
            nullptr,
            m_impl->buffers,
            m_impl->bufLen,
            m_impl->file,
            m_impl->filePos
        ); ec) {
            return false;
        }
    }
    return true;
}

//===========================================================================
void FileAppendStream::close() {
    if (!m_impl->file)
        return;

    unique_lock lk{m_impl->mut};
    while (m_impl->fullBufs + m_impl->lockedBufs)
        m_impl->cv.wait(lk);

    if (auto used = m_impl->bufLen - m_impl->buf.size()) {
        auto oflags = fileMode(m_impl->file);
        if (!oflags.any(fm::fAligned)) {
            // Access is unaligned, so write whatever still in the buffer
            // without doing anything special.
            fileAppendWait(
                nullptr,
                m_impl->file,
                m_impl->buf.data() - used,
                used
            );
        } else {
            // Since the old file handle was opened with fAligned we can't use
            // it to write the trailing partial buffer.
            auto path = filePath(m_impl->file);
            fileClose(m_impl->file);
            if (auto ec = fileOpen(
                &m_impl->file,
                path,
                fm::fReadWrite | fm::fBlocking
            ); !ec) {
                fileAppendWait(
                    nullptr,
                    m_impl->file,
                    m_impl->buf.data() - used,
                    used
                );
            }
        }
    }
    fileClose(m_impl->file);
    m_impl->file = {};
}

//===========================================================================
void FileAppendStream::append(string_view data) {
    if (!m_impl->file)
        return;

    while (data.size()) {
        auto bytes = min(data.size(), m_impl->buf.size());
        memcpy((char *) m_impl->buf.data(), data.data(), bytes);
        m_impl->buf.remove_prefix(bytes);
        data.remove_prefix(bytes);
        if (!m_impl->buf.empty())
            return;

        unique_lock lk{m_impl->mut};
        m_impl->fullBufs += 1;
        if (m_impl->buf.data() ==
                m_impl->buffers + m_impl->numBufs * m_impl->bufLen
        ) {
            m_impl->buf = {m_impl->buffers, m_impl->bufLen};
        } else {
            m_impl->buf = {m_impl->buf.data(), m_impl->bufLen};
        }
        write_LK();

        while (m_impl->fullBufs + m_impl->lockedBufs == m_impl->numBufs)
            m_impl->cv.wait(lk);
    }
}

//===========================================================================
void FileAppendStream::write_LK() {
    if (m_impl->numWrites == m_impl->maxWrites)
        return;

    const char * writeBuf;
    size_t writeCount;
    auto epos =
        (int) ((m_impl->buf.data() - m_impl->buffers) / m_impl->bufLen);
    if (m_impl->fullBufs > epos) {
        writeCount = (m_impl->fullBufs - epos) * m_impl->bufLen;
        writeBuf =
            m_impl->buffers + m_impl->numBufs * m_impl->bufLen - writeCount;
        m_impl->lockedBufs += m_impl->fullBufs - epos;
        m_impl->fullBufs = epos;
    } else {
        writeCount = m_impl->fullBufs * m_impl->bufLen;
        writeBuf = m_impl->buffers + epos * m_impl->bufLen - writeCount;
        m_impl->lockedBufs += m_impl->fullBufs;
        m_impl->fullBufs = 0;
    }
    if (!writeCount)
        return;

    m_impl->numWrites += 1;
    size_t writePos = m_impl->filePos;
    m_impl->filePos += writeCount;

    fileWrite(
        this,
        m_impl->file,
        writePos,
        writeBuf,
        writeCount,
        taskComputeQueue()
    );
}

//===========================================================================
void FileAppendStream::onFileWrite(const FileWriteData & data) {
    {
        unique_lock lk{m_impl->mut};
        m_impl->numWrites -= 1;
        m_impl->lockedBufs -= (int) (data.data.size() / m_impl->bufLen);
        write_LK();
    }
    m_impl->cv.notify_all();
}


/****************************************************************************
*
*   FileStreamNotify
*
***/

namespace {

class FileStreamNotify : public IFileReadNotify {
    IFileReadNotify * m_notify;
    unique_ptr<char[]> m_out;
public:
    FileStreamNotify(
        IFileReadNotify * notify,
        TaskQueueHandle hq,
        string_view path,
        size_t blkSize
    );
    bool onFileRead(size_t * bytesUsed, const FileReadData & data) override;
};

} // namespace

//===========================================================================
FileStreamNotify::FileStreamNotify(
    IFileReadNotify * notify,
    TaskQueueHandle hq,
    string_view path,
    size_t blkSize
)
    : m_notify{notify}
    , m_out(new char[blkSize + 1])
{
    FileHandle file;
    auto ec = fileOpen(
        &file,
        path,
        fm::fReadOnly | fm::fSequential | fm::fDenyNone
    );
    if (ec) {
        logMsgError() << "File open failed: " << path;
        FileReadData data = {};
        data.f = file;
        data.ec = ec;
        size_t bytesUsed = data.data.size();
        onFileRead(&bytesUsed, data);
    } else {
        fileRead(this, m_out.get(), blkSize, file, 0, 0, hq);
    }
}

//===========================================================================
bool FileStreamNotify::onFileRead(
    size_t * bytesUsed,
    const FileReadData & data
) {
    auto eod = m_out.get() + data.data.size();
    *eod = 0;
    if (!m_notify->onFileRead(bytesUsed, data)
        || !data.more
    ) {
        fileClose(data.f);
        delete this;
        return false;
    }
    return true;
}


/****************************************************************************
*
*   FileLoadNotify
*
***/

namespace {

class FileLoadNotify : public IFileReadNotify {
    IFileReadNotify * m_notify;
    string * m_out;
public:
    FileLoadNotify(string * out, IFileReadNotify * notify);
    bool onFileRead(size_t * bytesUsed, const FileReadData & data) override;
};

} // namespace

//===========================================================================
FileLoadNotify::FileLoadNotify(string * out, IFileReadNotify * notify)
    : m_notify(notify)
    , m_out(out)
{}

//===========================================================================
bool FileLoadNotify::onFileRead(
    size_t * bytesUsed,
    const FileReadData & data
) {
    // Resize the string to match the bytes read, in case it was less than
    // the amount requested.
    m_out->resize(data.data.size());
    m_notify->onFileRead(bytesUsed, data);
    fileClose(data.f);
    delete this;
    return false;
}


/****************************************************************************
*
*   FileSaveNotify
*
***/

namespace {

class FileSaveNotify : public IFileWriteNotify {
    IFileWriteNotify * m_notify;
public:
    FileSaveNotify(IFileWriteNotify * notify);
    void onFileWrite(const FileWriteData & data) override;
};

} // namespace

//===========================================================================
FileSaveNotify::FileSaveNotify(IFileWriteNotify * notify)
    : m_notify(notify)
{}

//===========================================================================
void FileSaveNotify::onFileWrite(const FileWriteData & data) {
    fileClose(data.f);
    FileWriteData tmp = {};
    tmp.written = data.written;
    tmp.data = data.data;
    tmp.offset = data.offset;
    tmp.ec = data.ec;
    m_notify->onFileWrite(tmp);
    delete this;
}


/****************************************************************************
*
*   Public Path API
*
***/

//===========================================================================
error_code Dim::fileAbsolutePath(Path * out, string_view path) {
    *out = Path{path};
    Path tmp;
    auto ec = fileGetCurrentDir(&tmp, out->drive());
    if (!ec) {
        out->resolve(tmp);
    } else {
        out->clear();
    }
    return ec;
}

//===========================================================================
error_code Dim::fileChildPath(
    Path * out,
    const Path & root,
    string_view file,
    bool createDirIfNotExist
) {
    *out = file;
    out->resolve(root, "/");
    if (out->dir() == "/") {
        // Path doesn't have root as a parent.
        return make_error_code(errc::invalid_argument);
    }
    if (createDirIfNotExist) {
        auto fp = *out;
        if (auto ec = fileCreateDirs(fp.removeFilename()))
            return ec;
    }
    return {};
}

//===========================================================================
error_code Dim::fileSize(uint64_t * out, string_view path) {
    error_code ec;
    auto p8 = u8string_view((char8_t *) path.data(), path.size());
    auto f = fs::path(p8);
    *out = (uint64_t) fs::file_size(f, ec);
    if (ec) {
        logMsgError() << "fs::file_size(" << path << "): " << ec;
        *out = 0;
    }
    return ec;
}

//===========================================================================
static pair<error_code, fs::file_status> getPathStatus(string_view path) {
    error_code ec;
    auto p8 = u8string_view((char8_t *) path.data(), path.size());
    auto f = fs::path(p8);
    auto st = fs::status(f, ec);
    if (ec && st.type() != filesystem::file_type::not_found)
        logMsgError() << "fs::status(" << path << "): " << ec;
    return {ec, st};
}

//===========================================================================
error_code Dim::fileExists(bool * out, string_view path) {
    auto&& [ec, st] = getPathStatus(path);
    *out = fs::is_regular_file(st);
    if (ec == errc::no_such_file_or_directory)
        return {};
    return ec;
}

//===========================================================================
error_code Dim::fileDirExists(bool * out, string_view path) {
    auto&& [ec, st] = getPathStatus(path);
    *out = fs::is_directory(st);
    if (ec == errc::no_such_file_or_directory)
        return {};
    return ec;
}

//===========================================================================
error_code Dim::fileReadOnly(bool * out, string_view path) {
    auto&& [ec, st] = getPathStatus(path);
    *out = fs::exists(st)
        && (st.permissions() & fs::perms::owner_write) == fs::perms::none;
    return ec;
}

//===========================================================================
error_code Dim::fileReadOnly(string_view path, bool enable) {
    error_code ec;
    auto p8 = u8string_view((char8_t *) path.data(), path.size());
    auto f = fs::path(p8);
    auto st = fs::status(f, ec);
    if (ec) {
        logMsgError() << "fs::status(" << path << "): " << ec;
    } else {
        auto ro = fs::exists(st)
            && (st.permissions() & fs::perms::owner_write) == fs::perms::none;
        if (ro == enable)
            return {};

        auto perms = st.permissions();
        if (enable) {
            perms |= fs::perms::owner_write;
        } else {
            perms &= ~fs::perms::owner_write;
        }
        fs::permissions(f, perms, ec);
        if (ec)
            logMsgError() << "fs::permissions(" << path << "): " << ec;
    }
    return ec;
}

//===========================================================================
error_code Dim::fileResize(string_view path, size_t newSize) {
    error_code ec;
    auto p8 = u8string_view((char8_t *) path.data(), path.size());
    auto f = fs::path(p8);
    fs::resize_file(f, newSize, ec);
    if (ec)
        logMsgError() << "fs::resize_file(" << path << "): " << ec;
    return ec;
}

//===========================================================================
error_code Dim::fileRemove(string_view path, bool recurse) {
    error_code ec;
    auto p8 = u8string_view((char8_t *) path.data(), path.size());
    auto f = fs::path(p8);
    if (recurse) {
        fs::remove_all(f, ec);
        if (ec)
            logMsgError() << "fs::remove_all(" << path << "): " << ec;
    } else {
        fs::remove(f, ec);
        if (ec)
            logMsgError() << "fs::remove(" << path << "): " << ec;
    }
    return ec;
}

//===========================================================================
error_code Dim::fileCreateDirs(string_view path) {
    error_code ec;
    auto p8 = u8string_view((char8_t *) path.data(), path.size());
    auto f = fs::path(p8);
    fs::create_directories(f, ec);
    if (ec)
        logMsgError() << "fs::craete_directories(" << path << "): " << ec;
    return ec;
}


/****************************************************************************
*
*   Public API
*
***/

//===========================================================================
void Dim::fileStreamBinary(
    IFileReadNotify * notify,
    string_view path,
    size_t blkSize,
    TaskQueueHandle hq
) {
    NEW(FileStreamNotify)(notify, hq, path, blkSize);
}

//===========================================================================
void Dim::fileLoadBinary(
    IFileReadNotify * notify,
    string * out,
    string_view path,
    size_t maxSize,
    TaskQueueHandle hq
) {
    FileHandle file;
    auto ec = fileOpen(&file, path, fm::fReadOnly | fm::fDenyNone);
    if (ec) {
        logMsgError() << "File open failed: " << path;
        FileReadData data = {};
        data.f = file;
        data.ec = ec;
        size_t bytesUsed = 0;
        notify->onFileRead(&bytesUsed, data);
        return;
    }

    uint64_t bytes = 0;
    fileSize(&bytes, file);
    if (bytes > maxSize)
        logMsgError() << "File too large (" << bytes << " bytes): " << path;
    out->resize((size_t) bytes);
    auto proxy = NEW(FileLoadNotify)(out, notify);
    fileRead(proxy, out->data(), (size_t) bytes, file, 0, 0, hq);
}

//===========================================================================
error_code Dim::fileLoadBinaryWait(
    string * out,
    string_view path,
    size_t maxSize
) {
    FileHandle file;
    auto ec = fileOpen(
        &file,
        path,
        fm::fReadOnly | fm::fDenyNone | fm::fBlocking
    );
    if (ec) {
        logMsgError() << "File open failed: " << path;
        out->clear();
        return ec;
    }

    uint64_t bytes = 0;
    fileSize(&bytes, file);
    if (bytes > maxSize) {
        logMsgError() << "File too large (" << bytes << " bytes): " << path;
        out->clear();
        return make_error_code(errc::file_too_large);
    }
    out->resize((size_t) bytes);
    ec = fileReadWait(nullptr, out->data(), (size_t) bytes, file, 0);
    fileClose(file);
    return ec;
}

//===========================================================================
void Dim::fileSaveBinary(
    IFileWriteNotify * notify,
    string_view path,
    string_view data,
    TaskQueueHandle hq // queue to notify
) {
    FileHandle file;
    auto ec = fileOpen(&file, path, fm::fReadWrite | fm::fCreat | fm::fTrunc);
    if (ec) {
        logMsgError() << "File open failed: " << path;
        FileWriteData tmp = {};
        tmp.data = data;
        tmp.f = file;
        tmp.ec = ec;
        notify->onFileWrite(tmp);
        return;
    }

    auto proxy = NEW(FileSaveNotify)(notify);
    fileWrite(proxy, file, 0, data.data(), data.size(), hq);
}

//===========================================================================
error_code Dim::fileSaveBinaryWait(
    string_view path,
    string_view data
) {
    FileHandle file;
    auto ec = fileOpen(
        &file,
        path,
        fm::fReadWrite | fm::fCreat | fm::fTrunc | fm::fBlocking
    );
    if (ec) {
        logMsgError() << "File open failed: " << path;
        return ec;
    }

    uint64_t bytes = 0;
    ec = fileWriteWait(&bytes, file, 0, data.data(), data.size());
    fileClose(file);
    if (!ec && bytes == data.size())
        return make_error_code(errc::io_error);
    return ec;
}

//===========================================================================
void Dim::fileSaveTempFile(
    IFileWriteNotify * notify,
    string_view data,
    string_view suffix,
    TaskQueueHandle hq
) {
    FileHandle file;
    auto ec = fileCreateTemp(&file, {}, suffix);
    if (ec) {
        logMsgError() << "Create temp file failed.";
        FileWriteData tmp = {};
        tmp.data = data;
        tmp.f = file;
        tmp.ec = ec;
        notify->onFileWrite(tmp);
        return;
    }

    auto proxy = NEW(FileSaveNotify)(notify);
    fileWrite(proxy, file, 0, data.data(), data.size(), hq);
}
