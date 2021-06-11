// Copyright Glen Knowles 2017 - 2021.
// Distributed under the Boost Software License, Version 1.0.
//
// webadmin.cpp - dim app
#include "pch.h"
#pragma hdrstop

using namespace std;
using namespace Dim;


/****************************************************************************
*
*   WebRoot
*
***/

namespace {
class WebRoot : public IHttpRouteNotify {
    void onHttpRequest(unsigned reqId, HttpRequest & msg) override;
};
} // namespace

//===========================================================================
void WebRoot::onHttpRequest(unsigned reqId, HttpRequest & msg) {
    auto qpath = msg.query().path;
    auto prefix = httpRouteGetInfo(reqId).path.size();
    qpath.remove_prefix(prefix);
    auto path = Path(qpath).resolve("Web");
    qpath = path.view();
    if (qpath != "Web" && qpath.substr(0, 4) != "Web/")
        return httpRouteReplyNotFound(reqId, msg);

    if (fileDirExists(path))
        path /= "index.html";
    if (fileExists(path)) {
        httpRouteReplyWithFile(reqId, path);
    } else {
        httpRouteReplyNotFound(reqId, msg);
    }
}


/****************************************************************************
*
*   JsonCounters
*
***/

namespace {
class JsonCounters : public IHttpRouteNotify {
    void onHttpRequest(unsigned reqId, HttpRequest & msg) override;
    vector<PerfValue> m_values;
};
} // namespace

//===========================================================================
void JsonCounters::onHttpRequest(unsigned reqId, HttpRequest & msg) {
    perfGetValues(&m_values);

    HttpResponse res(kHttpStatusOk, "application/json");
    JBuilder bld(&res.body());

    bld.object();
    for (auto && perf : m_values) {
        bld.member(perf.name);
        bld.valueRaw(perf.value);
    }
    bld.end();
    httpRouteReply(reqId, move(res));
}


/****************************************************************************
*
*   JsonRoutes
*
***/

namespace {
class JsonRoutes : public IHttpRouteNotify {
    void onHttpRequest(unsigned reqId, HttpRequest & msg) override;
};
} // namespace

//===========================================================================
void JsonRoutes::onHttpRequest(unsigned reqId, HttpRequest & msg) {
    HttpResponse res(kHttpStatusOk, "application/json");
    JBuilder bld(&res.body());
    httpRouteGetRoutes(&bld);
    httpRouteReply(reqId, move(res));
}


/****************************************************************************
*
*   BinDump
*
***/

namespace {
class BinDump : public IHttpRouteNotify {
    void onHttpRequest(unsigned reqId, HttpRequest & msg) override;
};
} // namespace

//===========================================================================
void BinDump::onHttpRequest(unsigned reqId, HttpRequest & msg) {
    auto qpath = msg.query().path;
    auto prefix = httpRouteGetInfo(reqId).path.size();
    qpath.remove_prefix(prefix);
    Path path;
    if (!appCrashPath(&path, qpath, false))
        return httpRouteReplyNotFound(reqId, msg);
    if (fileExists(path)) {
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

static WebRoot s_webRoot;
static JsonCounters s_jsonCounters;
static JsonRoutes s_jsonRoutes;
static HttpRouteDirListNotify s_jsonDumpFiles({});
static BinDump s_jsonDump;

//===========================================================================
void Dim::iWebAdminInitialize() {
    if (appFlags() & fAppWithWebAdmin) {
        httpRouteAdd(&s_webRoot, "/", fHttpMethodGet, true);
        httpRouteAdd(&s_jsonCounters, "/srv/counters.json");
        httpRouteAdd(&s_jsonRoutes, "/srv/routes.json");
        s_jsonDumpFiles.set(appCrashDir());
        httpRouteAdd(&s_jsonDumpFiles, "/srv/crashes.json");
        httpRouteAdd(&s_jsonDump, "/srv/crashes/", fHttpMethodGet, true);
    }
}
