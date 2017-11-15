// Copyright Glen Knowles 2015 - 2017.
// Distributed under the Boost Software License, Version 1.0.
//
// winevent.cpp - dim windows platform
#include "pch.h"
#pragma hdrstop

using namespace std;
using namespace std::chrono;
using namespace Dim;


/****************************************************************************
*
*   WinEvent
*
***/

//===========================================================================
WinEvent::WinEvent() {
    m_handle = CreateEvent(
        nullptr, // security attributes
        false,   // manual reset
        false,   // initial signaled state
        nullptr  // name
    );
}

//===========================================================================
WinEvent::~WinEvent() {
    if (m_handle != INVALID_HANDLE_VALUE)
        CloseHandle(m_handle);
}

//===========================================================================
void WinEvent::signal() {
    assert(m_handle != INVALID_HANDLE_VALUE);
    SetEvent(m_handle);
}

//===========================================================================
void WinEvent::wait(Duration wait) {
    assert(m_handle != INVALID_HANDLE_VALUE);
    auto waitMs = duration_cast<milliseconds>(wait);
    if (wait <= 0ms || waitMs >= chrono::milliseconds(INFINITE)) {
        WaitForSingleObject(m_handle, INFINITE);
    } else {
        WaitForSingleObject(m_handle, (DWORD)waitMs.count());
    }
}

//===========================================================================
HANDLE WinEvent::release() {
    HANDLE evt = m_handle;
    m_handle = INVALID_HANDLE_VALUE;
    return evt;
}


/****************************************************************************
*
*   IWinEventWaitNotify
*
***/

//===========================================================================
static VOID CALLBACK eventWaitCallback(PVOID param, BOOLEAN timeout) {
    auto notify = reinterpret_cast<IWinEventWaitNotify *>(param);
    notify->pushOverlappedTask();
}

//===========================================================================
IWinEventWaitNotify::IWinEventWaitNotify(TaskQueueHandle hq)
    : IWinOverlappedNotify{hq}
{
    auto & hevt = overlapped().hEvent;
    hevt = CreateEvent(nullptr, 0, 0, nullptr);
    assert(hevt);

    if (!RegisterWaitForSingleObject(
        &m_registeredWait,
        hevt,
        &eventWaitCallback,
        this,
        INFINITE, // timeout
        WT_EXECUTEINWAITTHREAD
    )) {
        logMsgCrash() << "RegisterWaitForSingleObject: " << WinError{};
    }
}

//===========================================================================
IWinEventWaitNotify::~IWinEventWaitNotify() {
    if (m_registeredWait && !UnregisterWaitEx(m_registeredWait, nullptr)) {
        logMsgError() << "UnregisterWaitEx: " << WinError{};
    }
    if (overlapped().hEvent && !CloseHandle(overlapped().hEvent)) {
        logMsgError() << "CloseHandle(overlapped.hEvent): " << WinError{};
    }
}
