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
    void monitor_LK(
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
void ConfigFile::monitor_LK(
    string_view relpath,
    IAppConfigNotify * notify, 
    unique_lock<mutex> & lk
) {
    bool empty = m_notifiers.empty();
    NotifyInfo ni = { notify };
    m_notifiers.push_back(ni);
    lk.unlock();
    if (empty) {
        fileMonitor(s_hDir, relpath, this);
    } else {
        auto fp = fs::absolute(
            fs::u8path(string(relpath)), 
            fs::u8path(s_rootDir)
        );
        notify->onConfigChange(fp.u8string());
    }
    lk.lock();
}

//===========================================================================
void ConfigFile::onFileChange(string_view fullpath) {
    m_changes += 1;

    // load file

    // call notifiers
    NotifyInfo markerInfo = {};
    unique_lock<mutex> lk{s_mut};
    m_notifiers.push_front(markerInfo);
    auto marker = m_notifiers.begin();
    while (marker != m_notifiers.end()) {
        auto it = marker;
        ++it;
        auto notify = it->notify;
        m_notifiers.splice(it, m_notifiers, marker, marker);
        lk.unlock();
        notify->onConfigChange(fullpath);
        lk.lock();
    }
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
    s_hDir = fileMonitorDir(s_rootDir, true);
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
    cf.monitor_LK(path, notify, lk);
}

//===========================================================================
void Dim::appConfigChange(
    string_view file, 
    IAppConfigNotify * notify // = nullptr
) {
}
