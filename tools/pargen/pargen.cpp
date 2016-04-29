// pargen.cpp - pargen
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
*   Application
*     
***/

namespace {
class Application : public ITaskNotify {
    int m_argc;
    char ** m_argv;
public:
    Application (int argc, char * argv[]);
    void onTask () override;
};
} // namespace

//===========================================================================
Application::Application (int argc, char * argv[]) 
    : m_argc(argc)
    , m_argv(argv)
{}

//===========================================================================
void Application::onTask () {
    if (m_argc < 2) {
        cout << "pargen v0.1.0 (" __DATE__ ") - simplistic parser generator\n"
            << "usage: pargen\n";
        return appSignalShutdown(kExitBadArgs);
    }

    appSignalShutdown(kExitSuccess);
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
