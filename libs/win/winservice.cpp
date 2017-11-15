// Copyright Glen Knowles 2017.
// Distributed under the Boost Software License, Version 1.0.
//
// winservice.cpp - dim windows platform
#include "pch.h"
#pragma hdrstop

using namespace std;
using namespace Dim;


/****************************************************************************
*
*   Declarations
*
***/

namespace {

class ReportStatusTask : public ITaskNotify {
    void onTask() override;
};

} // namespace


/****************************************************************************
*
*   Variables
*
***/

static SERVICE_STATUS_HANDLE s_hstat;
static ReportStatusTask s_reportTask;

static mutex s_mut;
static condition_variable s_cv;
static RunMode s_mode{kRunStopped};


/****************************************************************************
*
*   Helpers
*
***/

//===========================================================================
static void setState(unsigned status) {
    SERVICE_STATUS ss = {
        SERVICE_WIN32_OWN_PROCESS,
        status,
        SERVICE_ACCEPT_STOP,
        NO_ERROR,
        0, // service specific exit code
        0, // check point
        30'000 // 30 second wait
    };
    SetServiceStatus(s_hstat, &ss);
}

//===========================================================================
static DWORD WINAPI svcCtrlHandler(
    DWORD control,
    DWORD type,
    LPVOID data,
    LPVOID context
) {
    switch (control) {
    default:
        return ERROR_CALL_NOT_IMPLEMENTED;
    case SERVICE_CONTROL_INTERROGATE:
        return NO_ERROR;
    case SERVICE_CONTROL_STOP:
        appSignalShutdown();
        return NO_ERROR;
    }
}

//===========================================================================
static VOID WINAPI ServiceMain(DWORD argc, LPTSTR * argv) {
    // Running as a service and services can't have GUI windows
    iAppSetFlags(appFlags() & ~fAppWithGui | fAppIsService);

    s_hstat = RegisterServiceCtrlHandlerEx(
        "",
        &svcCtrlHandler,
        NULL
    );
    if (!s_hstat)
        logMsgCrash() << "RegisterServiceCtrlHandlerEx: " << WinError{};

    assert(appStarting());
    setState(SERVICE_START_PENDING);
    iAppPushStartupTask(s_reportTask);

    {
        scoped_lock<mutex> lk{s_mut};
        s_mode = kRunRunning;
    }
    s_cv.notify_one();
}


/****************************************************************************
*
*   Service dispatch thread
*
***/

//===========================================================================
static void serviceDispatchThread() {
    assert(s_mode == kRunStarting);

    SERVICE_TABLE_ENTRY st[] = {
        { (char *) "", &ServiceMain },
        { NULL, NULL },
    };
    // StartServiceCtrlDispatcher starts the service message loop and doesn't
    // return until the service ends
    if (!StartServiceCtrlDispatcher(st)) {
        WinError err;
        if (err == ERROR_FAILED_SERVICE_CONTROLLER_CONNECT) {
            // failed service controller connect means that we're not running
            // as a service.
        } else {
            logMsgCrash() << "StartServiceCtrlDispatcher: " << err;
        }
    }

    {
        scoped_lock<mutex> lk{s_mut};
        s_mode = kRunStopped;
    }
    s_cv.notify_one();
}


/****************************************************************************
*
*   ReportStatusTask
*
***/

//===========================================================================
void ReportStatusTask::onTask() {
    setState(SERVICE_RUNNING);
}


/****************************************************************************
*
*   Shutdown notify
*
***/

namespace {
class ShutdownNotify : public IShutdownNotify {
    void onShutdownClient(bool firstTry) override;
    void onShutdownConsole(bool firstTry) override;
};
} // namespace
static ShutdownNotify s_cleanup;

//===========================================================================
void ShutdownNotify::onShutdownClient(bool firstTry) {
    if (appFlags() & fAppIsService)
        setState(SERVICE_STOP_PENDING);
}

//===========================================================================
void ShutdownNotify::onShutdownConsole(bool firstTry) {
    if (firstTry && (appFlags() & fAppIsService))
        setState(SERVICE_STOPPED);

    scoped_lock<mutex> lk{s_mut};
    if (s_mode != kRunStopped)
        return shutdownIncomplete();
}


/****************************************************************************
*
*   Public API
*
***/

//===========================================================================
void Dim::winServiceInitialize() {
    if (appFlags() & fAppWithService) {
        shutdownMonitor(&s_cleanup);

        // Start dispatch thread and wait for it to determine if we're
        // running as a service.
        s_mode = kRunStarting;
        taskPushOnce("Service Dispatcher", serviceDispatchThread);
        unique_lock<mutex> lk{s_mut};
        while (s_mode == kRunStarting)
            s_cv.wait(lk);
    }
}
