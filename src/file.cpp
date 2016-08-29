// file.cpp - dim services
#include "pch.h"
#pragma hdrstop

using namespace std;

namespace Dim {


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

  public:
    FileProxyNotify(IFileReadNotify * notify);

    // IFileReadNotify
    bool
    onFileRead(char data[], int bytes, int64_t offset, IFile * file) override;
    void onFileEnd(int64_t offset, IFile * file) override;
};
}

//===========================================================================
FileProxyNotify::FileProxyNotify(IFileReadNotify * notify)
    : m_notify(notify) {}

//===========================================================================
bool FileProxyNotify::onFileRead(
    char data[], int bytes, int64_t offset, IFile * file) {
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
void fileReadBinary(
    IFileReadNotify * notify,
    std::string & out,
    const std::experimental::filesystem::path & path) {
    unique_ptr<IFile> file;
    if (!fileOpen(file, path, IFile::kReadOnly | IFile::kDenyNone)) {
        logMsgError() << "file open failed, " << path;
        return;
    }

    auto proxy = new FileProxyNotify(notify);
    size_t bytes = fileSize(file.get());
    out.reserve(bytes);
    fileRead(proxy, const_cast<char *>(out.data()), bytes, file.release());
}

} // namespace
