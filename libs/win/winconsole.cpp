// Copyright Glen Knowles 2016 - 2017.
// Distributed under the Boost Software License, Version 1.0.
//
// winconsole.cpp - dim windows platform
#include "pch.h"
#pragma hdrstop

using namespace std;
using namespace Dim;


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
        case CTRL_BREAK_EVENT: appSignalShutdown(EX_IOERR); return true;
        }
    }

    return false;
}

//===========================================================================
static void enableConsoleFlags(bool enable, DWORD flags) {
    HANDLE hInput = GetStdHandle(STD_INPUT_HANDLE);
    DWORD mode = 0;
    GetConsoleMode(hInput, &mode);
    if (enable) {
        mode |= flags;
    } else {
        mode &= ~flags;
    }
    SetConsoleMode(hInput, mode);
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
void Dim::iConsoleInitialize() {
    // set ctrl-c handler
    SetConsoleCtrlHandler(&controlCallback, true);
}


/****************************************************************************
*
*   Public API
*
***/

//===========================================================================
void Dim::consoleEnableLineBuffer(bool enable) {
    enableConsoleFlags(enable, ENABLE_LINE_INPUT);
}

//===========================================================================
void Dim::consoleEnableEcho(bool enable) {
    enableConsoleFlags(enable, ENABLE_ECHO_INPUT);
}

//===========================================================================
void Dim::consoleEnableCtrlC(bool enable) {
    s_controlEnabled = enable;
}
