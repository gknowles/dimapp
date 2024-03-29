// Copyright Glen Knowles 2015 - 2022.
// Distributed under the Boost Software License, Version 1.0.
//
// winint.h - dim windows platform
#pragma once

#include "core/task.h"
#include "core/timer.h"

#include <cstdint>
#include <functional>
#include <iostream>
#include <memory> // pointer_traits
#include <type_traits>

namespace Dim {

enum class PlatformInit;


/****************************************************************************
*
*   Error
*
***/

class WinError {
public:
    enum NtStatus : unsigned {};
    enum SecurityStatus : unsigned {};
    enum HResult : unsigned {};

public:
    // default constructor calls GetLastError()
    WinError();
    WinError(const WinError & from) = default;
    WinError(int error);
    // sets equivalent standard windows error value
    WinError(NtStatus status);
    WinError(SecurityStatus status);
    WinError(HResult result);

    WinError & operator=(const WinError & from) = default;
    WinError & operator=(int error) { return *this = WinError{error}; }
    WinError & operator=(NtStatus status) { return *this = WinError{status}; }
    WinError & operator=(SecurityStatus status) {
        return *this = WinError{status};
    }
    WinError & operator=(HResult result) { return *this = WinError{result}; }

    WinError & set();   // calls GetLastError()

    operator int() const { return m_value; }
    explicit operator NtStatus() const { return (NtStatus) m_ntStatus; }
    explicit operator SecurityStatus() const {
        return (SecurityStatus) m_secStatus;
    }

    std::error_code code() const {
        return {m_value, std::system_category()};
    }

private:
    friend std::ostream & operator<<(std::ostream & os, const WinError & val);

private:
    int m_value;
    int m_ntStatus{0};
    int m_secStatus{0};
};

void winErrorInitialize();


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
    FN * fn,
    const char lib[],
    const char proc[],
    bool optional = false
) {
    // FN must be pointer to function, which will be set to the address of
    // the procedure being loaded.
    static_assert(std::is_pointer_v<FN>);
    static_assert(std::is_function_v<
        typename std::pointer_traits<FN>::element_type
    >);
    *fn = (FN) winLoadProc(lib, proc, optional);
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
    void * completionKey{};
    ITaskNotify * notify{};
    TaskQueueHandle hq{};
};
// Allow reinterpret_cast to/from OVERLAPPED
static_assert(std::is_standard_layout_v<WinOverlappedEvent>);
static_assert(offsetof(WinOverlappedEvent, overlapped) == 0);

class IWinOverlappedNotify : public ITaskNotify {
public:
    IWinOverlappedNotify(TaskQueueHandle hq = {});
    OVERLAPPED & overlapped() { return m_evt.overlapped; }
    void * overlappedKey() { return m_evt.completionKey; }
    TaskQueueHandle overlappedQueue() { return m_evt.hq; }
    void pushOverlappedTask(void * completionKey = {});

    // Get error and number of bytes transferred for the operation
    struct WinOverlappedResult { WinError err; DWORD bytes; };
    WinOverlappedResult decodeOverlappedResult();

    void onTask() override = 0;

private:
    WinOverlappedEvent m_evt;
};

//---------------------------------------------------------------------------
// Privileges

bool winEnablePrivilege(const wchar_t name[], bool enable = true);

//---------------------------------------------------------------------------
// Accounts

class WinSid {
public:
    WinSid() = default;
    WinSid(const WinSid & from) = default;
    WinSid & operator=(const WinSid & right) = default;
    WinSid & operator=(const SID & sid);

    explicit operator bool() const;
    bool operator==(const WinSid & right) const;
    bool operator==(WELL_KNOWN_SID_TYPE type) const;

    SID * operator->() { return &m_data.Sid; }
    const SID * operator->() const { return &m_data.Sid; }
    SID & operator*() { return *operator->(); }
    const SID & operator*() const { return *operator->(); }

    void reset();

private:
    SE_SID m_data {};
};

WinSid winCreateSid(std::string_view name);
WinSid winCreateSid(WELL_KNOWN_SID_TYPE type, const SID * domain = nullptr);
bool winGetAccountName(
    std::string * name,
    std::string * domain,
    const SID & sid
);

std::string toString(const SID & sid);
bool parse(WinSid * out, std::string_view src);


/****************************************************************************
*
*   Console
*
***/

void winConsoleInitialize();


/****************************************************************************
*
*   Crash
*
***/

void winCrashInitialize(PlatformInit phase);
void winCrashInitializeThread();


/****************************************************************************
*
*   Debug
*
***/

void winDebugInitialize(PlatformInit phase);


/****************************************************************************
*
*   Env
*
***/

void winEnvInitialize();


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

class WinEventWaitNotify : public IWinOverlappedNotify {
public:
    // Registers wait unless evt is INVALID_HANDLE_VALUE.
    WinEventWaitNotify(
        std::function<void()> fn,
        TaskQueueHandle hq = {},
        HANDLE evt = {}
    );
    ~WinEventWaitNotify();

    // Must not already have a registered handle. Does nothing (and can be
    // called again) if evt is INVALID_HANDLE_VALUE, creates and registers new
    // event handle if evt is {}, otherwise registers evt.
    void registerWait(HANDLE evt, bool once = false);

private:
    void onTask() override;

    HANDLE m_registeredWait{};
    std::function<void()> m_fn;
};


/****************************************************************************
*
*   Exec
*
***/

void winExecInitialize();


/****************************************************************************
*
*   File
*
***/

void winFileMonitorInitialize();
bool winFileSetErrno(int error);


/****************************************************************************
*
*   Gui
*
***/

void winGuiInitialize(PlatformInit phase);


/****************************************************************************
*
*   Iocp
*
***/

void winIocpInitialize();

HANDLE winIocpHandle();
bool winIocpBindHandle(HANDLE handle, void * key = nullptr);


/****************************************************************************
*
*   Service
*
***/

void winServiceInitialize();


/****************************************************************************
*
*   Time
*
***/

Dim::Duration duration(const FILETIME & ft);


} // namespace
