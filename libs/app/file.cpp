// Copyright Glen Knowles 2016 - 2017.
// Distributed under the Boost Software License, Version 1.0.
//
// file.cpp - dim app
#include "pch.h"
#pragma hdrstop

using namespace std;
using namespace Dim;


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
        string_view path, 
        size_t blkSize
    );
    bool onFileRead(
        string_view data, 
        int64_t offset, 
        FileHandle f
    ) override;
    void onFileEnd(int64_t offset, FileHandle f) override;
};
}

//===========================================================================
FileStreamNotify::FileStreamNotify(
    IFileReadNotify * notify, 
    string_view path,
    size_t blkSize
)
    : m_notify{notify}
    , m_out(new char[blkSize])
{
    auto file = fileOpen(path, File::fReadOnly | File::fDenyNone);
    if (!file) {
        logMsgError() << "File open failed: " << path;
        onFileEnd(0, file);
    } else {
        fileRead(this, m_out.get(), blkSize, file);
    }
}

//===========================================================================
bool FileStreamNotify::onFileRead(
    string_view data, 
    int64_t offset,
    FileHandle f
) {
    return m_notify->onFileRead(data, offset, f);
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
        string_view data, 
        int64_t offset, 
        FileHandle f
    ) override;
    void onFileEnd(int64_t offset, FileHandle f) override;
};
}

//===========================================================================
FileLoadNotify::FileLoadNotify(string & out, IFileReadNotify * notify)
    : m_out(out)
    , m_notify(notify) {}

//===========================================================================
bool FileLoadNotify::onFileRead(
    string_view data, 
    int64_t offset,
    FileHandle f
) {
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
*   Public API
*
***/

//===========================================================================
void Dim::fileStreamBinary(
    IFileReadNotify * notify,
    string_view path,
    size_t blkSize
) {
    new FileStreamNotify(notify, path, blkSize);
}

//===========================================================================
void Dim::fileLoadBinary(
    IFileReadNotify * notify,
    string & out,
    string_view path,
    size_t maxSize
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
    fileRead(proxy, out.data(), bytes, file);
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
