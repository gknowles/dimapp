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
    WinOverlappedEvent & evt,
    int64_t off,
    HANDLE event
) {
    evt.overlapped = {};
    evt.overlapped.Offset = (DWORD)off;
    evt.overlapped.OffsetHigh = (DWORD)(off >> 32);
    if (event != INVALID_HANDLE_VALUE)
        evt.overlapped.hEvent = event;
}
