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
    void onSocketRead(AppSocketData & data) override;

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
void TnetConn::onSocketRead(AppSocketData & data) {
    cout << m_accept.remote << ": ";
    cout.write(data.data, data.bytes);
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
*   Application
*
***/

namespace {
class Application : public IAppNotify, public IConfigNotify {
    void onAppRun() override;
    void onConfigChange(const XDocument & doc) override;
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

    configMonitor("h2srv.xml", this);
    winTlsInitialize();
    appTlsInitialize();

    s_mgr = sockMgrListen(
        "raw", 
        getFactory<IAppSocketNotify, TnetConn>(), 
        AppSocket::kRaw
    );

    logMsgInfo() << "Server started";
}

//===========================================================================
void Application::onConfigChange(const XDocument & doc) {
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
