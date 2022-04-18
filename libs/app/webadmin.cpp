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
static void addRoute(IJBuilder * out, const HttpRouteInfo & ri) {
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
    out->end();
}


/****************************************************************************
*
*   IWebAdminNotify
*
***/

//===========================================================================
void IWebAdminNotify::onHttpRequest(
    unsigned reqId, 
    HttpRequest & msg
) {
    HttpResponse res(kHttpStatusOk);
    if (m_jsVar) {
        res.addHeader(kHttpContentType, "text/javascript");
        res.body().append("const ").append(*m_jsVar).append(" = ");
    } else {
        res.addHeader(kHttpContentType, "application/json");
    }
    JBuilder bld(&res.body());
    bld.object()
        .member("server").object()
            .member("baseName", appBaseName())
            .member("appIndex", appIndex())
            .member("version", toString(appVersion()))
            .member("buildTime", Time8601Str(envExecBuildTime()).view())
            .member("startTime", Time8601Str(envProcessStartTime()).view())
            .member("address", toString(appAddress()))
            .end()
        .member("now", Time8601Str(timeNow()).view());

    auto infos = httpRouteGetRoutes();
    bld.member("navbar").array();
    for (auto&& ri : infos) {
        if (!ri.name.empty()) {
            addRoute(&bld, ri);
        }
    }
    bld.end();

    onWebAdminRequest(&bld, reqId, msg);

    bld.end();
    httpRouteReply(reqId, move(res));
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
    void onWebAdminRequest(
        IJBuilder * out, 
        unsigned reqId, 
        HttpRequest & msg
    ) override;
};
} // namespace

//===========================================================================
void JsonAccount::onWebAdminRequest(
    IJBuilder * out,
    unsigned reqId, 
    HttpRequest & msg
) {
    envProcessAccount(out);
}


/****************************************************************************
*
*   JsonCounters
*
***/

namespace {
class JsonCounters : public IWebAdminNotify {
    void onWebAdminRequest(
        IJBuilder * out,
        unsigned reqId, 
        HttpRequest & msg
    ) override;
    vector<PerfValue> m_values;
};
} // namespace

//===========================================================================
void JsonCounters::onWebAdminRequest(
    IJBuilder * out,
    unsigned reqId, 
    HttpRequest & msg
) {
    perfGetValues(&m_values);

    out->member("counters");
    out->object();
    for (auto && perf : m_values) {
        out->member(perf.name);
        out->valueRaw(perf.value);
    }
    out->end();
}


/****************************************************************************
*
*   JsonRoutes
*
***/

namespace {
class JsonRoutes : public IWebAdminNotify {
    void onWebAdminRequest(
        IJBuilder * out,
        unsigned reqId, 
        HttpRequest & msg
    ) override;
};
} // namespace

//===========================================================================
void JsonRoutes::onWebAdminRequest(
    IJBuilder * out,
    unsigned reqId, 
    HttpRequest & msg
) {
    auto infos = httpRouteGetRoutes();
    out->member("routes");
    out->array();
    for (auto&& info : infos) 
        addRoute(out, info);
    out->end();
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

static HttpRouteRedirectNotify s_redirectRoot("/web/srv/counters.html");
static WebFiles s_webFiles;
static JsonAccount s_jsonAccount;
static JsonCounters s_jsonCounters;
static JsonRoutes s_jsonRoutes;
static HttpRouteDirListNotify s_jsonDumpFiles({});
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
        .notify = &s_jsonAccount, 
        .path = "/srv/account.json", 
        .name = "Account",
        .desc = "Details of the account this server is running as.",
        .renderPath = "/web/srv/account.html",
    });
    httpRouteAdd({
        .notify = &s_jsonCounters, 
        .path = "/srv/counters.json",
        .name = "Counters",
        .desc = "Performance counters",
        .renderPath = "/web/srv/counters.html",
    });
    httpRouteAdd({
        .notify = &s_jsonRoutes, 
        .path = "/srv/routes.json",
        .name = "Routes",
        .desc = "URL routes server is listening for.",
        .renderPath = "/web/srv/routes.html",
    });
    iLogFileWebInitialize();
    s_jsonDumpFiles.set(appCrashDir());
    httpRouteAdd({
        .notify = &s_jsonDumpFiles, 
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
}
