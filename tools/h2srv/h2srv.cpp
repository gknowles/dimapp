// main.cpp - h2srv
#include "pch.h"
#pragma hdrstop

using namespace std;
using namespace Dim;


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

    if (m_argc < 2) {
        cout << "tnet v1.0 (" __DATE__ ")\n"
             << "usage: h2srv <remote address> [<local address>]\n";
        return appSignalShutdown(EX_USAGE);
    }

    appSignalShutdown(EX_OK);
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
