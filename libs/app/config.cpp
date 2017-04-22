// Copyright Glen Knowles 2017.
// Distributed under the Boost Software License, Version 1.0.
//
// config.cpp - dim app
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
    IConfigNotify * notify;
};

class ConfigFile : public IFileChangeNotify {
public:
    void monitor_UNLK(string_view relpath, IConfigNotify * notify);
    bool notify_UNLK(IConfigNotify * notify);

    // IFileChangeNotify
    void onFileChange(string_view fullpath, FileHandle f) override;

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
void ConfigFile::monitor_UNLK(string_view relpath, IConfigNotify * notify) {
    bool empty = m_notifiers.empty();
    NotifyInfo ni = { notify };
    m_notifiers.push_back(ni);

    if (empty) {
        s_mut.unlock();
        fileMonitor(s_hDir, relpath, this);
    } else {
        notify_UNLK(notify);
    }
}

//===========================================================================
void ConfigFile::onFileChange(string_view fullpath, FileHandle f) {
    m_changes += 1;

    // load file
    size_t bytes = fileSize(f);
    if (bytes > kMaxConfigFileSize) {
        logMsgError() << "File too large (" << bytes << " bytes): " 
            << fullpath;
        bytes = 0;
    }

    if (!bytes) {
        m_content.clear();
    } else {
        m_content.resize(bytes);
        fileReadWait(m_content.data(), m_content.size(), f, 0);
    }

    m_xml.parse(m_content.data());
    m_fullpath = m_xml.heap().strdup(fullpath);
    m_relpath = m_fullpath;
    m_relpath.remove_prefix(s_rootDir.size() + 1);

    // call notifiers
    configChange(m_fullpath, nullptr);
}

//===========================================================================
bool ConfigFile::notify_UNLK(IConfigNotify * notify) {
    unique_lock<mutex> lk{s_mut, adopt_lock};
    for (auto it = m_notifiers.begin(); it != m_notifiers.end(); ++it) {
        if (!notify || notify == it->notify) {
            lk.unlock();
            it->notify->onConfigChange(m_relpath, m_xml.root());
            if (notify)
                return true;
            lk.lock();
        }
    }
    return !notify;
}


/****************************************************************************
*
*   Shutdown
*
***/

namespace {
class ShutdownNotify : public IShutdownNotify {
    void onShutdownConsole(bool retry) override;
};
} // namespace
static ShutdownNotify s_cleanup;

//===========================================================================
void ShutdownNotify::onShutdownConsole(bool retry) {
    fileMonitorCloseWait(s_hDir);
}


/****************************************************************************
*
*   Internal API
*
***/

//===========================================================================
void Dim::iConfigInitialize (string_view dir) {
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
void Dim::configMonitor(string_view file, IConfigNotify * notify) {
    string path;
    if (!fileMonitorPath(path, s_hDir, file)) {
        logMsgError() << "Monitor file outside of config directory, " << file;
        return;
    }

    s_mut.lock();
    auto & cf = s_files[path];
    cf.monitor_UNLK(path, notify);
}

//===========================================================================
void Dim::configChange(
    string_view file, 
    IConfigNotify * notify // = nullptr
) {
    string path;
    if (!fileMonitorPath(path, s_hDir, file)) {
        logMsgError() << "Change file outside of config directory, " << file;
        return;
    }

    s_mut.lock();
    auto & cf = s_files[path];
    if (!cf.notify_UNLK(notify))
        logMsgError() << "Change notify not registered, " << file;
}
