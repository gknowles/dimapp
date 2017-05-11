// Copyright Glen Knowles 2017.
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
*   Private
*
***/

/****************************************************************************
*
*   WebRoot
*
***/

class WebRoot : public IHttpRouteNotify {
    void onHttpRequest(
        unsigned reqId,
        unordered_multimap<string_view, string_view> & params,
        HttpRequest & msg
    ) override;
};

//===========================================================================
void WebRoot::onHttpRequest(
    unsigned reqId,
    unordered_multimap<string_view, string_view> & params,
    HttpRequest & msg
) {
    string path = "Web";
    path += msg.pathAbsolute();
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

class HtmlCounters : public IHttpRouteNotify {
    void onHttpRequest(
        unsigned reqId,
        unordered_multimap<string_view, string_view> & params,
        HttpRequest & msg
    ) override;
};

//===========================================================================
void HtmlCounters::onHttpRequest(
    unsigned reqId,
    unordered_multimap<string_view, string_view> & params,
    HttpRequest & msg
) {
    vector<PerfValue> perfs;
    perfGetValues(perfs);

    HttpResponse res;
    XBuilder bld(res.body());
    bld.start("html").start("body");
    bld.start("table");
    bld.start("tr")
        .elem("th", "Value")
        .elem("th", "Name")
        .end();
    for (auto && perf : perfs) {
        bld.start("tr")
            .elem("td", perf.value.c_str())
            .elem("td", perf.name.data())
            .end();
    }
    bld.end();
    bld.end().end();
    res.addHeader(kHttpContentType, "text/html");
    res.addHeader(kHttp_Status, "200");
    httpRouteReply(reqId, res);
}


/****************************************************************************
*
*   JsonCounters
*
***/

class JsonCounters : public IHttpRouteNotify {
    void onHttpRequest(
        unsigned reqId,
        unordered_multimap<string_view, string_view> & params,
        HttpRequest & msg
    ) override;
};

//===========================================================================
void JsonCounters::onHttpRequest(
    unsigned reqId,
    unordered_multimap<string_view, string_view> & params,
    HttpRequest & msg
) {
    vector<PerfValue> perfs;
    perfGetValues(perfs);

    HttpResponse res;
    JBuilder bld(res.body());
    bld.object();
    for (auto && perf : perfs) {
        bld.startMember(perf.name);
        bld.valueRaw(perf.value);
    }
    bld.end();
    res.addHeader(kHttpContentType, "application/json");
    res.addHeader(kHttp_Status, "200");
    httpRouteReply(reqId, res);
}


/****************************************************************************
*
*   ShutdownNotify
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
}


/****************************************************************************
*
*   Public API
*
***/

static WebRoot s_webRoot;
static HtmlCounters s_htmlCounters;
static JsonCounters s_jsonCounters;

//===========================================================================
void Dim::iWebAdminInitialize() {
    shutdownMonitor(&s_cleanup);

    httpRouteAdd(&s_webRoot, "/", fHttpMethodGet, true);
    httpRouteAdd(&s_htmlCounters, "/srv/counters", fHttpMethodGet);
    httpRouteAdd(&s_jsonCounters, "/srv/counters.json", fHttpMethodGet);
}
