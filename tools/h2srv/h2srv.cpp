// Copyright Glen Knowles 2016 - 2021.
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

class TnetConn : public IAppSocketNotify {
public:
    bool onSocketAccept(const AppSocketInfo & accept) override;
    void onSocketDisconnect() override;
    bool onSocketRead(AppSocketData & data) override;

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
bool TnetConn::onSocketRead(AppSocketData & data) {
    cout << m_accept.remote << ": ";
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
static void app(int argc, char *argv[]) {
    shutdownMonitor(&s_cleanup);

    Cli cli;
    cli.header(cli.header() + " sample http/2 server");
    if (!cli.parse(argc, argv))
        return appSignalUsageError();

    consoleCatchCtrlC();

    vector<HostAddr> addrs;
    addressGetLocal(&addrs);
    cout << "Local Addresses:" << endl;
    for (auto && addr : addrs)
        cout << addr << endl;

    configMonitor("h2srv.xml", &s_h2srvXml);
    winTlsInitialize();
    appTlsInitialize();

    s_mgr = sockMgrListen<TnetConn>("raw", AppSocket::kRaw);

    logMsgInfo() << "Server started";
}

//===========================================================================
static int doMain(int argc, char * argv[]) {
    return appRun(app, argc, argv, kVersion, {}, fAppServer | fAppWithConsole);
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
