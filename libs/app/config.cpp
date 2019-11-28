// Copyright Glen Knowles 2017 - 2019.
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

struct NotifyInfo : ListBaseLink<> {
    IConfigNotify * notify{};
};

class ConfigFile : public IFileChangeNotify {
public:
    ~ConfigFile();

    void monitor_UNLK(string_view relpath, IConfigNotify * notify);
    bool notify_UNLK(IConfigNotify * notify);
    bool closeWait_UNLK(IConfigNotify * notify);

    void write(IJBuilder * out);

    // IFileChangeNotify
    void onFileChange(string_view fullpath) override;

private:
    void parseContent(string_view fullpath, string && content);

    List<NotifyInfo> m_notifiers;
    unsigned m_changes{};
    TimePoint m_lastChanged{};

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
static unordered_map<Path, ConfigFile> s_files;
static thread::id s_inThread; // thread running any current callback
static condition_variable s_inCv; // when running callback completes
static IConfigNotify * s_inNotify; // notify currently in progress


/****************************************************************************
*
*   ConfigFile
*
***/

//===========================================================================
ConfigFile::~ConfigFile() {
    m_notifiers.clear();
}

//===========================================================================
void ConfigFile::monitor_UNLK(string_view relpath, IConfigNotify * notify) {
    bool wasEmpty = !m_notifiers;
    auto ni = new NotifyInfo;
    m_notifiers.link(ni);
    ni->notify = notify;

    if (wasEmpty) {
        s_mut.unlock();
        if (appFlags() & fAppWithFiles) {
            fileMonitor(s_hDir, relpath, this);
        } else {
            string fullpath;
            fullpath = appConfigDir();
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
    unique_lock lk{s_mut, adopt_lock};
    while (s_inNotify && s_inThread != this_thread::get_id())
        s_inCv.wait(lk);

    for (auto ni = m_notifiers.front(); ni; ni = m_notifiers.next(ni)) {
        if (notify == ni->notify) {
            delete ni;
            if (!m_notifiers)
                s_files.erase(Path(m_relpath));
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
    m_relpath.remove_prefix(appConfigDir().size() + 1);
    m_xml.parse(m_content.data(), m_relpath);
    m_relpath = m_xml.filename();
    m_fullpath = m_xml.heap().strdup(fullpath);

    // call notifiers
    configChange(m_fullpath, nullptr);
}

//===========================================================================
void ConfigFile::onFileChange(string_view fullpath) {
    m_changes += 1;
    m_lastChanged = timeNow();
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
    unique_lock lk{s_mut, adopt_lock};
    while (s_inNotify && s_inThread != this_thread::get_id())
        s_inCv.wait(lk);

    // Iterate by walking a marker through the list so that it stays valid
    // if monitors are added or removed while the change events are running.
    NotifyInfo marker;
    m_notifiers.linkFront(&marker);
    while (auto ni = m_notifiers.next(&marker)) {
        if (!notify || notify == ni->notify) {
            s_inThread = this_thread::get_id();
            s_inNotify = ni->notify;
            lk.unlock();
            found += 1;
            ni->notify->onConfigChange(m_xml);
            lk.lock();
            if (notify)
                break;
        }
        m_notifiers.link(&marker, ni);
    }
    m_notifiers.unlink(&marker);
    if (!m_notifiers)
        s_files.erase(Path(m_relpath));

    s_inThread = {};
    s_inNotify = nullptr;
    if (found) {
        lk.unlock();
        s_inCv.notify_all();
        return true;
    }
    return !notify;
}

//===========================================================================
void ConfigFile::write(IJBuilder * out) {
    out->object();
    out->member("path", m_relpath);
    out->member("lastChanged", m_lastChanged);
    out->member("changes", m_changes);
    out->end();
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
        fileMonitorDir(&s_hDir, appConfigDir(), true);
}


/****************************************************************************
*
*   Public API
*
***/

//===========================================================================
static bool getFullpath(Path * out, string_view file) {
    bool result;
    if (appFlags() & fAppWithFiles) {
        result = fileMonitorPath(out, s_hDir, file);
    } else {
        result = appConfigPath(out, file, false);
    }
    if (!result)
        logMsgError() << "File outside of configuration directory: " << file;
    return result;
}

//===========================================================================
void Dim::configMonitor(string_view file, IConfigNotify * notify) {
    Path path;
    if (!getFullpath(&path, file))
        return;

    s_mut.lock();
    auto & cf = s_files[path];
    cf.monitor_UNLK(path, notify);
}

//===========================================================================
void Dim::configCloseWait(string_view file, IConfigNotify * notify) {
    Path path;
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
    Path path;
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


/****************************************************************************
*
*   Debugging
*
***/

//===========================================================================
void Dim::configWriteRules(IJBuilder * out) {
    out->array();
    scoped_lock lk{s_mut};
    for (auto && kv : s_files)
        kv.second.write(out);
    out->end();
}

