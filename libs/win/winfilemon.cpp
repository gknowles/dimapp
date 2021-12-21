// Copyright Glen Knowles 2017 - 2021.
// Distributed under the Boost Software License, Version 1.0.
//
// winfilemon.cpp - dim windows platform
#include "pch.h"
#pragma hdrstop

using namespace std;
using namespace Dim;


/****************************************************************************
*
*   Private
*
***/

namespace {

struct FileInfo {
    TimePoint mtime;
    list<IFileChangeNotify *> notifiers;
};

class DirInfo
    : public IWinOverlappedNotify
    , public ITimerNotify
    , public HandleContent
{
public:
    explicit DirInfo(IFileChangeNotify * notify);
    ~DirInfo();

    bool start(string_view path, bool recurse);
    void closeWait_UNLK(unique_lock<mutex> && lk, FileMonitorHandle dir);

    void addMonitor_UNLK(
        unique_lock<mutex> && lk, 
        string_view file, 
        IFileChangeNotify * notify
    );
    void removeMonitorWait_UNLK(
        unique_lock<mutex> && lk, 
        string_view file, 
        IFileChangeNotify * notify
    );

    string_view base() const { return m_base; }
    // returns false if file is outside of base directory
    bool expandPath(
        Path * fullpath,
        Path * relpath,
        string_view file
    ) const;

    void onTask() override;
    Duration onTimer(TimePoint now) override;

private:
    bool queue();

    Path m_base;
    bool m_recurse{false}; // monitoring includes child directories?
    IFileChangeNotify * m_notify{};

    unordered_map<Path, FileInfo> m_files;
    HANDLE m_handle{INVALID_HANDLE_VALUE};
    FileMonitorHandle m_stopping;
};

} // namespace


/****************************************************************************
*
*   Variables
*
***/

static mutex s_mut;
static HandleMap<FileMonitorHandle, DirInfo> s_dirs;
static HandleMap<FileMonitorHandle, DirInfo> s_stoppingDirs;
static thread::id s_inThread; // thread running any current callback
static condition_variable s_inCv; // when running callback completes
static IFileChangeNotify * s_inNotify; // notify currently in progress
static bool s_inAddMonitor; // notify called from inside addMonitor


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
DirInfo::~DirInfo() {
    if (m_handle != INVALID_HANDLE_VALUE && !CloseHandle(m_handle)) {
        WinError err;
        logMsgError() << "CloseHandle(hDir): " << m_base << ", " << err;
    }
}

//===========================================================================
bool DirInfo::start(string_view path, bool recurse) {
    m_base = fileAbsolutePath(path);
    m_recurse = recurse;
    fileCreateDirs(m_base);

    auto wpath = toWstring(m_base);
    m_handle = CreateFileW(
        wpath.c_str(),
        FILE_LIST_DIRECTORY, // access
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        NULL, // security attributes
        OPEN_EXISTING,
        FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED,
        NULL // template
    );
    if (m_handle == INVALID_HANDLE_VALUE) {
        WinError err;
        logMsgError() << "CreateFile(FILE_LIST_DIRECTORY): " << m_base
            << ", " << err;
        winFileSetErrno(err);
        return false;
    }
    if (!winIocpBindHandle(m_handle)) {
        CloseHandle(m_handle);
        winFileSetErrno(WinError{});
        m_handle = INVALID_HANDLE_VALUE;
        return false;
    }

    return queue();
}

//===========================================================================
bool DirInfo::queue() {
    if (!ReadDirectoryChangesW(
        m_handle,
        NULL, // output buffer
        0, // output buffer length
        m_recurse, // include subdirs
        FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_LAST_WRITE,
        NULL, // bytes returned (not for async)
        &overlapped(),
        NULL // completion routine
    )) {
        WinError err;
        logMsgError() << "ReadDirectoryChangesW(): " << m_base << ", " << err;
        return false;
    }

    return true;
}

//===========================================================================
void DirInfo::closeWait_UNLK(unique_lock<mutex> && lk, FileMonitorHandle dir) {
    assert(lk);

    while (s_inNotify && s_inThread != this_thread::get_id())
        s_inCv.wait(lk);

    if (m_handle == INVALID_HANDLE_VALUE)
        return;

    s_dirs.release(dir);
    m_stopping = s_stoppingDirs.insert(this);

    if (!CancelIoEx(m_handle, &overlapped())) {
        WinError err;
        logMsgError() << "CancelIoEx(hDir): " << m_base << ", " << err;
    }
}

//===========================================================================
void DirInfo::addMonitor_UNLK(
    unique_lock<mutex> && lk, 
    string_view path, 
    IFileChangeNotify * notify
) {
    assert(lk);

    Path fullpath;
    Path relpath;
    if (!expandPath(&fullpath, &relpath, path)) {
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
    fileSize(fullpath);
    if (fi.notifiers.empty())
        fi.mtime = fileLastWriteTime(fullpath);
    fi.notifiers.push_back(notify);

    // call the notify unless we're in a notify
    while (s_inNotify && s_inThread != this_thread::get_id())
        s_inCv.wait(lk);

    if (s_inAddMonitor) {
        // In recursive addMonitor call from onFileChange handler. No need to
        // set the s_in* variables because they're already set and don't clear
        // them on exit - because we're still in the prior addMonitor call.
        lk.unlock();
        notify->onFileChange(fullpath);
        return;
    }

    s_inThread = this_thread::get_id();
    s_inNotify = notify;
    s_inAddMonitor = true;
    lk.unlock();
    notify->onFileChange(fullpath);
    lk.lock();
    s_inThread = {};
    s_inNotify = nullptr;
    s_inAddMonitor = false;
    lk.unlock();
    s_inCv.notify_all();
}

//===========================================================================
void DirInfo::removeMonitorWait_UNLK(
    unique_lock<mutex> && lk, 
    string_view file,
    IFileChangeNotify * notify
) {
    assert(lk);

    Path fullpath;
    Path relpath;
    if (!expandPath(&fullpath, &relpath, file))
        return;

    while (notify == s_inNotify && s_inThread != this_thread::get_id())
        s_inCv.wait(lk);

    auto & fi = m_files[relpath];
    for (auto i = fi.notifiers.begin(), e = fi.notifiers.end(); i != e; ++i) {
        if (*i == notify) {
            fi.notifiers.erase(i);
            if (fi.notifiers.empty()) {
                lk.unlock();
                lk.release();
                // Queue the timer event, which will notice the empty FileInfo
                // and delete it.
                timerUpdate(this, 5s, true);
            }
            return;
        }
    }
}

//===========================================================================
void DirInfo::onTask() {
    auto close = false;
    if (auto err = decodeOverlappedResult().err) {
        if (err != ERROR_NOTIFY_ENUM_DIR) {
            if (err != ERROR_OPERATION_ABORTED) {
                logMsgError() << "ReadDirectoryChangesW() overlapped: "
                    << m_base << ", " << err;
            }
            close = true;
        }
    }

    unique_lock lk{s_mut};
    if (close && m_handle != INVALID_HANDLE_VALUE) {
        CloseHandle(m_handle);
        m_handle = INVALID_HANDLE_VALUE;
    }
    if (m_stopping) {
        s_stoppingDirs.erase(m_stopping);
        return;
    }
    if (m_handle == INVALID_HANDLE_VALUE)
        return;
    lk.unlock();

    queue();
    timerUpdate(this, 5s);
    if (m_notify)
        m_notify->onFileChange(m_base);
}

//===========================================================================
Duration DirInfo::onTimer(TimePoint now) {
    Path fullpath;
    Path relpath;
    unsigned notified = 0;

    unique_lock lk{s_mut};
    for (auto && kv : m_files) {
        expandPath(&fullpath, &relpath, kv.first);
        auto mtime = fileLastWriteTime(fullpath);
        if (mtime == kv.second.mtime)
            continue;
        kv.second.mtime = mtime;

        while (s_inNotify)
            s_inCv.wait(lk);

        // Iterate through the list of notifiers by adding a marker that can't
        // be externally removed to front of the list and advancing it until it
        // reaches the end. This allows onFileChange() notifiers to safely
        // modify the list.
        auto ntfs = kv.second.notifiers;
        ntfs.push_front({});
        auto marker = ntfs.begin();
        for (;;) {
            auto it = marker;
            if (++it == ntfs.end())
                break;
            auto notify = *it;
            ntfs.splice(marker, ntfs, it);
            s_inThread = this_thread::get_id();
            s_inNotify = notify;
            lk.unlock();
            notified += 1;
            notify->onFileChange(fullpath);
            lk.lock();
        }
        s_inThread = {};
        s_inNotify = nullptr;
        ntfs.pop_back();
    }

    // Removals of files aren't allowed during the callbacks, just their
    // notifiers. So we make a pass to delete the file entries that no one
    // cares about anymore.
    auto next = m_files.begin(),
        e = m_files.end();
    while (next != e) {
        auto it = next++;
        if (it->second.notifiers.empty())
            m_files.erase(it);
    }

    if (notified) {
        lk.unlock();
        s_inCv.notify_all();
    }
    return kTimerInfinite;
}

//===========================================================================
bool DirInfo::expandPath(
    Path * fullpath,
    Path * relpath,
    string_view file
) const {
    *fullpath = Path{file}.resolve(m_base);
    if (fullpath->str().compare(0, m_base.str().size(), m_base.str()) != 0) {
        relpath->clear();
        return false;
    }

    *relpath = string(fullpath->str()).erase(0, m_base.str().size() + 1);
    return true;
}


/****************************************************************************
*
*   Internal API
*
***/

namespace {

class ShutdownNotify : public IShutdownNotify {
    void onShutdownConsole(bool firstTry) override;
};

} // namespace

static ShutdownNotify s_cleanup;

//===========================================================================
void ShutdownNotify::onShutdownConsole(bool firstTry) {
    scoped_lock lk{s_mut};
    if (s_dirs || s_stoppingDirs)
        return shutdownIncomplete();
}

//===========================================================================
void Dim::winFileMonitorInitialize() {
    shutdownMonitor(&s_cleanup);
}


/****************************************************************************
*
*   Public API
*
***/

//===========================================================================
bool Dim::fileMonitorDir(
    FileMonitorHandle * handle,
    string_view dir,
    bool recurse,
    IFileChangeNotify * notify
) {
    bool success = false;
    FileMonitorHandle h;
    auto hostage = make_unique<DirInfo>(notify);
    if (hostage->start(dir, recurse)) {
        scoped_lock lk{s_mut};
        h = s_dirs.insert(hostage.release());
        success = true;
    }
    if (handle)
        *handle = h;
    return success;
}

//===========================================================================
void Dim::fileMonitorCloseWait(FileMonitorHandle dir) {
    unique_lock lk(s_mut);
    auto di = s_dirs.find(dir);
    assert(di);
    di->closeWait_UNLK(move(lk), dir);
}

//===========================================================================
string_view Dim::fileMonitorPath(FileMonitorHandle dir) {
    scoped_lock lk{s_mut};
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
    assert(notify);
    unique_lock lk(s_mut);
    auto di = s_dirs.find(dir);
    assert(di);
    di->addMonitor_UNLK(move(lk), file, notify);
}

//===========================================================================
void Dim::fileMonitorCloseWait(
    FileMonitorHandle dir,
    string_view file,
    IFileChangeNotify * notify
) {
    assert(notify);
    unique_lock lk(s_mut);
    auto di = s_dirs.find(dir);
    assert(di);
    di->removeMonitorWait_UNLK(move(lk), file, notify);
}

//===========================================================================
bool Dim::fileMonitorPath(
    Path * out,
    FileMonitorHandle dir,
    string_view file
) {
    scoped_lock lk{s_mut};
    auto di = s_dirs.find(dir);
    if (!di)
        return false;

    Path fullpath;
    return di->expandPath(&fullpath, out, file);
}
