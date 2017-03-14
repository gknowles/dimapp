// main.cpp - h2srv
#include "pch.h"
#pragma hdrstop

using namespace std;
using namespace Dim;


/****************************************************************************
*
*   Declarations
*
***/

const char kVersion[] = "1.0";


/****************************************************************************
*
*   Variables
*
***/

static Endpoint s_endpoint;


/****************************************************************************
*
*   TNetConn
*
***/

class TnetConn : public ISocketNotify {
public:
    void onSocketAccept(const SocketInfo & accept) override;
    void onSocketDisconnect() override;
    void onSocketRead(const SocketData & data) override;

private:
    SocketInfo m_accept;
};

//===========================================================================
void TnetConn::onSocketAccept(const SocketInfo & accept) {
    m_accept = accept;
    cout << m_accept.remote << " connected on " 
        << m_accept.local << endl;
}

//===========================================================================
void TnetConn::onSocketDisconnect() {
    cout << m_accept.remote << " disconnected" << endl;
}

//===========================================================================
void TnetConn::onSocketRead(const SocketData & data) {
    cout << m_accept.remote << ": ";
    cout.write(data.data, data.bytes);
}


/****************************************************************************
*
*   WebRoute
*
***/

class WebRoute : public IHttpRouteNotify {
    void onHttpRequest(
        unsigned reqId,
        unordered_multimap<string_view, string_view> & params,
        HttpRequest & msg
    ) override;
};
static WebRoute s_web;

//===========================================================================
void WebRoute::onHttpRequest(
    unsigned reqId,
    unordered_multimap<string_view, string_view> & params,
    HttpRequest & msg
) {
    cout << "http request #" << reqId << endl;
}


/****************************************************************************
*
*   MainShutdown
*
***/

class MainShutdown : public IAppShutdownNotify {
    void onAppStartClientCleanup() override;
    bool onAppQueryClientDestroy() override;
};
static MainShutdown s_cleanup;

//===========================================================================
void MainShutdown::onAppStartClientCleanup() {
    appSocketRemoveListener<TnetConn>(AppSocket::kByte, "", s_endpoint);
}

//===========================================================================
bool MainShutdown::onAppQueryClientDestroy() {
    return true;
}


/****************************************************************************
*
*   Application
*
***/

namespace {
class Application : public IAppNotify {
    void onAppRun() override;
};
} // namespace

//===========================================================================
void Application::onAppRun() {
    appMonitorShutdown(&s_cleanup);

    Cli cli;
    cli.header(
        "h2srv v"s + kVersion + " (" __DATE__ ") "
                                "sample http/2 server");
    cli.versionOpt(kVersion);
    if (!cli.parse(m_argc, m_argv))
        return appSignalUsageError();

    consoleEnableCtrlC();

    vector<Address> addrs;
    addressGetLocal(&addrs);
    cout << "Local Addresses:" << endl;
    for (auto && addr : addrs)
        cout << addr << endl;

    parse(&s_endpoint, "0.0.0.0", 8888);

    appSocketAddListener<TnetConn>(AppSocket::kByte, "", s_endpoint);

    httpRouteAdd(&s_web, "127.0.0.1:8888", "/", fHttpMethodGet);
}


/****************************************************************************
*
*   Externals
*
***/

//===========================================================================
int main(int argc, char * argv[]) {
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
    _set_error_mode(_OUT_TO_MSGBOX);

    Application app;
    return appRun(app, argc, argv);
}
