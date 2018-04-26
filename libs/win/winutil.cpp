// Copyright Glen Knowles 2017 - 2018.
// Distributed under the Boost Software License, Version 1.0.
//
// winutil.cpp - dim windows platform
#include "pch.h"
#pragma hdrstop

using namespace std;
using namespace Dim;


/****************************************************************************
*
*   Public LoadProc API
*
***/

//===========================================================================
FARPROC Dim::winLoadProc(const char lib[], const char proc[], bool optional) {
    FARPROC fn{nullptr};
    HMODULE mod = LoadLibrary(lib);
    if (!mod) {
        if (!optional)
            logMsgFatal() << "LoadLibrary(" << lib << "): " << WinError{};
        return fn;
    }

    fn = GetProcAddress(mod, proc);
    if (!fn) {
        if (!optional)
            logMsgFatal() << "GetProcAddress(" << proc << "): " << WinError{};
    }
    return fn;
}


/****************************************************************************
*
*   Public Privileges API
*
***/

//===========================================================================
bool Dim::winEnablePrivilege(string_view name, bool enable) {
    auto proc = GetCurrentProcess();
    HANDLE token;
    if (!OpenProcessToken(proc, TOKEN_ADJUST_PRIVILEGES, &token))
        return false;

    bool success = false;
    auto wname = toWstring(name);
    TOKEN_PRIVILEGES tp;
    tp.PrivilegeCount = 1;
    tp.Privileges[0].Attributes = enable ? SE_PRIVILEGE_ENABLED : 0;
    if (LookupPrivilegeValueW(
        NULL,
        wname.c_str(),
        &tp.Privileges[0].Luid
    )) {
        if (AdjustTokenPrivileges(
            token,
            false,  // disable all
            &tp,
            0,      // size of buffer to receive previous state
            NULL,   // pointer to buffer to previous state
            NULL    // size of previous state placed in buffer
        )) {
            WinError err;
            success = (err == ERROR_SUCCESS);
        }
    }
    CloseHandle(token);
    return success;
}


/****************************************************************************
*
*   Public Overlapped API
*
***/

//===========================================================================
void Dim::winSetOverlapped(
    OVERLAPPED & overlapped,
    int64_t off,
    HANDLE event
) {
    overlapped = {};
    overlapped.Offset = (DWORD)off;
    overlapped.OffsetHigh = (DWORD)(off >> 32);
    if (event != INVALID_HANDLE_VALUE)
        overlapped.hEvent = event;
}


/****************************************************************************
*
*   IWinOverlappedNotify
*
***/

//===========================================================================
IWinOverlappedNotify::IWinOverlappedNotify(TaskQueueHandle hq) {
    m_evt.notify = this;
    m_evt.hq = hq;
}

//===========================================================================
void IWinOverlappedNotify::pushOverlappedTask() {
    if (m_evt.hq) {
        taskPush(m_evt.hq, m_evt.notify);
    } else {
        taskPushEvent(m_evt.notify);
    }
}

//===========================================================================
IWinOverlappedNotify::WinOverlappedResult
IWinOverlappedNotify::getOverlappedResult() {
    DWORD bytes = 0;
    if (!GetOverlappedResult(
        INVALID_HANDLE_VALUE,
        &m_evt.overlapped,
        &bytes,
        false
    )) {
        return {{}, bytes};
    }
    return {0, bytes};
}
