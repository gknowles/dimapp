// file.cpp - dim services
#include "pch.h"
#pragma hdrstop

using namespace std;
using namespace Dim;


/****************************************************************************
*
*   Private
*
***/

/****************************************************************************
*
*   ReadNotify
*
***/

namespace {
class FileProxyNotify : public IFileReadNotify {
    IFileReadNotify * m_notify;
    string & m_out;

public:
    FileProxyNotify(string & out, IFileReadNotify * notify);

    // IFileReadNotify
    bool
    onFileRead(char data[], int bytes, int64_t offset, IFile * file) override;
    void onFileEnd(int64_t offset, IFile * file) override;
};
}

//===========================================================================
FileProxyNotify::FileProxyNotify(string & out, IFileReadNotify * notify)
    : m_out(out)
    , m_notify(notify) {}

//===========================================================================
bool FileProxyNotify::onFileRead(
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
void FileProxyNotify::onFileEnd(int64_t offset, IFile * file) {
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
void Dim::fileReadBinary(
    IFileReadNotify * notify,
    string & out,
    string_view path) {
    auto file = fileOpen(path, IFile::kReadOnly | IFile::kDenyNone);
    if (!file) {
        logMsgError() << "File open failed, " << path;
        notify->onFileEnd(0, nullptr);
        return;
    }

    size_t bytes = fileSize(file.get());
    out.resize(bytes);
    auto proxy = new FileProxyNotify(out, notify);
    fileRead(proxy, out.data(), bytes, file.release());
}
