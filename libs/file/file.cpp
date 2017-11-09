// Copyright Glen Knowles 2016 - 2017.
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
    auto file = fileOpen(path, File::fReadOnly | File::fDenyNone);
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
    auto eod = m_out.get() + (&*data.end() - m_out.get());
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
    string & m_out;
public:
    FileLoadNotify(string & out, IFileReadNotify * notify);
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
FileLoadNotify::FileLoadNotify(string & out, IFileReadNotify * notify)
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
    m_out.resize(data.size());
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
TimePoint Dim::fileLastWriteTime(std::string_view path) {
    error_code ec;
    auto f = fs::u8path(path.begin(), path.end());
    auto tp = TimePoint{fs::last_write_time(f, ec).time_since_epoch()};
    return tp;
}

//===========================================================================
uint64_t Dim::fileSize(std::string_view path) {
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
bool Dim::fileRemove(std::string_view path) {
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
    string & out,
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

    size_t bytes = fileSize(file);
    if (bytes > maxSize)
        logMsgError() << "File too large (" << bytes << " bytes): " << path;
    out.resize(bytes);
    auto proxy = new FileLoadNotify(out, notify);
    fileRead(proxy, out.data(), bytes, file, 0, 0, hq);
}

//===========================================================================
void Dim::fileLoadBinaryWait(
    string & out,
    string_view path,
    size_t maxSize
) {
    auto file = fileOpen(path, File::fReadOnly | File::fDenyNone);
    if (!file) {
        logMsgError() << "File open failed: " << path;
        out.clear();
        return;
    }

    size_t bytes = fileSize(file);
    if (bytes > maxSize)
        logMsgError() << "File too large (" << bytes << " bytes): " << path;
    out.resize(bytes);
    fileReadWait(out.data(), bytes, file, 0);
    fileClose(file);
}
