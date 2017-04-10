// Copyright Glen Knowles 2016 - 2017.
// Distributed under the Boost Software License, Version 1.0.
//
// h2srv.cpp - h2srv
#include "pch.h"
#pragma hdrstop

using namespace std;
using namespace Dim;
namespace fs = experimental::filesystem;


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

class TnetConn : public IAppSocketNotify {
public:
    bool onSocketAccept(const AppSocketInfo & accept) override;
    void onSocketDisconnect() override;
    void onSocketRead(const AppSocketData & data) override;

private:
    AppSocketInfo m_accept;
};

//===========================================================================
bool TnetConn::onSocketAccept(const AppSocketInfo & accept) {
    m_accept = accept;
    cout << m_accept.remote << " connected on " 
        << m_accept.local << endl;
    return true;
}

//===========================================================================
void TnetConn::onSocketDisconnect() {
    cout << m_accept.remote << " disconnected" << endl;
}

//===========================================================================
void TnetConn::onSocketRead(const AppSocketData & data) {
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
*   MainShutdown
*
***/

class MainShutdown : public IShutdownNotify {
    void onShutdownClient(bool retry) override;
};
static MainShutdown s_cleanup;

//===========================================================================
void MainShutdown::onShutdownClient(bool retry) {
    socketStopWait<TnetConn>(AppSocket::kByte, "", s_endpoint);
}


/****************************************************************************
*
*   Application
*
***/

namespace {
class Application : public IAppNotify, public IAppConfigNotify {
    void onAppRun() override;
    void onConfigChange(string_view relpath, const XNode * root) override;
};
} // namespace

//===========================================================================
void Application::onAppRun() {
    shutdownMonitor(&s_cleanup);

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

    appConfigMonitor("h2srv.xml", this);
    winTlsInitialize();

    parse(&s_endpoint, "0.0.0.0", 8888);
    socketListen<TnetConn>(AppSocket::kByte, "", s_endpoint);
    httpRouteAdd(&s_web, "/", fHttpMethodGet, true);
}

//===========================================================================
void Application::onConfigChange(string_view relpath, const XNode * root) {
}


/****************************************************************************
*
*   Externals
*
***/

//===========================================================================
int main(int argc, char * argv[]) {
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
    //_CrtSetBreakAlloc(2424);
    _set_error_mode(_OUT_TO_MSGBOX);

    Application app;
    return appRun(app, argc, argv, fAppServer | fAppWithConsole);
}
