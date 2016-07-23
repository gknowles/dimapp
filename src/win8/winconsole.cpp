// winconsole.cpp - dim core - windows platform
#include "pch.h"
#pragma hdrstop

using namespace std;

namespace Dim {


/****************************************************************************
*
*   Variables
*
***/

static mutex s_mut;
static vector<WORD> s_consoleAttrs;
static bool s_controlEnabled = false;


/****************************************************************************
*
*   Helpers
*
***/

//===========================================================================
static BOOL WINAPI controlCallback(DWORD ctrl) {
    if (s_controlEnabled) {
        switch (ctrl) {
        case CTRL_C_EVENT:
        case CTRL_BREAK_EVENT: appSignalShutdown(kExitCtrlBreak); return true;
        }
    }

    return false;
}


/****************************************************************************
*
*   ConsoleScopedAttr
*
***/

//===========================================================================
ConsoleScopedAttr::ConsoleScopedAttr(ConsoleAttr attr) {
    // save console text attributes
    HANDLE hOutput = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO info;
    if (!GetConsoleScreenBufferInfo(hOutput, &info)) {
        logMsgCrash() << "GetConsoleScreenBufferInfo: " << GetLastError();
    }

    lock_guard<mutex> lk{s_mut};
    s_consoleAttrs.push_back(info.wAttributes);
    switch (attr) {
    case kConsoleNormal:
        info.wAttributes = FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_RED;
        break;
    case kConsoleGreen:
        info.wAttributes = FOREGROUND_INTENSITY | FOREGROUND_GREEN;
        break;
    case kConsoleHighlight:
        info.wAttributes =
            FOREGROUND_INTENSITY | FOREGROUND_BLUE | FOREGROUND_GREEN;
        break;
    case kConsoleError:
        info.wAttributes = FOREGROUND_INTENSITY | FOREGROUND_RED;
        break;
    }
    SetConsoleTextAttribute(hOutput, info.wAttributes);
}

//===========================================================================
ConsoleScopedAttr::~ConsoleScopedAttr() {
    HANDLE hOutput = GetStdHandle(STD_OUTPUT_HANDLE);
    lock_guard<mutex> lk{s_mut};
    SetConsoleTextAttribute(hOutput, s_consoleAttrs.back());
    s_consoleAttrs.pop_back();
}


/****************************************************************************
*
*   Internal API
*
***/

//===========================================================================
void iConsoleInitialize() {
    // set ctrl-c handler
    SetConsoleCtrlHandler(&controlCallback, true);
}


/****************************************************************************
*
*   Internal API
*
***/

//===========================================================================
void consoleEnableEcho(bool enable) {
    HANDLE hInput = GetStdHandle(STD_INPUT_HANDLE);
    DWORD mode = 0;
    GetConsoleMode(hInput, &mode);
    if (enable) {
        mode |= ENABLE_ECHO_INPUT;
    } else {
        mode &= ~ENABLE_ECHO_INPUT;
    }
    SetConsoleMode(hInput, mode);
}

//===========================================================================
void consoleEnableCtrlC(bool enable) {
    s_controlEnabled = enable;
}

} // namespace
