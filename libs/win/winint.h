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
*   Util
*
***/

//---------------------------------------------------------------------------
// Load DLL Procedure

FARPROC winLoadProc(const char lib[], const char proc[], bool optional);

//===========================================================================
template <typename FN>
static void winLoadProc(
    FN & fn, 
    const char lib[], 
    const char proc[], 
    bool optional = false
) {
    // FN must be pointer to function, which will be set to the address of
    // the procedure being loaded.
    static_assert(std::is_pointer_v<FN>);
    static_assert(std::is_function_v<std::pointer_traits<FN>::element_type>);
    fn = (FN) winLoadProc(lib, proc, optional);
}

//---------------------------------------------------------------------------
// Overlapped

void winSetOverlapped(
    OVERLAPPED & overlapped,
    int64_t off,
    HANDLE event = INVALID_HANDLE_VALUE
);

struct WinOverlappedEvent {
    OVERLAPPED overlapped{};
    ITaskNotify * notify{nullptr};
    TaskQueueHandle hq{};
};
// Allow reinterpret_cast to/from OVERLAPPED
static_assert(std::is_standard_layout_v<WinOverlappedEvent>);
static_assert(offsetof(WinOverlappedEvent, overlapped) == 0);

class IWinOverlappedNotify : public ITaskNotify {
public:
    IWinOverlappedNotify(TaskQueueHandle hq = {});
    OVERLAPPED & overlapped() { return m_evt.overlapped; }

    virtual void onTask() override = 0;

private:
    WinOverlappedEvent m_evt;
};


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

private:
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
    WinError(const WinError & from) = default;
    WinError(int error);
    // sets equivalent standard windows error value
    WinError(NtStatus status);
    WinError(SecurityStatus status);

    WinError & operator=(const WinError & from) = default;
    WinError & operator=(int error) { return *this = WinError{error}; }
    WinError & operator=(NtStatus status) { return *this = WinError{status}; }
    WinError & operator=(SecurityStatus status) { 
        return *this = WinError{status}; 
    }

    WinError & set();   // calls GetLastError()

    operator int() const { return m_value; }
    explicit operator NtStatus() const { return (NtStatus) m_ntStatus; }
    explicit operator SecurityStatus() const { 
        return (SecurityStatus) m_secStatus; 
    }

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
*   IOCP
*
***/

void winIocpInitialize();

bool winIocpBindHandle(HANDLE handle);


/****************************************************************************
*
*   Service
*
***/

void winServiceInitialize();


} // namespace
