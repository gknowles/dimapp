// Copyright Glen Knowles 2017 - 2018.
// Distributed under the Boost Software License, Version 1.0.
//
// config.cpp - dim app
#include "pch.h"
#pragma hdrstop

using namespace std;
using namespace Dim;


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

namespace {

struct NotifyInfo {
    IConfigNotify * notify;
};

class ConfigFile : public IFileChangeNotify {
public:
    void monitor_UNLK(string_view relpath, IConfigNotify * notify);
    bool notify_UNLK(IConfigNotify * notify);
    bool closeWait_UNLK(IConfigNotify * notify);

    // IFileChangeNotify
    void onFileChange(string_view fullpath) override;

private:
    void parseContent(string_view fullpath, string && content);

    list<NotifyInfo> m_notifiers;
    unsigned m_changes{0};

    string m_content;
    XDocument m_xml;
    string_view m_fullpath;
    string_view m_relpath;
};

} // namespace


/****************************************************************************
*
*   Variables
*
***/

static FileMonitorHandle s_hDir;

static mutex s_mut;
static unordered_map<string, ConfigFile> s_files;
static thread::id s_inThread; // thread running any current callback
static condition_variable s_inCv; // when running callback completes
static IConfigNotify * s_inNotify; // notify currently in progress


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
        if (appFlags() & fAppWithFiles) {
            fileMonitor(s_hDir, relpath, this);
        } else {
            string fullpath;
            fullpath = appConfigDirectory();
            fullpath += '/';
            fullpath += relpath;
            parseContent(fullpath, {});
        }
    } else {
        notify_UNLK(notify);
    }
}

//===========================================================================
bool ConfigFile::closeWait_UNLK(IConfigNotify * notify) {
    unique_lock<mutex> lk{s_mut, adopt_lock};
    while (s_inNotify && s_inThread != this_thread::get_id())
        s_inCv.wait(lk);

    for (auto it = m_notifiers.begin(); it != m_notifiers.end(); ++it) {
        if (notify == it->notify) {
            m_notifiers.erase(it);
            if (m_notifiers.empty())
                s_files.erase(string(m_relpath));
            return true;
        }
    }

    // notify not found
    return false;
}

//===========================================================================
void ConfigFile::parseContent(string_view fullpath, string && content) {
    m_content = move(content);
    m_relpath = fullpath;
    m_relpath.remove_prefix(appConfigDirectory().size() + 1);
    m_xml.parse(m_content.data(), m_relpath);
    m_relpath = m_xml.filename();
    m_fullpath = m_xml.heap().strdup(fullpath);

    // call notifiers
    configChange(m_fullpath, nullptr);
}

//===========================================================================
void ConfigFile::onFileChange(string_view fullpath) {
    m_changes += 1;
    auto f = fileOpen(fullpath, File::fReadOnly | File::fDenyWrite);

    // load file
    auto bytes = fileSize(f);
    if (bytes > kMaxConfigFileSize) {
        logMsgError() << "File too large (" << bytes << " bytes): "
            << fullpath;
        bytes = 0;
    }

    string content;
    if (bytes) {
        content.resize((size_t) bytes);
        fileReadWait(content.data(), content.size(), f, 0);
    }
    fileClose(f);

    parseContent(fullpath, move(content));
}

//===========================================================================
bool ConfigFile::notify_UNLK(IConfigNotify * notify) {
    unsigned found = 0;
    unique_lock<mutex> lk{s_mut, adopt_lock};
    while (s_inNotify && s_inThread != this_thread::get_id())
        s_inCv.wait(lk);

    for (auto it = m_notifiers.begin(); it != m_notifiers.end(); ++it) {
        if (!notify || notify == it->notify) {
            s_inThread = this_thread::get_id();
            s_inNotify = it->notify;
            lk.unlock();
            found += 1;
            it->notify->onConfigChange(m_xml);
            lk.lock();
            if (notify)
                break;
        }
    }
    s_inThread = {};
    s_inNotify = nullptr;
    if (found) {
        lk.unlock();
        s_inCv.notify_all();
        return true;
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
    void onShutdownConsole(bool firstTry) override;
};
} // namespace
static ShutdownNotify s_cleanup;

//===========================================================================
void ShutdownNotify::onShutdownConsole(bool firstTry) {
    if (s_hDir)
        fileMonitorCloseWait(s_hDir);
}


/****************************************************************************
*
*   Internal API
*
***/

//===========================================================================
void Dim::iConfigInitialize () {
    shutdownMonitor(&s_cleanup);
    if (appFlags() & fAppWithFiles)
        fileMonitorDir(&s_hDir, appConfigDirectory(), true);
}


/****************************************************************************
*
*   Public API
*
***/

//===========================================================================
static bool getFullpath(string * out, string_view file) {
    bool result;
    if (appFlags() & fAppWithFiles) {
        result = fileMonitorPath(out, s_hDir, file);
    } else {
        result = appConfigPath(out, file, false);
    }
    if (!result)
        logMsgError() << "File outside of config directory: " << file;
    return result;
}

//===========================================================================
void Dim::configMonitor(string_view file, IConfigNotify * notify) {
    string path;
    if (!getFullpath(&path, file))
        return;

    s_mut.lock();
    auto & cf = s_files[path];
    cf.monitor_UNLK(path, notify);
}

//===========================================================================
void Dim::configCloseWait(string_view file, IConfigNotify * notify) {
    string path;
    if (!getFullpath(&path, file))
        return;

    s_mut.lock();
    auto & cf = s_files[path];
    if (!cf.closeWait_UNLK(notify))
        logMsgError() << "Close notify not registered, " << file;
}

//===========================================================================
void Dim::configChange(
    string_view file,
    IConfigNotify * notify // = nullptr
) {
    string path;
    if (!getFullpath(&path, file))
        return;

    s_mut.lock();
    auto & cf = s_files[path];
    if (!cf.notify_UNLK(notify))
        logMsgError() << "Change notify not registered, " << file;
}


/****************************************************************************
*
*   Public Query API
*
***/

//===========================================================================
const XNode * Dim::configElement(
    const ConfigContext & context,
    const XDocument & doc,
    string_view name
) {
    return firstChild(doc.root(), name);
}

//===========================================================================
const XNode * Dim::configElement(
    const XDocument & doc,
    string_view name
) {
    ConfigContext context;
    return configElement(context, doc, name);
}

//===========================================================================
const char * Dim::configString(
    const ConfigContext & context,
    const XDocument & doc,
    string_view name,
    const char defVal[]
) {
    auto elem = configElement(context, doc, name);
    auto val = attrValue(elem, "value", defVal);
    return val;
}

//===========================================================================
const char * Dim::configString(
    const XDocument & doc,
    string_view name,
    const char defVal[]
) {
    ConfigContext context;
    return configString(context, doc, name, defVal);
}

//===========================================================================
double Dim::configNumber(
    const ConfigContext & context,
    const XDocument & doc,
    string_view name,
    double defVal
) {
    if (auto str = configString(context, doc, name, nullptr))
        (void) parse(&defVal, str);
    return defVal;
}

//===========================================================================
double Dim::configNumber(
    const XDocument & doc,
    string_view name,
    double defVal
) {
    ConfigContext context;
    return configNumber(context, doc, name, defVal);
}

//===========================================================================
Duration Dim::configDuration(
    const ConfigContext & context,
    const XDocument & doc,
    std::string_view name,
    Duration defVal
) {
    if (auto str = configString(context, doc, name, nullptr))
        (void) parse(&defVal, str);
    return defVal;
}

//===========================================================================
Duration Dim::configDuration(
    const XDocument & doc,
    std::string_view name,
    Duration defVal
) {
    ConfigContext context;
    return configDuration(context, doc, name, defVal);
}
