// Copyright Glen Knowles 2017.
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
            logMsgCrash() << "LoadLibrary(" << lib << "): " << WinError{};
        return fn;
    }

    fn = GetProcAddress(mod, proc);
    if (!fn) {
        if (!optional)
            logMsgCrash() << "GetProcAddress(" << proc << "): " << WinError{};
    }
    return fn;
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
        taskPush(m_evt.hq, *m_evt.notify);
    } else {
        taskPushEvent(*m_evt.notify);
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
