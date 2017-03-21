// httproute.cpp - dim services
#include "pch.h"
#pragma hdrstop

using namespace std;
using namespace Dim;


/****************************************************************************
*
*   Tuning parameters
*
***/


/****************************************************************************
*
*   Private declarations
*
***/

namespace {
struct PathInfo {
    IHttpRouteNotify * notify;
    bool recurse;
    string path;
    unsigned methods;
};

class RouteConn : public IAppSocketNotify {
    bool onSocketAccept(const AppSocketInfo & info) override;
    void onSocketDisconnect() override;
    void onSocketRead(const AppSocketData & data) override;

private:
    HttpConnHandle m_conn;
    vector<unsigned> m_reqIds;
};

struct RequestInfo {
    RouteConn * conn;
    int stream;
};

} // namespace


/****************************************************************************
*
*   Variables
*
***/

static vector<PathInfo> s_paths;
static Endpoint s_endpoint;
static unordered_map<unsigned, RequestInfo> s_requests;
static unsigned s_nextReqId;


/****************************************************************************
*
*   Helpers
*
***/

//===========================================================================
static IHttpRouteNotify * find(std::string_view path, HttpMethod method) {
    IHttpRouteNotify * best = nullptr;
    size_t bestLen = 0;
    for (auto && pi : s_paths) {
        if (path == pi.path 
            || pi.recurse && pi.path.compare(0, string::npos, path) == 0
        ) {
            if (pi.path.size() > bestLen) {
                best = pi.notify;
                bestLen = pi.path.size();
            }
        }
    }
    return best;
}

//===========================================================================
static void replyNotFound(unsigned reqId, HttpRequest & req) {
    HttpResponse res;
    XBuilder bld(res.body());
}

//===========================================================================
static void route(unsigned reqId, HttpRequest & req) {
    auto path = string_view(req.pathAbsolute());
    auto method = httpMethodFromString(req.method());
    auto notify = find(path, method);
    if (!notify)
        return replyNotFound(reqId, req);

    unordered_multimap<string_view, string_view> params;
    notify->onHttpRequest(0, params, req);
}

//===========================================================================
static unsigned makeRequestInfo (RouteConn * conn, int stream) {
    auto ri = RequestInfo{conn, stream};
    for (;;) {
        auto id = ++s_nextReqId;
        if (id && s_requests.insert({id, ri}).second)
            return id;
    }
}


/****************************************************************************
*
*   RouteConn
*
***/

//===========================================================================
bool RouteConn::onSocketAccept(const AppSocketInfo & info) {
    m_conn = httpListen();
    return true;
}

//===========================================================================
void RouteConn::onSocketDisconnect() {
    httpClose(m_conn);
    m_conn = {};
}

//===========================================================================
void RouteConn::onSocketRead(const AppSocketData & data) {
    CharBuf buf;
    vector<unique_ptr<HttpMsg>> msgs;
    bool result = httpRecv(m_conn, &buf, &msgs, data.data, data.bytes);
    if (!buf.empty())
        appSocketWrite(this, buf);
    if (!result)
        return appSocketDisconnect(this);
    for (auto && msg : msgs) {
        if (msg->isRequest()) {
            auto id = makeRequestInfo(this, msg->stream());
            if (!m_reqIds.empty() && id < m_reqIds.back()) {
                auto ub = upper_bound(m_reqIds.begin(), m_reqIds.end(), id);
                m_reqIds.insert(ub, id);
            } else {
                m_reqIds.push_back(id);
            }
            auto req = static_cast<HttpRequest *>(msg.get());
            route(id, *req);
        } else {
        }
    }
}


/****************************************************************************
*
*   Shutdown monitor
*
***/

namespace {
    class ShutdownMonitor : public IAppShutdownNotify {
        bool onAppClientShutdown(bool retry) override;
    };
    static ShutdownMonitor s_cleanup;
} // namespace

//===========================================================================
bool ShutdownMonitor::onAppClientShutdown(bool retry) {
    if (!s_paths.empty())
        appSocketRemoveListener<RouteConn>(AppSocket::kHttp2, "", s_endpoint);
    return true;
}


/****************************************************************************
*
*   Internal API
*
***/

//===========================================================================
void Dim::iHttpRouteInitialize() {
    s_endpoint = {};
    s_endpoint.port = 8888;
    appMonitorShutdown(&s_cleanup);
}


/****************************************************************************
*
*   Public API
*
***/

//===========================================================================
void Dim::httpRouteAdd(
    IHttpRouteNotify * notify,
    std::string_view path,
    unsigned methods,
    bool recurse
) {
    assert(!path.empty());
    if (s_paths.empty())
        appSocketAddListener<RouteConn>(AppSocket::kHttp2, "", s_endpoint);
    PathInfo pi{notify, recurse, string(path), methods};
    s_paths.push_back(pi);
}

//===========================================================================
void Dim::httpRouteReply(unsigned reqId, HttpResponse & msg, bool more) {
}

//===========================================================================
void Dim::httpRouteReply(unsigned reqId, const CharBuf & data, bool more) {
}
