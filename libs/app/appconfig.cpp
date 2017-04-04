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
*   Tuning parameters
*
***/

const unsigned kMaxConfigFileSize = 10'000'000;


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
        string_view relpath, 
        IAppConfigNotify * notify,
        unique_lock<mutex> & lk
    );
    void notify_UNLK(
        IAppConfigNotify * notify,
        unique_lock<mutex> & lk
    );

    // IFileChangeNotify
    void onFileChange(string_view fullpath, IFile * file) override;

private:
    list<NotifyInfo> m_notifiers;
    unsigned m_changes{0};

    string m_content;
    XDocument m_xml;
    string_view m_fullpath;
    string_view m_relpath;
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
        notify_UNLK(notify, lk);
    }
}

//===========================================================================
void ConfigFile::onFileChange(string_view fullpath, IFile * file) {
    m_changes += 1;

    // load file
    size_t bytes = fileSize(file);
    if (bytes > kMaxConfigFileSize) {
        logMsgError() << "File too large (" << bytes << " bytes): " 
            << fullpath;
        bytes = 0;
    }

    if (!bytes) {
        m_content.clear();
    } else {
        m_content.resize(bytes);
        fileReadSync(m_content.data(), m_content.size(), file, 0);
    }

    m_xml.parse(m_content.data());
    m_fullpath = m_xml.heap().strdup(fullpath);
    m_relpath = m_fullpath;
    m_relpath.remove_prefix(s_rootDir.size() + 1);

    // call notifiers
    appConfigChange(m_fullpath, nullptr);
}

//===========================================================================
void ConfigFile::notify_UNLK(
    IAppConfigNotify * notify,
    unique_lock<mutex> & lk
) {
    for (auto it = m_notifiers.begin(); it != m_notifiers.end(); ++it) {
        if (!notify || notify == it->notify) {
            lk.unlock();
            it->notify->onConfigChange(m_relpath, m_xml.root());
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
    cf.notify_UNLK(notify, lk);
}
