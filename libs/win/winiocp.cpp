// Copyright Glen Knowles 2015 - 2017.
// Distributed under the Boost Software License, Version 1.0.
//
// winiocp.cpp - dim windows platform
#include "pch.h"
#pragma hdrstop

using namespace std;
using namespace Dim;


/****************************************************************************
*
*   Incomplete public types
*
***/


/****************************************************************************
*
*   Private declarations
*
***/

namespace {} // namespace


/****************************************************************************
*
*   Variables
*
***/

static HANDLE s_iocp;
static mutex s_mut;


/****************************************************************************
*
*   Iocp thread
*
***/

//===========================================================================
static void iocpDispatchThread() {
    OVERLAPPED * overlapped;
    ULONG_PTR key;
    ULONG bytes;
    for (;;) {
        // TODO: use GetQueuedCompletionStatusEx and array version of
        // DimTaskPushEvent
        if (!GetQueuedCompletionStatus(
                s_iocp, &bytes, &key, &overlapped, INFINITE)) {
            WinError err;
            if (err == ERROR_ABANDONED_WAIT_0) {
                // completion port was closed
                break;
            } else if (err == ERROR_OPERATION_ABORTED) {
                // probably file handle was closed
            } else if (err == ERROR_HANDLE_EOF) {
                // probably read at end of file
            } else {
                logMsgCrash() << "GetQueuedCompletionStatus: " << err;
            }
        }

        auto evt = (WinOverlappedEvent *)overlapped;
        if (evt->hq) {
            taskPush(evt->hq, *evt->notify);
        } else {
            taskPushEvent(*evt->notify);
        }
    }

    lock_guard<mutex> lk{s_mut};
    s_iocp = 0;
}


/****************************************************************************
*
*   WinIocpShutdown
*
***/

namespace {
class WinIocpShutdown : public IShutdownNotify {
    void onShutdownConsole(bool retry) override;
};
static WinIocpShutdown s_cleanup;
} // namespace

//===========================================================================
void WinIocpShutdown::onShutdownConsole(bool retry) {
    if (!retry) {
        if (!CloseHandle(s_iocp))
            logMsgError() << "CloseHandle(iocp): " << WinError{};

        Sleep(0);
    }

    unique_lock<mutex> lk(s_mut);
    if (s_iocp)
        shutdownIncomplete();
}


/****************************************************************************
*
*   Internal API
*
***/

//===========================================================================
void Dim::winIocpInitialize() {
    shutdownMonitor(&s_cleanup);

    s_iocp = CreateIoCompletionPort(
        INVALID_HANDLE_VALUE,
        NULL, // existing port
        NULL, // completion key
        0     // num threads, 0 for default
        );
    if (!s_iocp)
        logMsgCrash() << "CreateIoCompletionPort(null): " << WinError{};

    thread thr{iocpDispatchThread};
    thr.detach();
}

//===========================================================================
bool Dim::winIocpBindHandle(HANDLE handle) {
    assert(s_iocp);

    if (!CreateIoCompletionPort(handle, s_iocp, NULL, 0)) {
        logMsgError() << "CreateIoCompletionPort(handle): " << WinError{};
        return false;
    }

    return true;
}
