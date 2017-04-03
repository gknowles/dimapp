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
    list<IFileChangeNotify *> notifiers;
};

class DirInfo : public ITaskNotify, public ITimerNotify {
public:
    DirInfo(IFileChangeNotify * notify);
    ~DirInfo();

    bool start(string_view path, bool recurse);
    void stopSync_LK(FileMonitorHandle dir);

    void addMonitor_LK(IFileChangeNotify * notify, string_view file);
    void removeMonitorSync_LK(IFileChangeNotify * notify, string_view file);

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
    WinOverlappedEvent m_evt;

    unordered_map<string, FileInfo> m_files;
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
static HandleMap<FileMonitorHandle, DirInfo> s_stopping;
static thread::id s_inThread; // thread running any current callback
static condition_variable s_inCv; // when running callback completes
static IFileChangeNotify * s_inNotify; // notify currently in progress
static bool s_inSyncNotify; // notify called from inside addMonitor


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
    if (m_handle != INVALID_HANDLE_VALUE && !CloseHandle(m_handle)) {
        WinError err;
        logMsgError() << "CloseHandle(hDir), " << m_base << ", " << err;
    }
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
    m_handle = CreateFile(
        m_base.c_str(), 
        FILE_LIST_DIRECTORY, // access
        FILE_SHARE_READ | FILE_SHARE_WRITE, // share mode
        NULL, 
        OPEN_EXISTING, 
        FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED,
        NULL // template 
    );
    if (m_handle == INVALID_HANDLE_VALUE) {
        WinError err;
        logMsgError() << "CreateFile(FILE_LIST_DIRECTORY), " << m_base 
            << ", " << err;
        iFileSetErrno(err);
        return false;
    }
    if (!winIocpBindHandle(m_handle)) {
        CloseHandle(m_handle);
        iFileSetErrno(WinError{});
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
void DirInfo::stopSync_LK (FileMonitorHandle dir) {
    if (m_handle == INVALID_HANDLE_VALUE)
        return;

    unique_lock<mutex> lk{s_mut, adopt_lock};
    while (s_inNotify && s_inThread != this_thread::get_id())
        s_inCv.wait(lk);

    s_dirs.release(dir);
    m_stopping = s_stopping.insert(this);

    lk.unlock();
    if (!CancelIoEx(m_handle, &m_evt.overlapped)) {
        WinError err;
        logMsgError() << "CancelIoEx(hDir), " << m_base << ", " << err;
    }
    lk.lock();
    lk.release();
}

//===========================================================================
void DirInfo::addMonitor_LK(IFileChangeNotify * notify, string_view file) {
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

    // call the notify unless we're in a notify
    unique_lock<mutex> lk{s_mut, adopt_lock};
    while (s_inSyncNotify && s_inThread != this_thread::get_id())
        s_inCv.wait(lk);

    if (s_inSyncNotify) {
        lk.unlock();
        notify->onFileChange(fullpath);
        lk.lock();
    } else {
        s_inThread = this_thread::get_id();
        s_inNotify = notify;
        s_inSyncNotify = true;
        lk.unlock();
        notify->onFileChange(fullpath);
        lk.lock();
        s_inThread = {};
        s_inNotify = nullptr;
        s_inSyncNotify = false;
        s_inCv.notify_all();
    }

    lk.release();
}

//===========================================================================
void DirInfo::removeMonitorSync_LK(
    IFileChangeNotify * notify, 
    string_view file
) {
    string fullpath;
    string relpath;
    if (!expandPath(fullpath, relpath, file))
        return;

    unique_lock<mutex> lk{s_mut, adopt_lock};
    while (notify == s_inNotify && s_inThread != this_thread::get_id())
        s_inCv.wait(lk);
    lk.release();

    auto & fi = m_files[relpath];
    for (auto i = fi.notifiers.begin(), e = fi.notifiers.end(); i != e; ++i) {
        if (*i == notify) {
            fi.notifiers.erase(i);
            if (fi.notifiers.empty())
                timerUpdate(this, 5s, true);
            return;
        }
    }
}

//===========================================================================
void DirInfo::onTask () {
    DWORD bytes;
    WinError err{0};
    if (!GetOverlappedResult(NULL, &m_evt.overlapped, &bytes, false)) {
        err = WinError{};
        if (err != ERROR_NOTIFY_ENUM_DIR) {
            if (err != ERROR_OPERATION_ABORTED) {
                logMsgError() << "ReadDirectoryChangesW() overlapped, " 
                    << m_base << ", " << err;
            }
            m_handle = INVALID_HANDLE_VALUE;
        }
    }

    unique_lock<mutex> lk{s_mut};
    if (m_stopping) {
        s_stopping.erase(m_stopping);
        return;
    }
    lk.unlock();

    if (m_handle == INVALID_HANDLE_VALUE)
        return;

    queue();
    timerUpdate(this, 5s);
}

//===========================================================================
Duration DirInfo::onTimer (TimePoint now) {
    unique_lock<mutex> lk{s_mut};
    for (auto && kv : m_files) {
        auto ntfs = kv.second.notifiers;
        // Iterate through the list of notifiers by adding a marker that can't 
        // be externally removed to front of the list and advancing it until it 
        // reaches the end. This allows onFileChange() notifiers to safely 
        // modify the list.
        ntfs.push_front({});
        auto marker = ntfs.begin();
        for (;;) {
            auto it = marker;
            if (++it == ntfs.end())
                break;
            auto notify = *it;
            ntfs.splice(it, ntfs, marker, marker);
            lk.unlock();
            notify->onFileChange(kv.first);
            lk.lock();
        }
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
*   Internal API
*
***/

namespace {

class Shutdown : public IShutdownNotify {
    void onShutdownConsole(bool retry) override;
};
static Shutdown s_cleanup;

//===========================================================================
void Shutdown::onShutdownConsole(bool retry) {
    lock_guard<mutex> lk{s_mut};
    if (!s_dirs.empty() || !s_stopping.empty())
        return shutdownIncomplete();
}

} // namespace

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
        lock_guard<mutex> lk{s_mut};
        h = s_dirs.insert(hostage.release());
        success = true;
    }
    if (handle)
        *handle = h;
    return success;
}

//===========================================================================
void Dim::fileMonitorStopSync(FileMonitorHandle dir) {
    lock_guard<mutex> lk{s_mut};
    auto di = s_dirs.find(dir);
    assert(di);
    di->stopSync_LK(dir);
}

//===========================================================================
string_view Dim::fileMonitorPath(FileMonitorHandle dir) {
    lock_guard<mutex> lk{s_mut};
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
    lock_guard<mutex> lk{s_mut};
    auto di = s_dirs.find(dir);
    assert(di);
    di->addMonitor_LK(notify, file);
}

//===========================================================================
void Dim::fileMonitorStopSync(
    FileMonitorHandle dir,
    string_view file,
    IFileChangeNotify * notify
) {
    assert(notify);
    lock_guard<mutex> lk{s_mut};
    auto di = s_dirs.find(dir);
    assert(di);
    di->removeMonitorSync_LK(notify, file);
}

//===========================================================================
bool Dim::fileMonitorPath(
    string & out, 
    FileMonitorHandle dir, 
    string_view file
) {
    lock_guard<mutex> lk{s_mut};
    auto di = s_dirs.find(dir);
    if (!di)
        return false;

    string fullpath;
    return di->expandPath(fullpath, out, file);
}
