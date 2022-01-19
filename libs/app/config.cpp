// Copyright Glen Knowles 2017 - 2021.
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

struct NotifyInfo : ListLink<> {
    IConfigNotify * notify{};
};

class ConfigFile : public IFileChangeNotify {
public:
    ~ConfigFile();

    void monitor_UNLK(
        unique_lock<mutex> && lk, 
        string_view relpath, 
        IConfigNotify * notify
    );
    bool notify_UNLK(unique_lock<mutex> && lk, IConfigNotify * notify);
    bool closeWait_UNLK(unique_lock<mutex> && lk, IConfigNotify * notify);

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

static ConfigContext s_context;


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
void ConfigFile::monitor_UNLK(
    unique_lock<mutex> && lk, 
    string_view relpath, 
    IConfigNotify * notify
) {
    assert(lk);
    bool wasEmpty = !m_notifiers;
    auto ni = new NotifyInfo;
    m_notifiers.link(ni);
    ni->notify = notify;

    if (wasEmpty) {
        lk.unlock();
        if (appFlags().any(fAppWithFiles)) {
            fileMonitor(s_hDir, relpath, this);
        } else {
            string fullpath;
            fullpath = appConfigDir();
            fullpath += '/';
            fullpath += relpath;
            parseContent(fullpath, {});
        }
    } else {
        notify_UNLK(move(lk), notify);
    }
}

//===========================================================================
bool ConfigFile::closeWait_UNLK(
    unique_lock<mutex> && lk, 
    IConfigNotify * notify
) {
    assert(lk);
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
    m_fullpath = m_xml.heap().strDup(fullpath);

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
bool ConfigFile::notify_UNLK(
    unique_lock<mutex> && lk, 
    IConfigNotify * notify
) {
    assert(lk);
    unsigned found = 0;
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
void Dim::iConfigInitialize() {
    shutdownMonitor(&s_cleanup);
    vector<HostAddr> addrs;
    addressGetLocal(&addrs);
    s_context.saddr.addr = addrs[0];
    s_context.appBaseName = appBaseName();
    s_context.appIndex = appIndex();
    if (appFlags().any(fAppWithFiles))
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
    if (appFlags().any(fAppWithFiles)) {
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

    unique_lock lk(s_mut);
    auto & cf = s_files[path];
    cf.monitor_UNLK(move(lk), path, notify);
}

//===========================================================================
void Dim::configCloseWait(string_view file, IConfigNotify * notify) {
    Path path;
    if (!getFullpath(&path, file))
        return;

    unique_lock lk(s_mut);
    auto & cf = s_files[path];
    if (!cf.closeWait_UNLK(move(lk), notify))
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

    unique_lock lk(s_mut);
    auto & cf = s_files[path];
    if (!cf.notify_UNLK(move(lk), notify))
        logMsgError() << "Change notify not registered, " << file;
}


/****************************************************************************
*
*   Public Query API
*
***/

//===========================================================================
static bool match(const XNode & node, const ConfigContext & context) {
    enum { kInvalid, kName, kIndex, kConfig, kModule };
    constexpr TokenTable::Token keys[] = {
        { kName, "name" },
        { kIndex, "index" },
        { kConfig, "config" },
        { kModule, "module" },
    };
    static const TokenTable keyTbl(keys);

    for (auto&& a : attrs(&node)) {
        auto key = keyTbl.find(a.name, kInvalid);
        if (key == kName && context.appBaseName != a.value
            || key == kIndex && context.appIndex != strToUint(a.value)
            || key == kConfig && context.config != a.value
            || key == kModule && context.module != a.value
        ) {
            return false;
        }
    }
    return true;
}

//===========================================================================
static const XNode * find(
    const ConfigContext & context,
    const XNode * node,
    string_view name
) {
    for (auto&& e : elems(node, name)) {
        if (match(e, context))
            return &e;
    }
    for (auto&& f : elems(node, "Filter")) {
        auto found = match(f, context);
        if (!found) {
            for (auto&& m : elems(&f, "Match")) {
                if (match(m, context)) {
                    found = true;
                    break;
                }
            }
        }
        if (found) {
            if (auto out = find(context, &f, name))
                return out;
        }
    }
    return nullptr;
}

//===========================================================================
const XNode * Dim::configElement(
    const ConfigContext & context,
    const XDocument & doc,
    string_view name
) {
    return find(context, doc.root(), name);
}

//===========================================================================
const XNode * Dim::configElement(
    const XDocument & doc,
    string_view name
) {
    return configElement(s_context, doc, name);
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
    return configString(s_context, doc, name, defVal);
}

//===========================================================================
bool Dim::configBool(
    const ConfigContext & context,
    const XDocument & doc,
    string_view name,
    bool defVal
) {
    static const TokenTable::Token values[] = {
        { 1, "true" },
        { 1, "1" },
        { 0, "false" },
        { 0, "0" },
    };
    static const TokenTable tbl(values);
    if (auto str = configString(context, doc, name, nullptr)) 
        defVal = tbl.find(str, false);
    return defVal;
}

//===========================================================================
bool Dim::configBool(
    const XDocument & doc,
    string_view name,
    bool defVal
) {
    return configBool(s_context, doc, name, defVal);
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
    return configNumber(s_context, doc, name, defVal);
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
    return configDuration(s_context, doc, name, defVal);
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

