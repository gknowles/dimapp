// Copyright Glen Knowles 2017 - 2023.
// Distributed under the Boost Software License, Version 1.0.
//
// webadmin.cpp - dim app
#include "pch.h"
#pragma hdrstop

using namespace std;
using namespace Dim;


/****************************************************************************
*
*   Variables
*
***/

static vector<Path> s_webRoots;
static unordered_map<string, string> s_appData;


/****************************************************************************
*
*   Helpers
*
***/

//===========================================================================
static void addRoute(
    IJBuilder * out,
    const HttpRouteInfo & ri,
    bool active = false
) {
    out->object();
    out->member("path", ri.path);
    if (ri.recurse)
        out->member("recurse", ri.recurse);
    out->member("implemented", ri.notify || !ri.renderPath.empty());
    out->member("methods");
    out->array();
    for (auto mname : toViews(ri.methods))
        out->value(mname);
    out->end();
    if (!ri.name.empty())
        out->member("name", ri.name);
    if (!ri.desc.empty())
        out->member("desc", ri.desc);
    if (!ri.renderPath.empty())
        out->member("renderPath", ri.renderPath);
    out->member("matched", ri.matched);
    if (ri.matched)
        out->member("lastMatched", ri.lastMatched);
    if (active)
        out->member("active", true);
    out->end();
}


/****************************************************************************
*
*   IWebAdminNotify
*
***/

//===========================================================================
JBuilder IWebAdminNotify::initResponse(
    HttpResponse * res,
    unsigned reqId,
    const HttpRequest & msg
) {
    if (m_jsVar) {
        res->addHeader(kHttpContentType, "text/javascript");
        res->body().append("const ").append(*m_jsVar).append(" = ");
    } else {
        res->addHeader(kHttpContentType, "application/json");
    }
    auto ri = httpRouteGetInfo(reqId);
    auto root = Path(msg.query().path);
    auto segs = count(ri.path.begin(), ri.path.end(), '/');
    for (auto i = 0; i < segs; ++i)
        root.removeFilename();
    auto relPath = ri.path.substr(root.str().size() - 1);
    JBuilder bld(&res->body());
    bld.object().member("server").object();
    bld.member("productName", appBaseName())
        .member("appIndex", appIndex())
        .member("version", toString(appVersion()))
        .member("buildTime", Time8601Str(envExecBuildTime()).view())
        .member("startTime", Time8601Str(envProcessStartTime()).view())
        .member("address", toString(appAddress()));
    bld.member("groupIndex", appGroupIndex())
        .member("groupType", appGroupType());
    if (appFlags().any(fAppIsService))
        bld.member("service", true);
    bld.end();
    bld.member("now", Time8601Str(timeNow()).view())
        .member("root", root);
    bld.member("appData").object();
    for (auto&& [n, v] : s_appData)
        bld.member(n, v);
    bld.end();

    auto infos = httpRouteGetRoutes();
    HttpRouteInfo * best = nullptr;
    for (auto&& ri : infos) {
        if (!ri.name.empty()) {
            auto cpath = Path(ri.path).setExt({}).str();
            if (cpath.size() <= relPath.size()
                && relPath.compare(0, cpath.size(), cpath) == 0
                && (!best || best->path.size() < cpath.size())
            ) {
                best = &ri;
            }
        }
    }
    bld.member("navbar").array();
    for (auto&& ri : infos) {
        if (!ri.name.empty()) {
            addRoute(&bld, ri, best == &ri);
        }
    }
    bld.end();
    return bld;
}


/****************************************************************************
*
*   WebFiles - files from WebRoot
*
***/

namespace {
class WebFiles : public IHttpRouteNotify {
    void onHttpRequest(unsigned reqId, HttpRequest & msg) override;
};
} // namespace

//===========================================================================
void WebFiles::onHttpRequest(unsigned reqId, HttpRequest & msg) {
    auto qpath = msg.query().path;
    auto prefix = httpRouteGetInfo(reqId).path.size();
    qpath.remove_prefix(prefix);
    if (qpath.size() && qpath[0] == '/')
        qpath.remove_prefix(1);
    Path path;
    for (auto&& root : s_webRoots) {
        if (fileChildPath(&path, root, qpath, false))
            continue;
        bool found = false;
        if (!fileDirExists(&found, path) && found)
            path /= "index.html";
        if (!fileExists(&found, path.view()) && found)
            return httpRouteReplyWithFile(reqId, path);
    }
    httpRouteReplyNotFound(reqId, msg);
}


/****************************************************************************
*
*   About helpers
*
***/

//===========================================================================
static void addAboutVars(IJBuilder * out) {
    out->member("runAs", envProcessAccount());
    out->member("computerName", envComputerName());
}


/****************************************************************************
*
*   About - JsonAccount
*
***/

namespace {
class JsonAccount : public IWebAdminNotify {
    void onHttpRequest(unsigned reqId, HttpRequest & msg) override;
};
} // namespace

//===========================================================================
void JsonAccount::onHttpRequest(unsigned reqId, HttpRequest & msg) {
    auto res = HttpResponse(kHttpStatusOk);
    auto bld = initResponse(&res, reqId, msg);
    addAboutVars(&bld);
    envProcessAccountInfo(&bld);
    bld.end();
    httpRouteReply(reqId, move(res));
}


/****************************************************************************
*
*   About - JsonComputer
*
***/

namespace {
class JsonComputer : public IWebAdminNotify {
    void onHttpRequest(unsigned reqId, HttpRequest & msg) override;
};
} // namespace

//===========================================================================
void JsonComputer::onHttpRequest(unsigned reqId, HttpRequest & msg) {
    auto res = HttpResponse(kHttpStatusOk);
    auto bld = initResponse(&res, reqId, msg);
    addAboutVars(&bld);

    // Domain
    auto di = envDomainMembership();
    bld.member("domain").object()
        .member("name", di.name)
        .member("status", envDomainStatusToString(di.status))
        .end();

    // Addresses
    std::vector<HostAddr> addrs;
    addressGetLocal(&addrs);
    bld.member("addresses").array();
    for (auto&& addr : addrs)
        bld.value(toString(addr));
    bld.end();

    // OS Version
    envOSVersion(&bld);

    // Fully qualified DNS name of thie host.
    bld.member("fullDnsName", envComputerDnsName());

    // Environment Variables
    bld.member("env").object();
    for (auto&& [name, val] : envGetVars())
        bld.member(name, val);
    bld.end();

    bld.end();
    httpRouteReply(reqId, move(res));
}


/****************************************************************************
*
*   About - JsonCounters
*
***/

namespace {
class JsonCounters : public IWebAdminNotify {
    void onHttpRequest(unsigned reqId, HttpRequest & msg) override;

    vector<PerfValue> m_values;
};
} // namespace

//===========================================================================
void JsonCounters::onHttpRequest(unsigned reqId, HttpRequest & msg) {
    perfGetValues(&m_values);

    auto res = HttpResponse(kHttpStatusOk);
    auto bld = initResponse(&res, reqId, msg);
    addAboutVars(&bld);
    bld.member("counters");
    bld.object();
    for (auto && perf : m_values) {
        bld.member(perf.name);
        bld.valueRaw(perf.value);
    }
    bld.end();
    bld.end();
    httpRouteReply(reqId, move(res));
}


/****************************************************************************
*
*   About - JsonMemory
*
***/

namespace {
class JsonMemory : public IWebAdminNotify {
    void onHttpRequest(unsigned reqId, HttpRequest & msg) override;

    Param & m_confirm = param("confirm");
};
} // namespace

//===========================================================================
void JsonMemory::onHttpRequest(unsigned reqId, HttpRequest & msg) {
    auto res = HttpResponse(kHttpStatusOk);
    auto bld = initResponse(&res, reqId, msg);
    addAboutVars(&bld);
    if (*m_confirm == "on") {
        debugDumpMemory(&bld);
    } else {
        bld.member("blocks").array().end();
    }
    bld.end();
    httpRouteReply(reqId, move(res));
}


/****************************************************************************
*
*   Network - JsonRoutes
*
***/

namespace {
class JsonRoutes : public IWebAdminNotify {
    void onHttpRequest(unsigned reqId, HttpRequest & msg) override;
};
} // namespace

//===========================================================================
void JsonRoutes::onHttpRequest(unsigned reqId, HttpRequest & msg) {
    auto infos = httpRouteGetRoutes();

    auto res = HttpResponse(kHttpStatusOk);
    auto bld = initResponse(&res, reqId, msg);
    bld.member("routes");
    bld.array();
    for (auto&& info : infos)
        addRoute(&bld, info);
    bld.end();
    bld.end();
    httpRouteReply(reqId, move(res));
}


/****************************************************************************
*
*   Files - JsonCrashFiles
*
***/

namespace {
class JsonCrashFiles : public IWebAdminNotify {
    void onHttpRequest(unsigned reqId, HttpRequest & msg) override;
};
} // namespace

//===========================================================================
void JsonCrashFiles::onHttpRequest(unsigned reqId, HttpRequest & msg) {
    auto res = HttpResponse(kHttpStatusOk);
    auto bld = initResponse(&res, reqId, msg);
    auto dir = appCrashDir();
    bld.member("files").array();
    uint64_t bytes = 0;
    for (auto&& f : fileGlob(dir, "*.dmp")) {
        auto rname = f.path.view();
        rname.remove_prefix(dir.size() + 1);
        fileSize(&bytes, f.path);
        bld.object()
            .member("name", rname)
            .member("mtime", f.mtime)
            .member("size", bytes)
            .end();
    }
    bld.end();
    bld.end();
    httpRouteReply(reqId, move(res));
}


/****************************************************************************
*
*   Files - CrashFiles
*
***/

namespace {
class CrashFiles : public IHttpRouteNotify {
    void onHttpRequest(unsigned reqId, HttpRequest & msg) override;
};
} // namespace

//===========================================================================
void CrashFiles::onHttpRequest(unsigned reqId, HttpRequest & msg) {
    auto qpath = msg.query().path;
    auto prefix = httpRouteGetInfo(reqId).path.size();
    qpath.remove_prefix(prefix);
    Path path;
    if (!appCrashPath(&path, qpath, false))
        return httpRouteReplyNotFound(reqId, msg);
    bool found = false;
    if (auto ec = fileExists(&found, path); !ec && found) {
        httpRouteReplyWithFile(reqId, path);
    } else {
        httpRouteReplyNotFound(reqId, msg);
    }
}


/****************************************************************************
*
*   Configuration files - s_appXml
*
***/

namespace {
class ConfigAppXml : public IConfigNotify {
    void onConfigChange(const XDocument & doc) override;
};
} // namespace
static ConfigAppXml s_appXml;

//===========================================================================
void ConfigAppXml::onConfigChange(const XDocument & doc) {
    s_webRoots.clear();
    for (auto&& root : configStrings(doc, "WebRoot"))
        s_webRoots.push_back(Path(root));
    if (s_webRoots.empty())
        s_webRoots.push_back(Path("web"));
    for (auto&& root : s_webRoots)
        root.resolve(appRootDir());
}


/****************************************************************************
*
*   ShutdownNotify
*
***/

namespace {
class ShutdownNotify : public IShutdownNotify {
    void onShutdownConsole (bool firstTry) override;
};
} // namespace
static ShutdownNotify s_cleanup;

//===========================================================================
void ShutdownNotify::onShutdownConsole (bool firstTry) {
    s_appData.clear();
}


/****************************************************************************
*
*   Public API
*
***/

static HttpRouteRedirectNotify s_redirectRoot("/web/srv/about-counters.html");
static WebFiles s_webFiles;
static JsonAccount s_jsonAccount;
static JsonComputer s_jsonComputer;
static JsonCounters s_jsonCounters;
static JsonMemory s_jsonMemory;
static JsonRoutes s_jsonRoutes;
static JsonCrashFiles s_jsonCrashFiles;
static CrashFiles s_crashFiles;

//===========================================================================
void Dim::iWebAdminInitialize() {
    if (!appFlags().any(fAppWithWebAdmin))
        return;

    configMonitor("app.xml", &s_appXml);

    httpRouteAdd({
        .notify = &s_redirectRoot,
        .path = "/",
    });
    httpRouteAdd({
        .notify = &s_webFiles,
        .path = "/web",
        .recurse = true
    });
    httpRouteAddAlias({ .path = "/favicon.ico" }, "/web/favicon.ico");

    // About
    httpRouteAdd({
        .notify = nullptr,
        .path = "/srv/about",
        .name = "About",
        .desc = "Details about this server instance",
        .renderPath = "/web/srv/about-counters.html",
    });
    httpRouteAdd({
        .notify = &s_jsonCounters,
        .path = "/srv/about/counters.json",
    });
    httpRouteAdd({
        .notify = &s_jsonAccount,
        .path = "/srv/about/account.json",
    });
    httpRouteAdd({
        .notify = &s_jsonComputer,
        .path = "/srv/about/computer.json",
    });
    httpRouteAdd({
        .notify = &s_jsonMemory,
        .path = "/srv/about/memory.json"
    });

    // Network
    httpRouteAdd({
        .notify = nullptr,
        .path = "/srv/network",
        .name = "Network",
        .desc = "Communication between servers.",
        .renderPath = "/web/srv/network-routes.html",
    });
    httpRouteAdd({
        .notify = nullptr,
        .path = "/srv/network/conns.json",
    });
    httpRouteAdd({
        .notify = &s_jsonRoutes,
        .path = "/srv/network/routes.json",
    });
    httpRouteAdd({
        .notify = nullptr,
        .path = "/srv/network/messages.json",
    });

    // Files
    iConfigWebInitialize();
    iLogFileWebInitialize();
    httpRouteAdd({
        .notify = nullptr,
        .path = "/srv/file",
        .name = "Files",
        .desc = "Config, log, crash, and other files.",
        .renderPath = "/web/srv/file-configs.html"
    });
    httpRouteAdd({
        .notify = &s_jsonCrashFiles,
        .path = "/srv/file/crashes.json",
    });
    httpRouteAdd({
        .notify = &s_crashFiles,
        .path = "/srv/file/crashes/",
        .recurse = true,
    });
}

//===========================================================================
unordered_map<string, string> & Dim::webAdminAppData() {
    return s_appData;
}
