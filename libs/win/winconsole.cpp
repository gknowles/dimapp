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
    CONSOLE_SCREEN_BUFFER_INFO info{};
    if (!GetConsoleScreenBufferInfo(hOutput, &info)) {
        logMsgFatal() << "GetConsoleScreenBufferInfo: " << WinError{};
    }

    scoped_lock lk{s_mut};
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
        scoped_lock lk{s_mut};
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
static bool getStdInfo(int * dst, const wchar_t ** dev, DWORD nstd) {
    switch (nstd) {
    case STD_INPUT_HANDLE: *dst = 0; *dev = L"conin$"; break;
    case STD_OUTPUT_HANDLE: *dst = 1; *dev = L"conout$"; break;
    case STD_ERROR_HANDLE: *dst = 2; *dev = L"conout$"; break;
    default:
        assert(!"Invalid nStdHandle value");
        return false;
    }
    return true;
}

//===========================================================================
inline static void attachStd(DWORD nstd) {
    int dst;
    const wchar_t * dev;
    if (!getStdInfo(&dst, &dev, nstd))
        return;

    int fd = -1;
    auto hs = GetStdHandle(nstd);
    if (!hs || hs == INVALID_HANDLE_VALUE) {
        logMsgFatal() << "GetStdHandle(" << dst << '/' << dev << "): "
            << errno << ", " << _doserrno;
        return;
    }
    fd = _open_osfhandle((intptr_t) hs, _O_TEXT);
    if (fd == -1) {
        logMsgFatal() << "open_osfhandle(" << dst << '/' << dev << "): "
            << errno << ", " << _doserrno;
        return;
    }
    if (fd != dst) {
        if (_dup2(fd, dst) == -1)
            logMsgFatal() << "dup2(" << dst << '/' << dev << "): "
                << errno << ", " << _doserrno;
        _close(fd);
    }
    SetStdHandle(nstd, (HANDLE) _get_osfhandle(dst));
}

//===========================================================================
static void resetStd(DWORD nstd) {
    if (!consoleAttached()) {
        // process isn't attached to a console
        return;
    }
    HANDLE hs = GetStdHandle(nstd);
    DWORD mode = 0;
    if (GetConsoleMode(hs, &mode))
        return;
    int dst;
    const wchar_t * dev;
    if (!getStdInfo(&dst, &dev, nstd))
        return;

    // Create explicit handle to console and attach it to standard file
    // descriptor.
    hs = CreateFileW(
        dev,
        GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        NULL, // security attributes
        OPEN_EXISTING,
        0, // flags and attributes
        NULL // template file
    );
    if (hs == INVALID_HANDLE_VALUE) {
        logMsgError() << "CreateFileW(" << dev << "): " << WinError{};
        return;
    }
    if (!SetStdHandle(nstd, hs)) {
        logMsgError() << "SetStdHandle(in): " << WinError{};
    } else {
        attachStd(nstd);
    }
}

//===========================================================================
void Dim::consoleResetStdin() {
    resetStd(STD_INPUT_HANDLE);
}

//===========================================================================
void Dim::consoleResetStdout() {
    resetStd(STD_OUTPUT_HANDLE);
}

//===========================================================================
void Dim::consoleResetStderr() {
    resetStd(STD_ERROR_HANDLE);
}

//===========================================================================
bool Dim::consoleAttach(intptr_t pid) {
    // The underlying Windows console handles get closed twice, once by
    // closing the CRT handles and again by FreeConsole. This is arguably
    // broken, definitely not thread-safe (the handle could be reused), and
    // the second CloseHandle, quite properly, raises an invalid handle
    // exception when in the debugger.
    //
    // But there is no way to do any of the following:
    //  - close the CRT handles without closing the os handles
    //  - attach the CRT handles to new os handles without first closing
    //      them (i.e. _dup2 implicitly closes the target).
    //  - free the console without closing the os handles
    //      (SetStdHandle doesn't affect the internal list of handles)
    //
    // A set_osfhandle(fd, osfhandle) or release_osfhandle(fd) function would
    // be nice... :/
    _close(0);
    _close(1);
    _close(2);
    FreeConsole();

    bool success = true;
    if (!AttachConsole((DWORD) pid)) {
        success = false;
        WinError err;
        if (err == ERROR_INVALID_HANDLE) {
            // Process supposedly attached to the target console doesn't
            // exist or doesn't have a console.
        } else {
            logMsgError() << "AttachConsole(pid): " << err;
        }
        if (!AllocConsole()) {
            err.set();
            logMsgFatal() << "AllocConsole: " << err;
        }
    }
    if (consoleAttached()) {
        attachStd(STD_INPUT_HANDLE);
        attachStd(STD_OUTPUT_HANDLE);
        attachStd(STD_ERROR_HANDLE);
    }
    return success;
}
