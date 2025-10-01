// Copyright Glen Knowles 2016 - 2025.
// Distributed under the Boost Software License, Version 1.0.
//
// h2srv.cpp - h2srv
#include "pch.h"
#pragma hdrstop

using namespace std;
using namespace Dim;


/****************************************************************************
*
*   Tuning parameters
*
***/

const VersionInfo kVersion = { 1, 0 };


/****************************************************************************
*
*   Variables
*
***/

static SockMgrHandle s_mgr;


/****************************************************************************
*
*   TNetConn
*
***/

class TNetConn : public IAppSocketNotify {
public:
    bool onSocketAccept(const AppSocketConnectInfo & accept) override;
    void onSocketDisconnect() override;
    bool onSocketRead(AppSocketData & data) override;

private:
    AppSocketConnectInfo m_accept;
};

//===========================================================================
bool TNetConn::onSocketAccept(const AppSocketConnectInfo & accept) {
    m_accept = accept;
    cout << m_accept.remoteAddr << " connected on "
        << m_accept.localAddr << endl;
    return true;
}

//===========================================================================
void TNetConn::onSocketDisconnect() {
    cout << m_accept.remoteAddr << " disconnected" << endl;
}

//===========================================================================
bool TNetConn::onSocketRead(AppSocketData & data) {
    cout << m_accept.remoteAddr << ": ";
    cout.write(data.data, data.bytes);
    return true;
}


/****************************************************************************
*
*   ShutdownNotify
*
***/

namespace {
class ShutdownNotify : public IShutdownNotify {
    void onShutdownClient(bool firstTry) override;
};
} // namespace
static ShutdownNotify s_cleanup;

//===========================================================================
void ShutdownNotify::onShutdownClient(bool firstTry) {
    logMsgInfo() << "Server stopped";
}


/****************************************************************************
*
*   h2srvXml
*
***/

namespace {
class ConfigH2srvXml : public IConfigNotify {
    void onConfigChange(const XDocument & doc) override;
};
} // namespace
static ConfigH2srvXml s_h2srvXml;

//===========================================================================
void ConfigH2srvXml::onConfigChange(const XDocument & doc) {
}


/****************************************************************************
*
*   Application
*
***/

//===========================================================================
static void app(Cli & cli) {
    shutdownMonitor(&s_cleanup);

    webAdminAppData()["github"] = "http://github.com/gknowles/dimapp";
    consoleCatchCtrlC();

    vector<HostAddr> addrs;
    addressGetLocal(&addrs);
    cout << "Local Addresses:" << endl;
    for (auto && addr : addrs)
        cout << addr << endl;

    configMonitor("h2srv.xml", &s_h2srvXml);
    winTlsInitialize();
    appTlsInitialize();

    s_mgr = sockMgrListen<TNetConn>("raw", AppSocket::kRaw);

    logMsgInfo() << "Server started";
    cli.fail(EX_PENDING);
}

//===========================================================================
static int doMain(int argc, char * argv[]) {
    Cli cli;
    cli.header(cli.header() + " sample http/2 server")
        .action(app);
    return appRun(argc, argv, kVersion, {}, fAppServer | fAppWithConsole);
}


/****************************************************************************
*
*   Externals
*
***/

//===========================================================================
int main(int argc, char * argv[]) {
    return doMain(argc, argv);
}

//===========================================================================
int WINAPI WinMain(
    _In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ char * lpCmdLine,
    _In_ int nShowCmd
) {
    auto cmdline = GetCommandLineA();
    auto args = Cli::toArgv(cmdline);
    auto pargs = Cli::toPtrArgv(args);
    return doMain((int) pargs.size(), (char **) pargs.data());
}
