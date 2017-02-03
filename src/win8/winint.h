// winint.h - dim core - windows platform
#pragma once

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
    HANDLE event = INVALID_HANDLE_VALUE);


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
*   Iocp
*
***/

void winIocpInitialize();

bool winIocpBindHandle(HANDLE handle);


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
    enum NtStatus : unsigned;

public:
    // default constructor calls GetLastError()
    WinError();
    WinError(NtStatus status);
    WinError(int error);

    WinError & operator=(int error);
    // sets equivalent standard windows error value
    WinError & operator=(NtStatus status);

    operator int() const { return m_value; }

private:
    int m_value;
};

std::ostream & operator<<(std::ostream & os, const WinError & val);

void winErrorInitialize();


/****************************************************************************
*
*   Socket
*
***/

SOCKET winSocketCreate();
SOCKET winSocketCreate(const Endpoint & localEnd);

} // namespace
