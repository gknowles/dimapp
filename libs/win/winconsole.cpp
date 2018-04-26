// Copyright Glen Knowles 2016 - 2018.
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
static bool s_catchEnabled = false;


/****************************************************************************
*
*   Helpers
*
***/

//===========================================================================
static BOOL WINAPI controlCallback(DWORD ctrl) {
    if (s_catchEnabled) {
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
    if (!GetConsoleMode(hInput, &mode))
        return;
    if (enable) {
        mode |= flags;
    } else {
        mode &= ~flags;
    }
    if (!SetConsoleMode(hInput, mode))
        logMsgFatal() << "SetConsoleMode(): " << WinError{};
}


/****************************************************************************
*
*   ConsoleScopedAttr
*
***/

static unordered_map<ConsoleAttr, int> s_attrs = {
    { kConsoleNormal,    // normal white
        FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE },
    { kConsoleGreen,     // bright green
        FOREGROUND_INTENSITY | FOREGROUND_GREEN },
    { kConsoleHighlight, // bright cyan
        FOREGROUND_INTENSITY | FOREGROUND_GREEN | FOREGROUND_BLUE },
    { kConsoleWarn,      // bright yellow
        FOREGROUND_INTENSITY | FOREGROUND_RED | FOREGROUND_GREEN },
    { kConsoleError,     // bright red
        FOREGROUND_INTENSITY | FOREGROUND_RED },
};

//===========================================================================
ConsoleScopedAttr::ConsoleScopedAttr(ConsoleAttr attr) {
    if (!consoleAttached()) {
        m_active = false;
        return;
    }

    // save console text attributes
    HANDLE hOutput = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO info;
    if (!GetConsoleScreenBufferInfo(hOutput, &info)) {
        logMsgFatal() << "GetConsoleScreenBufferInfo: " << GetLastError();
    }

    scoped_lock<mutex> lk{s_mut};
    s_consoleAttrs.push_back(info.wAttributes);
    if (auto i = s_attrs.find(attr); i != s_attrs.end()) {
        info.wAttributes = (WORD) i->second;
        SetConsoleTextAttribute(hOutput, info.wAttributes);
    }
}

//===========================================================================
ConsoleScopedAttr::~ConsoleScopedAttr() {
    if (m_active) {
        HANDLE hOutput = GetStdHandle(STD_OUTPUT_HANDLE);
        scoped_lock<mutex> lk{s_mut};
        SetConsoleTextAttribute(hOutput, s_consoleAttrs.back());
        s_consoleAttrs.pop_back();
    }
}


/****************************************************************************
*
*   Internal API
*
***/

//===========================================================================
void Dim::iConsoleInitialize() {
    // set control-c handler
    SetConsoleCtrlHandler(&controlCallback, true);
}


/****************************************************************************
*
*   Console manipulation
*
***/

//===========================================================================
bool Dim::consoleAttached() {
    return GetConsoleWindow() != NULL;
}

//===========================================================================
void Dim::consoleEnableEcho(bool enable) {
    enableConsoleFlags(enable, ENABLE_LINE_INPUT | ENABLE_ECHO_INPUT);
}

//===========================================================================
void Dim::consoleCatchCtrlC(bool enable) {
    s_catchEnabled = enable;
}

//===========================================================================
void Dim::consoleRedoLine() {
    HANDLE hOutput = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO info;
    if (!GetConsoleScreenBufferInfo(hOutput, &info)) {
        logMsgFatal() << "GetConsoleScreenBufferInfo: " << GetLastError();
    }
    if (info.dwCursorPosition.Y != 0) {
        info.dwCursorPosition.Y -= 1;
        info.dwCursorPosition.X = 0;
        SetConsoleCursorPosition(hOutput, info.dwCursorPosition);
    }
    DWORD count;
    FillConsoleOutputCharacter(
        hOutput,
        ' ',
        info.dwMaximumWindowSize.X,
        info.dwCursorPosition,
        &count
    );
}


/****************************************************************************
*
*   Console handle
*
***/

//===========================================================================
void Dim::consoleResetStdin() {
    if (!consoleAttached()) {
        // process isn't attached to a console
        return;
    }
    HANDLE hInput = GetStdHandle(STD_INPUT_HANDLE);
    DWORD mode = 0;
    if (GetConsoleMode(hInput, &mode))
        return;

    // Create explicit handle to console and duplicate it into stdin
    hInput = CreateFileW(
        L"conin$",
        GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        NULL, // security attributes
        OPEN_EXISTING,
        0, // flags and attributes
        NULL // template file
    );
    if (hInput == INVALID_HANDLE_VALUE) {
        logMsgError() << "CreateFileW(conin$): " << WinError{};
        return;
    }
    if (!SetStdHandle(STD_INPUT_HANDLE, hInput))
        logMsgError() << "SetStdHandle(in): " << WinError{};
}
