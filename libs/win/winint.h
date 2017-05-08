// Copyright Glen Knowles 2015 - 2017.
// Distributed under the Boost Software License, Version 1.0.
//
// winint.h - dim windows platform
#pragma once

#include "core/task.h"
#include "core/timer.h"

#include <cstdint>
#include <iostream>

namespace Dim {


/****************************************************************************
*
*   Load DLL Proc
*
***/

//===========================================================================
template <typename FN>
static void loadProc(FN & fn, const char lib[], const char func[]) {
    HMODULE mod = LoadLibrary(lib);
    if (!mod)
        logMsgCrash() << "LoadLibrary(" << lib << "): " << WinError{};

    fn = (FN)GetProcAddress(mod, func);
    if (!fn) {
        logMsgCrash() << "GetProcAddress(" << func << "): " << WinError{};
    }
}


/****************************************************************************
*
*   Overlapped
*
***/

struct WinOverlappedEvent {
    OVERLAPPED overlapped{};
    ITaskNotify * notify{nullptr};
    TaskQueueHandle hq = {};
};

void winSetOverlapped(
    WinOverlappedEvent & evt,
    int64_t off,
    HANDLE event = INVALID_HANDLE_VALUE
);


/****************************************************************************
*
*   Event
*
***/

class WinEvent {
public:
    WinEvent();
    explicit WinEvent(HANDLE evt)
        : m_handle(evt) {}
    ~WinEvent();

    void signal();
    void wait(Duration wait = kTimerInfinite);
    HANDLE release();

    HANDLE nativeHandle() const { return m_handle; };

private:
    HANDLE m_handle;
};


/****************************************************************************
*
*   Wait for events
*
***/

class IWinEventWaitNotify : public ITaskNotify {
public:
    IWinEventWaitNotify();
    ~IWinEventWaitNotify();

    virtual void onTask() override = 0;

    OVERLAPPED m_overlapped{};
    HANDLE m_registeredWait{nullptr};
};


/****************************************************************************
*
*   Error
*
***/

class WinError {
public:
    enum NtStatus : unsigned {};
    enum SecurityStatus : unsigned {};

public:
    // default constructor calls GetLastError()
    WinError();
    WinError(const WinError & from);
    WinError(NtStatus status);
    WinError(SecurityStatus status);
    WinError(int error);

    WinError & operator=(int error);
    // sets equivalent standard windows error value
    WinError & operator=(NtStatus status);
    WinError & operator=(SecurityStatus status);

    WinError & set();   // calls GetLastError()

    operator int() const { return m_value; }

private:
    int m_value;
    int m_ntStatus{0};
    int m_secStatus{0};
};

std::ostream & operator<<(std::ostream & os, const WinError & val);

void winErrorInitialize();


/****************************************************************************
*
*   App
*
***/

void winAppInitialize();
void winAppConfigInitialize();


/****************************************************************************
*
*   File
*
***/

void winFileMonitorInitialize();

bool winFileSetErrno(int error);


/****************************************************************************
*
*   Iocp
*
***/

void winIocpInitialize();

bool winIocpBindHandle(HANDLE handle);


} // namespace
