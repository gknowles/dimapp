// Copyright Glen Knowles 2015 - 2018.
// Distributed under the Boost Software License, Version 1.0.
//
// winiocp.cpp - dim windows platform
#include "pch.h"
#pragma hdrstop

using namespace std;
using namespace Dim;


/****************************************************************************
*
*   Variables
*
***/

static HANDLE s_iocp;
static mutex s_mut;


/****************************************************************************
*
*   IOCP completion thread
*
***/

//===========================================================================
static void iocpDispatchThread() {
    const int kMaxEntries = 8;
    OVERLAPPED_ENTRY entries[kMaxEntries];
    ULONG found;
    ITaskNotify * tasks[size(entries)];
    bool used[size(entries)];
    for (;;) {
        if (!GetQueuedCompletionStatusEx(
            s_iocp,
            entries,
            (ULONG) size(entries),
            &found,
            INFINITE,   // timeout
            false       // alertable
        )) {
            WinError err;
            if (err == ERROR_ABANDONED_WAIT_0) {
                // Completion port closed while inside get status.
                break;
            } else if (err == ERROR_INVALID_HANDLE) {
                // Completion port closed before call to get status.
                break;
            } else {
                logMsgFatal() << "GetQueuedCompletionStatusEx: " << err;
            }
        }

        memset(used, 0, sizeof(used));
        for (;;) {
            int pos = 0;
            int match = numeric_limits<int>::max();
            for (unsigned i = 0; i < found; ++i) {
                if (used[i])
                    continue;
                auto evt = reinterpret_cast<WinOverlappedEvent *>(
                    entries[i].lpOverlapped
                );
                evt->completionKey = (void *) entries[i].lpCompletionKey;
                auto val = evt->hq.pos;
                if (match == numeric_limits<int>::max())
                    match = val;
                if (val == match) {
                    tasks[pos++] = evt->notify;
                    used[i] = true;
                }
            }
            if (!pos)
                break;
            if (!match) {
                taskPushEvent(tasks, pos);
            } else {
                TaskQueueHandle hq;
                hq.pos = match;
                taskPush(hq, tasks, pos);
            }
        }
    }

    scoped_lock<mutex> lk{s_mut};
    s_iocp = 0;
}


/****************************************************************************
*
*   Shutdown monitor
*
***/

namespace {

class ShutdownNotify : public IShutdownNotify {
    void onShutdownConsole(bool firstTry) override;

    bool m_closing{false};
};

} // namespace

static ShutdownNotify s_cleanup;

//===========================================================================
void ShutdownNotify::onShutdownConsole(bool firstTry) {
    if (firstTry)
        return shutdownIncomplete();

    if (!m_closing) {
        m_closing = true;
        if (!CloseHandle(s_iocp))
            logMsgError() << "CloseHandle(IOCP): " << WinError{};

        s_iocp = INVALID_HANDLE_VALUE;
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
        0     // concurrent threads, 0 for default
    );
    if (!s_iocp)
        logMsgFatal() << "CreateIoCompletionPort(null): " << WinError{};

    // Start IOCP dispatch task
    taskPushOnce("IOCP Dispatch", iocpDispatchThread);
}

//===========================================================================
HANDLE Dim::winIocpHandle() {
    assert(s_iocp);
    return s_iocp;
}

//===========================================================================
bool Dim::winIocpBindHandle(HANDLE handle) {
    assert(s_iocp);

    if (!CreateIoCompletionPort(handle, s_iocp, NULL, 0)) {
        logMsgError() << "CreateIoCompletionPort(handle): " << WinError{};
        return false;
    }

    if (!SetFileCompletionNotificationModes(
        handle,
        FILE_SKIP_SET_EVENT_ON_HANDLE
    )) {
        logMsgError() << "SetFileCompletionNotificationModes(SKIP_EVENT): "
            << WinError{};
        return false;
    }

    return true;
}
