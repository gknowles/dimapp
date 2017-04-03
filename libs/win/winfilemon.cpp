// Copyright Glen Knowles 2017.
// Distributed under the Boost Software License, Version 1.0.
//
// winfilemon.cpp - dim windows platform
#include "pch.h"
#pragma hdrstop

using namespace std;
using namespace Dim;
namespace fs = std::experimental::filesystem;


/****************************************************************************
*
*   Private
*
***/

namespace {

struct FileInfo {
    TimePoint mtime;
    vector<IFileChangeNotify *> notifiers;
};

class DirInfo : public ITaskNotify, public ITimerNotify {
public:
    DirInfo(IFileChangeNotify * notify);
    ~DirInfo();

    bool start(string_view path, bool recurse);
    void stopSync ();

    void addMonitor(IFileChangeNotify * notify, string_view file);
    void removeMonitorSync(IFileChangeNotify * notify, string_view file);

    string_view base() const { return m_base; }
    // returns false if file is outside of base directory
    bool expandPath(
        string & fullpath, 
        string & relpath, 
        string_view file
    ) const;

    void onTask () override;
    Duration onTimer (TimePoint now) override;

private:
    bool queue ();

    string m_base;
    bool m_recurse{false}; // monitoring includes child directories?
    IFileChangeNotify * m_notify{nullptr};

    unordered_map<string, FileInfo> m_files;
    HANDLE m_hDir{INVALID_HANDLE_VALUE};
    WinOverlappedEvent m_evt;
};

} // namespace


/****************************************************************************
*
*   Variables
*
***/

static HandleMap<FileMonitorHandle, DirInfo> s_dirs;


/****************************************************************************
*
*   DirInfo
*
***/

//===========================================================================
DirInfo::DirInfo(IFileChangeNotify * notify) 
    : m_notify{notify}
{}

//===========================================================================
DirInfo::~DirInfo () {
    stopSync();
    assert(m_hDir == INVALID_HANDLE_VALUE);
}

//===========================================================================
bool DirInfo::start(string_view path, bool recurse) {
    error_code ec;
    auto fp = fs::u8path(path.begin(), path.end());
    fp = fs::canonical(fp);
    m_base = fp.u8string();
    m_recurse = recurse;
    fs::create_directories(fp, ec);

    m_evt.notify = this;
    m_hDir = CreateFile(
        m_base.c_str(), 
        FILE_LIST_DIRECTORY, // access
        FILE_SHARE_READ | FILE_SHARE_WRITE, // share mode
        NULL, 
        OPEN_EXISTING, 
        FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED,
        NULL // template 
    );
    if (m_hDir == INVALID_HANDLE_VALUE) {
        WinError err;
        logMsgError() << "CreateFile(FILE_LIST_DIRECTORY), " << m_base 
            << ", " << err;
        iFileSetErrno(err);
        return false;
    }
    if (!winIocpBindHandle(m_hDir)) {
        CloseHandle(m_hDir);
        iFileSetErrno(WinError{});
        m_hDir = INVALID_HANDLE_VALUE;
        return false;
    }

    return queue();
}

//===========================================================================
bool DirInfo::queue() {
    if (!ReadDirectoryChangesW(
        m_hDir,
        NULL, // output buffer
        0, // output buffer len
        m_recurse, // include subdirs
        FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_LAST_WRITE,
        NULL, // bytes returned (not for async)
        &m_evt.overlapped,
        NULL // completion routine
    )) {
        WinError err;
        logMsgError() << "ReadDirectoryChangesW(), " << m_base << ", " << err;
        return false;
    }

    return true;
}

//===========================================================================
void DirInfo::stopSync () {
    if (m_hDir == INVALID_HANDLE_VALUE)
        return;

    if (!CloseHandle(m_hDir)) {
        WinError err;
        logMsgError() << "CloseHandle(hDir), " << m_base << ", " << err;
    }
    m_hDir = INVALID_HANDLE_VALUE;
    // wait until overlapped call completes
}

//===========================================================================
void DirInfo::addMonitor(IFileChangeNotify * notify, string_view file) {
    string fullpath;
    string relpath;
    if (!expandPath(fullpath, relpath, file)) {
        logMsgError() << "Monitor file not in base directory, " 
            << fullpath << ", " << m_base;
        return;
    }

    auto & fi = m_files[relpath];
    for (auto && ntf : fi.notifiers) {
        // already monitoring the file with this notifier
        if (ntf == notify)
            return;
    }
    if (fi.notifiers.empty()) {
        if (auto file = fileOpen(fullpath, IFile::kNoAccess))
            fi.mtime = fileLastWriteTime(file.get());
    }
    fi.notifiers.push_back(notify);
    notify->onFileChange(fullpath);
}

//===========================================================================
void DirInfo::removeMonitorSync(
    IFileChangeNotify * notify, 
    string_view file
) {
    string fullpath;
    string relpath;
    if (!expandPath(fullpath, relpath, file))
        return;

    auto & fi = m_files[relpath];
    for (auto i = fi.notifiers.begin(), e = fi.notifiers.end(); i != e; ++i) {
        if (*i == notify) {
            fi.notifiers.erase(i);
            return;
        }
    }
}

//===========================================================================
void DirInfo::onTask () {
    DWORD bytes;
    WinError err{0};
    if (!GetOverlappedResult(NULL, &m_evt.overlapped, &bytes, false))
        err = WinError{};

    if (err == ERROR_SUCCESS)
        queue();
}

//===========================================================================
Duration DirInfo::onTimer (TimePoint now) {
    return kTimerInfinite;
}

//===========================================================================
bool DirInfo::expandPath(
    string & fullpath, 
    string & relpath, 
    string_view file
) const {
    auto fp = fs::u8path(file.begin(), file.end());
    fp = fs::absolute(fp, fs::u8path(m_base));
    fullpath = fp.u8string();
    if (fullpath.compare(0, m_base.size(), m_base) != 0) {
        relpath.clear();
        return false;
    }

    relpath = fullpath;
    relpath.erase(0, m_base.size());
    return true;
}


/****************************************************************************
*
*   Public API
*
***/

//===========================================================================
FileMonitorHandle Dim::fileMonitorDir(
    string_view dir,
    bool recurse,
    IFileChangeNotify * notify
) {
    auto di = new DirInfo(notify);
    auto h = s_dirs.insert(di);
    di->start(dir, recurse);
    return h;
}

//===========================================================================
void Dim::fileMonitorStopSync(FileMonitorHandle dir) {
    s_dirs.erase(dir);
}

//===========================================================================
string_view Dim::fileMonitorPath(FileMonitorHandle dir) {
    if (auto di = s_dirs.find(dir))
        return di->base();
    return {};
}

//===========================================================================
void Dim::fileMonitor(
    FileMonitorHandle dir, 
    string_view file,
    IFileChangeNotify * notify
) {
    auto di = s_dirs.find(dir);
    assert(di);
    di->addMonitor(notify, file);
}

//===========================================================================
void Dim::fileMonitorStopSync(
    FileMonitorHandle dir,
    string_view file,
    IFileChangeNotify * notify
) {
    auto di = s_dirs.find(dir);
    di->removeMonitorSync(notify, file);
}

//===========================================================================
bool Dim::fileMonitorPath(
    string & out, 
    FileMonitorHandle dir, 
    string_view file
) {
    auto di = s_dirs.find(dir);
    if (!di)
        return false;

    string fullpath;
    return di->expandPath(fullpath, out, file);
}
