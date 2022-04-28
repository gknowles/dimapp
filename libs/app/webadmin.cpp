// Copyright Glen Knowles 2017 - 2022.
// Distributed under the Boost Software License, Version 1.0.
//
// webadmin.cpp - dim app
#include "pch.h"
#pragma hdrstop

using namespace std;
using namespace Dim;


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
    bld.member("baseName", appBaseName())
        .member("appIndex", appIndex())
        .member("version", toString(appVersion()))
        .member("buildTime", Time8601Str(envExecBuildTime()).view())
        .member("startTime", Time8601Str(envProcessStartTime()).view())
        .member("address", toString(appAddress()));
    bld.member("runAs", envProcessAccount());
    bld.member("computerName", envComputerName());
    bld.member("groupIndex", appGroupIndex())
        .member("groupType", appGroupType());
    bld.end();
    bld.member("now", Time8601Str(timeNow()).view())
        .member("root", root);

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
*   WebFiles - files from appWebDir()
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
    if (!appWebPath(&path, qpath))
        return httpRouteReplyNotFound(reqId, msg);
    qpath = path.view();

    bool found = false;
    if (auto ec = fileDirExists(&found, path); !ec && found)
        path /= "index.html";
    if (auto ec = fileExists(&found, path); !ec && found) {
        httpRouteReplyWithFile(reqId, path);
    } else {
        httpRouteReplyNotFound(reqId, msg);
    }
}


/****************************************************************************
*
*   JsonAccount
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
    envProcessAccountInfo(&bld);
    bld.end();
    httpRouteReply(reqId, move(res));
}


/****************************************************************************
*
*   JsonComputer
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
    auto di = envDomainMembership();
    bld.member("domain").object()
        .member("name", di.name)
        .member("status", envDomainStatusToString(di.status))
        .end();
    //addresses
    //os version
    //dns?
    bld.end();
    httpRouteReply(reqId, move(res));
}


/****************************************************************************
*
*   JsonCounters
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
*   JsonRoutes
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
*   JsonCrashFiles
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
    for (auto&& f : FileIter(dir, "*.dmp")) {
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
*   CrashFiles
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
*   Public API
*
***/

static HttpRouteRedirectNotify s_redirectRoot("/web/srv/about-counters.html");
static WebFiles s_webFiles;
static JsonAccount s_jsonAccount;
static JsonComputer s_jsonComputer;
static JsonCounters s_jsonCounters;
static JsonRoutes s_jsonRoutes;
static JsonCrashFiles s_jsonCrashFiles;
static CrashFiles s_crashFiles;

//===========================================================================
void Dim::iWebAdminInitialize() {
    if (appFlags().none(fAppWithWebAdmin)) 
        return;

    httpRouteAdd({
        .notify = &s_redirectRoot,
        .path = "/",
    });
    httpRouteAdd({
        .notify = &s_webFiles, 
        .path = "/web", 
        .recurse = true
    });
    httpRouteAdd({
        .notify = &s_redirectRoot,
        .path = "/srv/about",
        .name = "About",
        .desc = "Details about this server instance",
        .renderPath = "/web/srv/about-counters.html",
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
        .notify = &s_jsonCounters, 
        .path = "/srv/about/counters.json",
    });
    httpRouteAdd({
        .notify = &s_jsonRoutes, 
        .path = "/srv/about/routes.json",
    });
    iLogFileWebInitialize();
    httpRouteAdd({
        .notify = &s_jsonCrashFiles, 
        .path = "/srv/crashes.json",
        .name = "Crashes",
        .desc = "List of crash dump files.",
        .renderPath = "/web/srv/crashes.html",
    });
    httpRouteAdd({
        .notify = &s_crashFiles, 
        .path = "/srv/crashes/", 
        .recurse = true,
    });
    httpRouteAdd({
        .notify = nullptr, 
        .path = "/srv/configs.json",
        .name = "Configs",
        .desc = "Configuration file references.",
        .renderPath = "/web/srv/configs.html",
    });
    httpRouteAdd({
        .notify = nullptr, 
        .path = "/srv/conns.json",
        .name = "Connections",
        .desc = "Connections to other servers.",
        .renderPath = "/web/srv/conns.html",
    });
    httpRouteAdd({
        .notify = nullptr, 
        .path = "/srv/memory.json",
        .name = "Memory",
        .desc = "Internal memory usage of process.",
        .renderPath = "/web/srv/memory.html",
    });
    httpRouteAdd({
        .notify = nullptr, 
        .path = "/srv/messages.json",
        .name = "Messages",
        .desc = "Messages sent or received.",
        .renderPath = "/web/srv/messages.html",
    });
}
