// charbuftest.cpp - dim test charbuf
#include "pch.h"
#pragma hdrstop

using namespace std;
using namespace Dim;


/****************************************************************************
*
*   Declarations
*
***/


/****************************************************************************
*
*   Helpers
*
***/


/****************************************************************************
*
*   Application
*
***/

namespace {

class Application : public ITaskNotify, public ILogNotify {
    // ITaskNotify
    void onTask() override;

    // ILogNotify
    void onLog(LogType type, const string & msg) override;

    int m_errors{0};
};

} // namespace

//===========================================================================
void Application::onLog(LogType type, const string & msg) {
    if (type >= kLogError) {
        ConsoleScopedAttr attr(kConsoleError);
        m_errors += 1;
        cout << "ERROR: " << msg << endl;
    } else {
        cout << msg << endl;
    }
}

//===========================================================================
void Application::onTask() {

    if (m_errors) {
        ConsoleScopedAttr attr(kConsoleError);
        cerr << "*** " << m_errors << " FAILURES" << endl;
        appSignalShutdown(1);
    } else {
        cout << "All tests passed" << endl;
        appSignalShutdown(0);
    }
}


/****************************************************************************
*
*   External
*
***/

int main(int argc, char * argv[]) {
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
    _set_error_mode(_OUT_TO_MSGBOX);
    Application app;
    logAddNotify(&app);
    return appRun(app);
}
