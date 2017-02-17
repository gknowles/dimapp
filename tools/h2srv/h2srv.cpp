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
*   TNetConn
*
***/

class TnetConn : public IAppSocketNotify {
public:
    TnetConn(AppSocket & sock)
        : IAppSocketNotify(sock) {}
};


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
void MainShutdown::onAppStartClientCleanup() {}

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
class Application : public ITaskNotify {
    int m_argc;
    char ** m_argv;

public:
    Application(int argc, char * argv[]);
    void onTask() override;
};
} // namespace

//===========================================================================
Application::Application(int argc, char * argv[])
    : m_argc(argc)
    , m_argv(argv) {}

//===========================================================================
void Application::onTask() {
    appMonitorShutdown(&s_cleanup);

    Cli cli;
    cli.header(
        "h2srv v"s + kVersion + " (" __DATE__ ") "
                                "sample http/2 server");
    cli.versionOpt(kVersion);
    if (!cli.parse(m_argc, m_argv))
        return appSignalUsageError();

    vector<Address> addrs;
    addressGetLocal(&addrs);
    cout << "Local Addresses:" << endl;
    for (auto && addr : addrs)
        cout << addr << endl;

    Endpoint end;
    parse(&end, "127.0.0.1", 8888);

    appSocketAddListener<TnetConn>(AppSocket::kByte, "", end);
    // socketListen(&s_listen, end);

    // httpRouteAdd(&s_web,

    // appSignalShutdown(EX_OK);
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

    Application app(argc, argv);
    return appRun(app);
}
