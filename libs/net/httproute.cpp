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

class RouteConn : public IAppSocketNotify {
    void onSocketAccept(const AppSocketInfo & info) override;
    void onSocketDisconnect() override;
    void onSocketRead(const AppSocketData & data) override;

private:
    HttpConnHandle m_conn;
};

} // namespace


/****************************************************************************
*
*   Variables
*
***/

static vector<PathInfo> s_paths;
static Endpoint s_endpoint;


/****************************************************************************
*
*   RouteConn
*
***/

//===========================================================================
void RouteConn::onSocketAccept(const AppSocketInfo & info) {
    m_conn = httpListen();
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
            auto req = static_cast<HttpRequest *>(msg.get());
            auto path = req->pathAbsolute();
            assert(path);
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
    unsigned methods
) {
    assert(!path.empty());
    if (s_paths.empty())
        appSocketAddListener<RouteConn>(AppSocket::kHttp2, "", s_endpoint);
    PathInfo pi{notify, string(path), methods};
    s_paths.push_back(pi);
}

//===========================================================================
void Dim::httpRouteReply(unsigned reqId, HttpResponse & msg, bool more) {
}

//===========================================================================
void Dim::httpRouteReply(unsigned reqId, const CharBuf & data, bool more) {
}
