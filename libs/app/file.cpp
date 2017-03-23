// file.cpp - dim services
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
    string m_out;
public:
    FileStreamNotify(
        IFileReadNotify * notify, 
        string_view path, 
        size_t blkSize
    );
    bool onFileRead(
        char data[], 
        int bytes, 
        int64_t offset, 
        IFile * file
    ) override;
    void onFileEnd(int64_t offset, IFile * file) override;
};
}

//===========================================================================
FileStreamNotify::FileStreamNotify(
    IFileReadNotify * notify, 
    string_view path,
    size_t blkSize
)
    : m_notify{notify}
    , m_out(blkSize, 0)
{
    auto file = fileOpen(path, IFile::kReadOnly | IFile::kDenyNone);
    if (!file) {
        logMsgError() << "File open failed, " << path;
        onFileEnd(0, nullptr);
    } else {
        fileRead(this, m_out.data(), m_out.size(), file.release());
    }
}

//===========================================================================
bool FileStreamNotify::onFileRead(
    char data[],
    int bytes,
    int64_t offset,
    IFile * file) {
    return m_notify->onFileRead(m_out.data(), bytes, offset, file);
}

//===========================================================================
void FileStreamNotify::onFileEnd(int64_t offset, IFile * file) {
    m_notify->onFileEnd(offset, file);
    delete file;
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
        char data[], 
        int bytes, 
        int64_t offset, 
        IFile * file
    ) override;
    void onFileEnd(int64_t offset, IFile * file) override;
};
}

//===========================================================================
FileLoadNotify::FileLoadNotify(string & out, IFileReadNotify * notify)
    : m_out(out)
    , m_notify(notify) {}

//===========================================================================
bool FileLoadNotify::onFileRead(
    char data[],
    int bytes,
    int64_t offset,
    IFile * file) {
    // resize the string to match the bytes read, in case it was less than
    // the amount requested
    m_out.resize(bytes);
    return false;
}

//===========================================================================
void FileLoadNotify::onFileEnd(int64_t offset, IFile * file) {
    m_notify->onFileEnd(offset, file);
    delete file;
    delete this;
}


/****************************************************************************
*
*   External
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
    size_t maxSize) {
    auto file = fileOpen(path, IFile::kReadOnly | IFile::kDenyNone);
    if (!file) {
        logMsgError() << "File open failed, " << path;
        notify->onFileEnd(0, nullptr);
        return;
    }

    size_t bytes = fileSize(file.get());
    if (bytes > maxSize)
        logMsgError() << "File too large, " << bytes << ", " << path;
    out.resize(bytes);
    auto proxy = new FileLoadNotify(out, notify);
    fileRead(proxy, out.data(), bytes, file.release());
}

//===========================================================================
void Dim::fileLoadSyncBinary(
    string & out,
    string_view path,
    size_t maxSize) {
    auto file = fileOpen(path, IFile::kReadOnly | IFile::kDenyNone);
    if (!file) {
        logMsgError() << "File open failed, " << path;
        out.clear();
        return;
    }

    size_t bytes = fileSize(file.get());
    if (bytes > maxSize)
        logMsgError() << "File too large, " << bytes << ", " << path;
    out.resize(bytes);
    fileReadSync(out.data(), bytes, file.release(), 0);
}
