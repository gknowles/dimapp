// Copyright Glen Knowles 2017 - 2023.
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
    IConfigNotify * notify {};
    unique_ptr<IConfigNotify> owner;
};

class ConfigFile : public IFileChangeNotify {
public:
    ~ConfigFile();

    void monitor_UNLK(
        unique_lock<mutex> && lk,
        string_view relpath,
        IConfigNotify * notify,
        bool takeOwnership
    );
    bool notify_UNLK(unique_lock<mutex> && lk, IConfigNotify * notify);
    bool closeWait_UNLK(unique_lock<mutex> && lk, IConfigNotify * notify);

    void reparse_LK();

    void write(IJBuilder * out, bool withContent);

    // IFileChangeNotify
    void onFileChange(string_view fullpath) override;

private:
    void parseContent(
        string_view fullpath,
        uint64_t bytes,
        TimePoint mtime,
        string && content
    );

    List<NotifyInfo> m_notifiers;
    unsigned m_changes = {};
    TimePoint m_lastChanged = {};

    string m_content;
    string m_parseSource;
    XDocument m_xml;
    string m_fullpath;
    string_view m_relpath;
    uint64_t m_bytes = 0;
    TimePoint m_mtime = {};
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
    IConfigNotify * notify,
    bool takeOwnersip
) {
    assert(lk);
    bool wasEmpty = !m_notifiers;
    auto ni = NEW(NotifyInfo);
    m_notifiers.link(ni);
    ni->notify = notify;
    if (takeOwnersip)
        ni->owner.reset(notify);

    if (!wasEmpty) {
        notify_UNLK(move(lk), notify);
        return;
    }

    lk.unlock();
    if (appFlags().any(fAppWithFiles)) {
        fileMonitor(s_hDir, relpath, this);
    } else {
        string fullpath;
        fullpath = appConfigDir();
        fullpath += '/';
        fullpath += relpath;
        parseContent(fullpath, 0, {}, {});
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
                s_files.erase(Path(m_fullpath));
            return true;
        }
    }

    // notify not found
    return false;
}

//===========================================================================
void ConfigFile::parseContent(
    string_view fullpath,
    uint64_t bytes,
    TimePoint mtime,
    string && content
) {
    m_bytes = bytes;
    m_mtime = mtime;
    m_content = move(content);
    m_fullpath = fullpath;
    m_relpath = m_fullpath;
    m_relpath.remove_prefix(appConfigDir().size() + 1);

    // call notifiers
    if (appFlags().any(fAppWithFiles))
        logMsgInfo() << "Config file '" << m_relpath << "' changed";
    configChange(m_fullpath, nullptr);
}

//===========================================================================
void ConfigFile::reparse_LK() {
    m_parseSource = m_content;
    m_xml.parse(m_parseSource.data(), m_relpath);
}

//===========================================================================
void ConfigFile::onFileChange(string_view fullpath) {
    using enum File::OpenMode;

    m_changes += 1;
    m_lastChanged = timeNow();
    string content;
    uint64_t bytes = 0;
    TimePoint mtime = {};
    for (;;) {
        FileHandle f;
        auto ec = fileOpen(&f, fullpath, fReadOnly | fDenyWrite);
        if (ec) {
            logMsgError() << "File open failed '" << fullpath << "': "
                << ec.message();
            break;
        }

        // load file
        fileLastWriteTime(&mtime, f);
        if (auto ec = fileSize(&bytes, f); ec)
            break;
        if (bytes > kMaxConfigFileSize) {
            logMsgError() << "File too large '" << fullpath << "': "
                << bytes << " bytes";
        } else if (bytes) {
            content.resize((size_t) bytes);
            if (auto ec = fileReadWait(
                nullptr,
                content.data(),
                content.size(),
                f,
                0
            ); ec) {
                content.clear();
            }
        }
        fileClose(f);
        break;
    }

    parseContent(fullpath, bytes, mtime, move(content));
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
        s_files.erase(Path(m_fullpath));

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
void ConfigFile::write(IJBuilder * out, bool withContent) {
    out->object();
    out->member("name", m_relpath);
    out->member("size", m_bytes);
    if (!empty(m_mtime))
        out->member("mtime", m_mtime);
    out->member("contentSize", size(m_content));
    out->member("lastChanged", m_lastChanged);
    out->member("changes", m_changes);
    out->member("notifiers", size(m_notifiers));
    if (withContent)
        out->member("content", m_parseSource);
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
    s_context.saddr = appAddress();
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
ConfigContext Dim::configDefaultContext() {
    return s_context;
}

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
static IConfigNotify * addMonitor(
    string_view file,
    IConfigNotify * notify,
    bool takeOwnership
) {
    Path path;
    if (!getFullpath(&path, file))
        return nullptr;

    unique_lock lk(s_mut);
    auto & cf = s_files[path];
    cf.monitor_UNLK(move(lk), path, notify, takeOwnership);
    return notify;
}

//===========================================================================
IConfigNotify * Dim::configMonitor(string_view file, IConfigNotify * notify) {
    return addMonitor(file, notify, false);
}

//===========================================================================
IConfigNotify * Dim::configMonitor(
    string_view file,
    unique_ptr<IConfigNotify> && notify
) {
    return addMonitor(file, notify.release(), true);
}

//===========================================================================
IConfigNotify * Dim::configMonitor(
    string_view file,
    function<void(const XDocument &)> fn
) {
    struct Notify : IConfigNotify {
        function<void(const XDocument &)> m_fn;

        void onConfigChange(const XDocument & doc) override {
            m_fn(doc);
        }
    };
    auto notify = make_unique<Notify>();
    notify->m_fn = move(fn);
    return configMonitor(file, move(notify));
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
    cf.reparse_LK();
    if (!cf.notify_UNLK(move(lk), notify))
        logMsgError() << "Change notify not registered, " << file;
}


/****************************************************************************
*
*   Public Query API
*
***/

namespace {
    enum MatchType {
        kInvalid,
        kName,
        kIndex,
        kConfig,
        kModule,
        kGroupType,
        kGroupIndex
    };
} // namespace

constexpr TokenTable::Token s_matchTypes[] = {
    { kName, "name" },
    { kIndex, "index" },
    { kConfig, "config" },
    { kModule, "module" },
    { kGroupType, "groupType" },
    { kGroupIndex, "groupIndex" },
};
const TokenTable s_matchTypeTbl(s_matchTypes);

//===========================================================================
static bool match(const XNode & node, const ConfigContext & context) {
    for (auto&& a : attrs(&node)) {
        auto key = s_matchTypeTbl.find(a.name, kInvalid);
        auto ival = strToUint(a.value);
        if (key == kName && context.appBaseName != a.value
            || key == kIndex && context.appIndex != ival
            || key == kConfig && context.config != a.value
            || key == kModule && context.module != a.value
            || key == kGroupType && context.groupType != a.value
            || key == kGroupIndex && context.groupIndex != ival
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
static bool evalAttrTemplate(
    string * out,
    const string & val,
    const ConfigContext & context
) {
    auto key = s_matchTypeTbl.find(val, kInvalid);
    switch (key) {
    case kName: *out = context.appBaseName; return true;
    case kIndex: *out = to_string(context.appIndex); return true;
    case kConfig: *out = context.config; return true;
    case kModule: *out = context.module; return true;
    case kGroupType: *out = context.groupType; return true;
    case kGroupIndex: *out = to_string(context.groupIndex); return true;
    case kInvalid:
        break;
    }
    return false;
}

//===========================================================================
static const char * elemToString(
    const ConfigContext & context,
    const XNode * elem,
    const char defVal[]
) {
    auto raw = attrValue(elem, "value");
    if (!raw)
        return defVal;
    auto val = attrValueSubst(
        raw,
        [&context](string * out, const string & val) {
            return evalAttrTemplate(out, val, context);
        }
    );
    if (val == raw)
        return raw;
    auto doc = const_cast<XDocument *>(document(elem));
    raw = doc->heap().strDup(val);
    return raw;
}

//===========================================================================
const char * Dim::configString(
    const ConfigContext & context,
    const XDocument & doc,
    string_view name,
    const char defVal[]
) {
    auto elem = configElement(context, doc, name);
    auto val = elemToString(context, elem, defVal);
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
static bool elemToBool(
    const ConfigContext & context,
    const XNode * elem,
    bool defVal
) {
    static const TokenTable::Token values[] = {
        { 1, "true" },
        { 1, "1" },
        { 0, "false" },
        { 0, "0" },
    };
    static const TokenTable tbl(values);

    if (auto str = elemToString(context, elem, nullptr))
        return tbl.find(str, false);
    return defVal;
}

//===========================================================================
bool Dim::configBool(
    const ConfigContext & context,
    const XDocument & doc,
    string_view name,
    bool defVal
) {
    auto elem = configElement(context, doc, name);
    auto val = elemToBool(context, elem, defVal);
    return val;
}

//===========================================================================
bool Dim::configBool(const XDocument & doc, string_view name, bool defVal) {
    return configBool(s_context, doc, name, defVal);
}

//===========================================================================
static double elemToNumber(
    const ConfigContext & context,
    const XNode * elem,
    double defVal
) {
    if (auto str = elemToString(context, elem, nullptr))
        (void) parse(&defVal, str);
    return defVal;
}

//===========================================================================
double Dim::configNumber(
    const ConfigContext & context,
    const XDocument & doc,
    string_view name,
    double defVal
) {
    auto elem = configElement(context, doc, name);
    auto val = elemToNumber(context, elem, defVal);
    return val;
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
    string_view name,
    Duration defVal
) {
    if (auto str = configString(context, doc, name, nullptr))
        (void) parse(&defVal, str);
    return defVal;
}

//===========================================================================
Duration Dim::configDuration(
    const XDocument & doc,
    string_view name,
    Duration defVal
) {
    return configDuration(s_context, doc, name, defVal);
}

//===========================================================================
vector<const char *> Dim::configStrings(
    const ConfigContext & context,
    const XDocument & doc,
    string_view name
) {
    vector<const char *> out;
    auto root = configElement(context, doc, name);
    if (auto str = elemToString(context, root, nullptr))
        out.push_back(str);
    for (auto&& elem : elems(root, "Value")) {
        if (!match(elem, context))
            continue;
        if (auto str = elemToString(context, &elem, nullptr))
            out.push_back(str);
    }
    return out;
}

//===========================================================================
vector<const char *> Dim::configStrings(
    const XDocument & doc,
    string_view name
) {
    return configStrings(s_context, doc, name);
}

//===========================================================================
vector<double> Dim::configNumbers(
    const ConfigContext & context,
    const XDocument & doc,
    string_view name
) {
    vector<double> out;
    for (auto&& str : configStrings(context, doc, name))
        out.push_back(strToUint(str));
    return out;
}

//===========================================================================
vector<double> Dim::configNumbers(
    const XDocument & doc,
    string_view name
) {
    return configNumbers(s_context, doc, name);
}


/****************************************************************************
*
*   Debugging
*
***/

//===========================================================================
void Dim::configWriteRules(IJBuilder * out, string_view member) {
    if (!member.empty())
        out->member(member);
    out->array();
    scoped_lock lk{s_mut};
    for (auto && kv : s_files)
        kv.second.write(out, false);
    out->end();
}


/****************************************************************************
*
*   JsonConfigs
*
***/

namespace {
class JsonConfigs : public IWebAdminNotify {
    void onHttpRequest(unsigned reqId, HttpRequest & msg) override;
};
} // namespace

//===========================================================================
void JsonConfigs::onHttpRequest(unsigned reqId, HttpRequest & msg) {
    auto res = HttpResponse(kHttpStatusOk);
    auto bld = initResponse(&res, reqId, msg);
    auto dir = appConfigDir();
    configWriteRules(&bld, "files");
    bld.end();
    httpRouteReply(reqId, move(res));
}


/****************************************************************************
*
*   JsonConfigFile
*
***/

namespace {
class JsonConfigFile : public IWebAdminNotify {
    void onHttpRequest(unsigned reqId, HttpRequest & msg) override;
};
} // namespace

//===========================================================================
void JsonConfigFile::onHttpRequest(unsigned reqId, HttpRequest & msg) {
    auto qpath = msg.query().path;
    auto prefix = httpRouteGetInfo(reqId).path.size();
    qpath.remove_prefix(prefix);
    Path path;
    if (!getFullpath(&path, qpath))
        return httpRouteReplyNotFound(reqId, msg);

    unique_lock lk(s_mut);
    auto & cf = s_files[path];
    auto res = HttpResponse(kHttpStatusOk);
    auto bld = initResponse(&res, reqId, msg);
    bld.member("file");
    cf.write(&bld, true);
    bld.end();
    lk.unlock();
    httpRouteReply(reqId, move(res));
}


static JsonConfigs s_jsonConfigs;
static JsonConfigFile s_jsonConfigFile;

//===========================================================================
void Dim::iConfigWebInitialize() {
    if (appFlags().any(fAppWithWebAdmin)) {
        httpRouteAdd({
            .notify = &s_jsonConfigs,
            .path = "/srv/file/configs.json",
        });
        httpRouteAdd({
            .notify = &s_jsonConfigFile,
            .path = "/srv/file/configs/",
            .recurse = true
        });
    }
}

