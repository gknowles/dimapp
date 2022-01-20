// Copyright Glen Knowles 2017 - 2022.
// Distributed under the Boost Software License, Version 1.0.
//
// winsvc.cpp - dim windows platform
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
static void setState(
    unsigned status,
    bool accepting = true,
    int exitcode = EX_OK
) {
    DWORD accepts = SERVICE_ACCEPT_STOP;
    SERVICE_STATUS ss = {
        SERVICE_WIN32_OWN_PROCESS,
        status,
        accepting ? accepts : 0,
        ERROR_SERVICE_SPECIFIC_ERROR,
        (DWORD) exitcode, // service specific exit code
        0, // check point
        30'000 // 30 second wait
    };
    if (!SetServiceStatus(s_hstat, &ss))
        logMsgFatal() << "SetServiceStatus(" << status << "): " << WinError{};
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
    iAppSetFlags(appFlags().reset(fAppWithGui).set(fAppIsService));

    s_hstat = RegisterServiceCtrlHandlerExW(
        L"",
        &svcCtrlHandler,
        NULL
    );
    if (!s_hstat)
        logMsgFatal() << "RegisterServiceCtrlHandlerExW: " << WinError{};

    assert(appStarting());
    setState(SERVICE_RUNNING, false);
    iAppPushStartupTask(&s_reportTask);

    {
        scoped_lock lk{s_mut};
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

    SERVICE_TABLE_ENTRYW st[] = {
        { (LPWSTR) L"", &ServiceMain },
        { NULL, NULL },
    };
    // StartServiceCtrlDispatcher starts the service message loop and doesn't
    // return until the service ends
    if (!StartServiceCtrlDispatcherW(st)) {
        WinError err;
        if (err == ERROR_FAILED_SERVICE_CONTROLLER_CONNECT) {
            // failed service controller connect means that we're not running
            // as a service.
        } else {
            logMsgFatal() << "StartServiceCtrlDispatcherW: " << err;
        }
    }

    {
        scoped_lock lk{s_mut};
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
    // Start accepting service control requests
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
    if (appFlags().any(fAppIsService))
        setState(SERVICE_STOP_PENDING);
}

//===========================================================================
void ShutdownNotify::onShutdownConsole(bool firstTry) {
    if (firstTry && appFlags().any(fAppIsService))
        setState(SERVICE_STOPPED, false, appExitCode());

    scoped_lock lk{s_mut};
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
    if (appFlags().any(fAppWithService)) {
        shutdownMonitor(&s_cleanup);

        // Start dispatch thread and wait for it to determine if we're
        // running as a service.
        s_mode = kRunStarting;
        taskPushOnce("Service Dispatcher", serviceDispatchThread);
        unique_lock lk{s_mut};
        while (s_mode == kRunStarting)
            s_cv.wait(lk);
    }
}
