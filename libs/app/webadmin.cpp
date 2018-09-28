// Copyright Glen Knowles 2017 - 2018.
// Distributed under the Boost Software License, Version 1.0.
//
// webadmin.cpp - dim app
#include "pch.h"
#pragma hdrstop

using namespace std;
using namespace Dim;
namespace fs = std::experimental::filesystem;


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
    string path = "Web";
    path += msg.query().path;
    if (fs::is_regular_file(path)) {
        httpRouteReplyWithFile(reqId, path);
    } else {
        httpRouteReplyNotFound(reqId, msg);
    }
}


/****************************************************************************
*
*   HtmlCounters
*
***/

namespace {
class HtmlCounters : public IHttpRouteNotify {
    void onHttpRequest(unsigned reqId, HttpRequest & msg) override;
    vector<PerfValue> m_values;
};
} // namespace

//===========================================================================
void HtmlCounters::onHttpRequest(unsigned reqId, HttpRequest & msg) {
    perfGetValues(&m_values, true);

    HttpResponse res;
    XBuilder bld(&res.body());
    bld.start("html").start("body");
    bld.start("table");
    bld.start("tr")
        .elem("th", "Value")
        .elem("th", "Name")
        .end();
    for (auto && perf : m_values) {
        bld.start("tr")
            .elem("td", perf.value.c_str())
            .elem("td", perf.name.data())
            .end();
    }
    bld.end();
    bld.end().end();
    res.addHeader(kHttpContentType, "text/html");
    res.addHeader(kHttp_Status, "200");
    httpRouteReply(reqId, move(res));
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

    HttpResponse res;
    JBuilder bld(&res.body());

    bld.object();
    for (auto && perf : m_values) {
        bld.member(perf.name);
        bld.valueRaw(perf.value);
    }
    bld.end();
    res.addHeader(kHttpContentType, "application/json");
    res.addHeader(kHttp_Status, "200");
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
    HttpResponse res;
    JBuilder bld(&res.body());
    httpRouteGetRoutes(&bld);
    res.addHeader(kHttpContentType, "application/json");
    res.addHeader(kHttp_Status, "200");
    httpRouteReply(reqId, move(res));
}


/****************************************************************************
*
*   Public API
*
***/

static WebRoot s_webRoot;
static HtmlCounters s_htmlCounters;
static JsonCounters s_jsonCounters;
static JsonRoutes s_jsonRoutes;

//===========================================================================
void Dim::iWebAdminInitialize() {
    httpRouteAdd(&s_webRoot, "/", fHttpMethodGet, true);
    httpRouteAdd(&s_htmlCounters, "/srv/counters", fHttpMethodGet);
    httpRouteAdd(&s_jsonCounters, "/srv/counters.json", fHttpMethodGet);
    httpRouteAdd(&s_jsonRoutes, "/srv/routes.json", fHttpMethodGet);
}
