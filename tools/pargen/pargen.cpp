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

enum {
    kExitBadArgs = 1,
    kExitConnectFailed = 2,
    kExitDisconnect = 3,
    kExitCtrlBreak = 4,
};


/****************************************************************************
*
*   Variables
*
***/

static WORD s_consoleAttrs;


/****************************************************************************
*
*   Helpers
*
***/

//===========================================================================
static BOOL WINAPI controlCallback (DWORD ctrl) {
    switch (ctrl) {
        case CTRL_C_EVENT:
        case CTRL_BREAK_EVENT:
            appSignalShutdown(kExitCtrlBreak);
            return true;
    }

    return false;
}

//===========================================================================
static void initializeConsole () {
    // set ctrl-c handler
    SetConsoleCtrlHandler(&controlCallback, true);

    // save console text attributes
    HANDLE hOutput = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO info;
    if (!GetConsoleScreenBufferInfo(hOutput, &info)) {
        logMsgCrash() << "GetConsoleScreenBufferInfo: " << GetLastError();
    }
    s_consoleAttrs = info.wAttributes;
}

//===========================================================================
static void setConsoleText (WORD attr) {
    HANDLE hOutput = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleTextAttribute(hOutput, attr);
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

    if (argc < 2) {
        cout << "tnet v1.0 (" __DATE__ ")\n"
            << "usage: tnet <remote address> [<local address>]\n";
        return kExitBadArgs;
    }

    initializeConsole();

    if (s_consoleAttrs)
        setConsoleText(s_consoleAttrs);

    return 0;
}
