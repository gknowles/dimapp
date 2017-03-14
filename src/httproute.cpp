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
    string path;
    unsigned methods;
};
struct HostInfo {
    vector<PathInfo> paths;
};

class RouteConn : public ISocketNotify {
    void onSocketAccept(const SocketInfo & info) override;
    void onSocketDisconnect() override;
    void onSocketRead(const SocketData & data) override;

private:
    HttpConnHandle m_conn;
};

} // namespace


/****************************************************************************
*
*   Variables
*
***/

static unordered_map<string, HostInfo> s_hosts;
static Endpoint s_endpoint;


/****************************************************************************
*
*   RouteConn
*
***/

//===========================================================================
void RouteConn::onSocketAccept(const SocketInfo & info) {
    m_conn = httpListen();
}

//===========================================================================
void RouteConn::onSocketDisconnect() {
    httpClose(m_conn);
    m_conn = {};
}

//===========================================================================
void RouteConn::onSocketRead(const SocketData & data) {
    CharBuf buf;
    vector<unique_ptr<HttpMsg>> msgs;
    bool result = httpRecv(m_conn, &buf, &msgs, data.data, data.bytes);
    if (!buf.empty()) {
        //socketWrite(this, 
    }
    if (!result)
        return socketDisconnect(this);
    for (auto && msg : msgs) {
        if (msg->isRequest()) {
            auto req = static_cast<HttpRequest *>(msg.get());
            auto host = req->authority();
            if (!host)
                host = req->headers(kHttpHost).begin()->m_value;
            assert(host);
            s_hosts.find(host);
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
        void onAppStartClientCleanup() override;
    };
    static ShutdownMonitor s_cleanup;
} // namespace

//===========================================================================
void ShutdownMonitor::onAppStartClientCleanup() {
    if (!s_hosts.empty())
        appSocketRemoveListener<RouteConn>(AppSocket::kHttp2, "", s_endpoint);
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
    std::string_view host,
    std::string_view path,
    unsigned methods
) {
    assert(!host.empty());
    if (s_hosts.empty())
        appSocketAddListener<RouteConn>(AppSocket::kHttp2, "", s_endpoint);
    PathInfo pi = {notify, string(path), methods};
    auto & hi = s_hosts[string(host)];
    hi.paths.push_back(pi);
}

//===========================================================================
void Dim::httpRouteReply(unsigned reqId, HttpResponse & msg, bool more) {
}

//===========================================================================
void Dim::httpRouteReply(unsigned reqId, const CharBuf & data, bool more) {
}
