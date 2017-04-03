// Copyright Glen Knowles 2017.
// Distributed under the Boost Software License, Version 1.0.
//
// appconfig.cpp - dim app
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

struct NotifyInfo {
    IAppConfigNotify * notify;
};

class ConfigFile : public IFileChangeNotify {
public:
    void monitor_UNLK(
        string_view file, 
        IAppConfigNotify * notify,
        unique_lock<mutex> & lk
    );
    void notify_UNLK(
        string_view file, 
        IAppConfigNotify * notify,
        unique_lock<mutex> & lk
    );

    // IFileChangeNotify
    void onFileChange(string_view path) override;

private:
    list<NotifyInfo> m_notifiers;
    unsigned m_changes{0};
};


/****************************************************************************
*
*   Variables
*
***/

static string s_rootDir;
static FileMonitorHandle s_hDir;

static mutex s_mut;
static unordered_map<string, ConfigFile> s_files;


/****************************************************************************
*
*   ConfigFile
*
***/

//===========================================================================
void ConfigFile::monitor_UNLK(
    string_view relpath, 
    IAppConfigNotify * notify,
    unique_lock<mutex> & lk
) {
    bool empty = m_notifiers.empty();
    NotifyInfo ni = { notify };
    m_notifiers.push_back(ni);

    if (empty) {
        lk.unlock();
        fileMonitor(s_hDir, relpath, this);
    } else {
        notify_UNLK(relpath, notify, lk);
    }
}

//===========================================================================
void ConfigFile::onFileChange(string_view fullpath) {
    m_changes += 1;

    // load file

    // call notifiers
    appConfigChange(fullpath, nullptr);
}

//===========================================================================
void ConfigFile::notify_UNLK(
    string_view relpath, 
    IAppConfigNotify * notify,
    unique_lock<mutex> & lk
) {
    auto fp = fs::absolute(
        fs::u8path(string(relpath)), 
        fs::u8path(s_rootDir)
    );
    auto fullpath = fp.u8string();

    for (auto it = m_notifiers.begin(); it != m_notifiers.end(); ++it) {
        if (!notify || notify == it->notify) {
            lk.unlock();
            it->notify->onConfigChange(fullpath);
            if (notify)
                return;
            lk.lock();
        }
    }
    lk.unlock();
}


/****************************************************************************
*
*   Shutdown
*
***/

namespace {

class Shutdown : public IShutdownNotify {
    void onShutdownConsole(bool retry) override;
};
static Shutdown s_cleanup;

//===========================================================================
void Shutdown::onShutdownConsole(bool retry) {
    fileMonitorStopSync(s_hDir);
}

} // namespace


/****************************************************************************
*
*   Internal API
*
***/

//===========================================================================
void Dim::iAppConfigInitialize (string_view dir) {
    shutdownMonitor(&s_cleanup);
    auto fp = fs::u8path(dir.begin(), dir.end());
    fp = fs::canonical(fp);
    s_rootDir = fp.u8string();
    fileMonitorDir(&s_hDir, s_rootDir, true);
}


/****************************************************************************
*
*   Public API
*
***/

//===========================================================================
void Dim::appConfigMonitor(string_view file, IAppConfigNotify * notify) {
    string path;
    if (!fileMonitorPath(path, s_hDir, file)) {
        logMsgError() << "File outside of config directory, " << file;
        return;
    }

    unique_lock<mutex> lk{s_mut};
    auto & cf = s_files[path];
    cf.monitor_UNLK(path, notify, lk);
}

//===========================================================================
void Dim::appConfigChange(
    string_view file, 
    IAppConfigNotify * notify // = nullptr
) {
    string path;
    if (!fileMonitorPath(path, s_hDir, file)) {
        logMsgError() << "File outside of config directory, " << file;
        return;
    }

    unique_lock<mutex> lk{s_mut};
    auto & cf = s_files[path];
    cf.notify_UNLK(path, notify, lk);
}
