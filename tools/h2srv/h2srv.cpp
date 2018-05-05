// Copyright Glen Knowles 2016 - 2018.
// Distributed under the Boost Software License, Version 1.0.
//
// h2srv.cpp - h2srv
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
    cli.header("h2srv v"s + kVersion + " (" __DATE__ ") sample http/2 server");
    cli.versionOpt(kVersion, "h2srv");
    if (!cli.parse(argc, argv))
        return appSignalUsageError();

    consoleCatchCtrlC();

    vector<Address> addrs;
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
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF
        | _CRTDBG_LEAK_CHECK_DF
        | _CRTDBG_DELAY_FREE_MEM_DF
    );
    // _CrtSetBreakAlloc(6909);

    return appRun(app, argc, argv, fAppServer | fAppWithConsole);
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

#include "Windows.h"

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
