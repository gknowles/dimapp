// Copyright Glen Knowles 2016 - 2018.
// Distributed under the Boost Software License, Version 1.0.
//
// file.cpp - dim file
#include "pch.h"
#pragma hdrstop

using namespace std;
using namespace Dim;
namespace fs = std::experimental::filesystem;


/****************************************************************************
*
*   FileAppendStream
*
***/

//===========================================================================
FileAppendStream::FileAppendStream()
{}

//===========================================================================
FileAppendStream::FileAppendStream(int numBufs, int maxWrites, size_t pageSize) {
    init(numBufs, maxWrites, pageSize);
}

//===========================================================================
FileAppendStream::~FileAppendStream() {
    close();
    if (m_buffers)
        aligned_free(m_buffers);
}

//===========================================================================
void FileAppendStream::init(int numBufs, int maxWrites, size_t pageSize) {
    assert(!m_numBufs);
    m_numBufs = numBufs;
    m_maxWrites = maxWrites;
    m_bufLen = pageSize;
    assert(m_numBufs && m_maxWrites && m_maxWrites <= m_numBufs);
}

//===========================================================================
bool FileAppendStream::open(string_view path, OpenExisting mode) {
    auto flags = File::fReadWrite | File::fAligned;
    switch (mode) {
    case kFail: flags |= File::fCreat | File::fExcl; break;
    case kAppend: flags |= File::fCreat; break;
    case kTrunc: flags |= File::fCreat | File::fTrunc; break;
    }
    if (auto f = fileOpen(path, flags); !attach(f))
        fileClose(f);
    return (bool) *this;
}

//===========================================================================
bool FileAppendStream::attach(Dim::FileHandle f) {
    assert(m_bufLen);
    close();
    m_filePos = fileSize(f);
    if (!m_filePos && errno)
        return false;

    m_file = f;
    if (!m_buffers)
        m_buffers = (char *) aligned_alloc(m_bufLen, m_numBufs * m_bufLen);

    auto used = m_filePos % m_bufLen;
    m_buf = string_view{m_buffers + used, m_bufLen - used};
    m_filePos -= used;
    if (m_filePos)
        fileReadWait(m_buffers, m_bufLen, m_file, m_filePos);
    return true;
}

//===========================================================================
void FileAppendStream::close() {
    if (!m_file)
        return;

    unique_lock<mutex> lk{m_mut};
    while (m_fullBufs + m_lockedBufs)
        m_cv.wait(lk);

    if (auto used = m_bufLen - m_buf.size()) {
        if (~fileMode(m_file) & File::fAligned) {
            fileAppendWait(m_file, m_buf.data() - used, used);
        } else {
            // Since the old file handle was opened with fAligned we can't use
            // it to write the trailing partial buffer.
            Path path = filePath(m_file);
            fileClose(m_file);
            m_file = fileOpen(path, File::fReadWrite | File::fBlocking);
            if (m_file)
                fileAppendWait(m_file, m_buf.data() - used, used);
        }
    }
    fileClose(m_file);
    m_file = {};
}

//===========================================================================
void FileAppendStream::append(string_view data) {
    if (!m_file)
        return;

    while (data.size()) {
        auto bytes = min(data.size(), m_buf.size());
        memcpy((char *) m_buf.data(), data.data(), bytes);
        m_buf.remove_prefix(bytes);
        data.remove_prefix(bytes);
        if (!m_buf.empty())
            return;

        unique_lock<mutex> lk{m_mut};
        m_fullBufs += 1;
        if (m_buf.data() == m_buffers + m_numBufs * m_bufLen) {
            m_buf = {m_buffers, m_bufLen};
        } else {
            m_buf = {m_buf.data(), m_bufLen};
        }
        write_LK();

        while (m_fullBufs + m_lockedBufs == m_numBufs)
            m_cv.wait(lk);
    }
}

//===========================================================================
void FileAppendStream::write_LK() {
    if (m_numWrites == m_maxWrites)
        return;

    const char * writeBuf;
    size_t writeCount;
    auto epos = (int) ((m_buf.data() - m_buffers) / m_bufLen);
    if (m_fullBufs > epos) {
        writeCount = (m_fullBufs - epos) * m_bufLen;
        writeBuf = m_buffers + m_numBufs * m_bufLen - writeCount;
        m_lockedBufs += m_fullBufs - epos;
        m_fullBufs = epos;
    } else {
        writeCount = m_fullBufs * m_bufLen;
        writeBuf = m_buffers + epos * m_bufLen - writeCount;
        m_lockedBufs += m_fullBufs;
        m_fullBufs = 0;
    }
    if (!writeCount)
        return;

    m_numWrites += 1;
    size_t writePos = m_filePos;
    m_filePos += writeCount;

    fileWrite(
        this,
        m_file,
        writePos,
        writeBuf,
        writeCount,
        taskComputeQueue()
    );
}

//===========================================================================
void FileAppendStream::onFileWrite(
    int written,
    string_view data,
    int64_t offset,
    FileHandle f
) {
    {
        unique_lock<mutex> lk{m_mut};
        m_numWrites -= 1;
        m_lockedBufs -= (int) (data.size() / m_bufLen);
        write_LK();
    }
    m_cv.notify_all();
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
    bool onFileRead(
        size_t * bytesUsed,
        string_view data,
        int64_t offset,
        FileHandle f
    ) override;
    void onFileEnd(int64_t offset, FileHandle f) override;
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
    auto file = fileOpen(
        path,
        File::fReadOnly | File::fSequential | File::fDenyNone
    );
    if (!file) {
        logMsgError() << "File open failed: " << path;
        onFileEnd(0, file);
    } else {
        fileRead(this, m_out.get(), blkSize, file, 0, 0, hq);
    }
}

//===========================================================================
bool FileStreamNotify::onFileRead(
    size_t * bytesUsed,
    string_view data,
    int64_t offset,
    FileHandle f
) {
    auto eod = m_out.get() + data.size();
    *eod = 0;
    return m_notify->onFileRead(bytesUsed, data, offset, f);
}

//===========================================================================
void FileStreamNotify::onFileEnd(int64_t offset, FileHandle f) {
    m_notify->onFileEnd(offset, f);
    fileClose(f);
    delete this;
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
    bool onFileRead(
        size_t * bytesUsed,
        string_view data,
        int64_t offset,
        FileHandle f
    ) override;
    void onFileEnd(int64_t offset, FileHandle f) override;
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
    string_view data,
    int64_t offset,
    FileHandle f
) {
    *bytesUsed = data.size();
    // resize the string to match the bytes read, in case it was less than
    // the amount requested
    m_out->resize(data.size());
    return false;
}

//===========================================================================
void FileLoadNotify::onFileEnd(int64_t offset, FileHandle f) {
    m_notify->onFileEnd(offset, f);
    fileClose(f);
    delete this;
}


/****************************************************************************
*
*   Public Path API
*
***/

//===========================================================================
TimePoint Dim::fileLastWriteTime(string_view path) {
    error_code ec;
    auto f = fs::u8path(path.begin(), path.end());
    auto tp = TimePoint{fs::last_write_time(f, ec).time_since_epoch()};
    return tp;
}

//===========================================================================
uint64_t Dim::fileSize(string_view path) {
    error_code ec;
    auto f = fs::u8path(path.begin(), path.end());
    auto len = (uint64_t) fs::file_size(f, ec);
    if (ec) {
        auto cond = ec.default_error_condition();
        errno = cond.value();
        return len;
    }
    if (!len)
        errno = 0;
    return len;
}

//===========================================================================
bool Dim::fileRemove(string_view path) {
    error_code ec;
    auto f = fs::u8path(path.begin(), path.end());
    if (fs::remove(f, ec))
        return true;
    errno = ec.default_error_condition().value();
    return false;
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
    new FileStreamNotify(notify, hq, path, blkSize);
}

//===========================================================================
void Dim::fileLoadBinary(
    IFileReadNotify * notify,
    string * out,
    string_view path,
    size_t maxSize,
    TaskQueueHandle hq
) {
    auto file = fileOpen(path, File::fReadOnly | File::fDenyNone);
    if (!file) {
        logMsgError() << "File open failed: " << path;
        notify->onFileEnd(0, file);
        return;
    }

    auto bytes = fileSize(file);
    if (bytes > maxSize)
        logMsgError() << "File too large (" << bytes << " bytes): " << path;
    out->resize((size_t) bytes);
    auto proxy = new FileLoadNotify(out, notify);
    fileRead(proxy, out->data(), (size_t) bytes, file, 0, 0, hq);
}

//===========================================================================
void Dim::fileLoadBinaryWait(
    string * out,
    string_view path,
    size_t maxSize
) {
    auto file = fileOpen(path, File::fReadOnly | File::fDenyNone);
    if (!file) {
        logMsgError() << "File open failed: " << path;
        out->clear();
        return;
    }

    auto bytes = fileSize(file);
    if (bytes > maxSize)
        logMsgError() << "File too large (" << bytes << " bytes): " << path;
    out->resize((size_t) bytes);
    fileReadWait(out->data(), (size_t) bytes, file, 0);
    fileClose(file);
}
