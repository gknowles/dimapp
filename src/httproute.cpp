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
    void onSocketAccept(const SocketAcceptInfo & info) override;
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


/****************************************************************************
*
*   RouteConn
*
***/

//===========================================================================
void RouteConn::onSocketAccept(const SocketAcceptInfo & info) {
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
        socketDisconnect(this);
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
    while (!s_hosts.empty()) {
        auto i = s_hosts.begin();
        appSocketRemoveListener<RouteConn>(AppSocket::kHttp2, i->first, {});
        s_hosts.erase(i);
    }
}


/****************************************************************************
*
*   Internal API
*
***/

//===========================================================================
void Dim::iHttpRouteInitialize() {
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
    const std::string host,
    const std::string path,
    unsigned methods
) {
    PathInfo pi = {notify, path, methods};
    auto & hi = s_hosts[host];
    hi.paths.push_back(pi);
    if (hi.paths.size() == 1)
        appSocketAddListener<RouteConn>(AppSocket::kHttp2, host, {});
}

//===========================================================================
void Dim::httpRouteReply(unsigned reqId, HttpMsg & msg, bool more) {
}

//===========================================================================
void Dim::httpRouteReply(unsigned reqId, const CharBuf & data, bool more) {
}
