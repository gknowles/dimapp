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

class ServiceDispatchTask : public ITaskNotify {
public:
    void onTask() override;
};

} // namespace


/****************************************************************************
*
*   Variables
*
***/

static SERVICE_STATUS_HANDLE s_hstat;
static ServiceDispatchTask s_dispatchTask;
static TaskQueueHandle s_taskq;

static mutex s_mut;
static bool s_stopped;


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
    void * data,
    void * context
) {
    switch (control) {
    case SERVICE_CONTROL_INTERROGATE:
        return NO_ERROR;
    case SERVICE_CONTROL_STOP:
        break;
    default:
        return ERROR_CALL_NOT_IMPLEMENTED;
    }

    // stop service
    appSignalShutdown();
    setState(SERVICE_STOPPED);
    return NO_ERROR;
}

//===========================================================================
static void WINAPI ServiceMain(DWORD argc, char ** argv) {
    s_hstat = RegisterServiceCtrlHandlerEx(
        "",
        &svcCtrlHandler,
        NULL
    );
    if (!s_hstat)
        logMsgCrash() << "RegisterServiceCtrlHandlerEx: " << WinError{};

    setState(SERVICE_START_PENDING);
}


/****************************************************************************
*
*   ServiceDispatchTask
*
***/

//===========================================================================
void ServiceDispatchTask::onTask() {
    assert(!s_stopped);

    SERVICE_TABLE_ENTRY st[] = {
        { (char *) "", &ServiceMain },
        { NULL, NULL },
    };
    if (!StartServiceCtrlDispatcher(st)) {
        WinError err;
        if (err != ERROR_FAILED_SERVICE_CONTROLLER_CONNECT) 
            logMsgCrash() << "StartServiceCtrlDispatcher: " << err;
    }
    
    lock_guard<mutex> lk{s_mut};
    s_stopped = true;
}


/****************************************************************************
*
*   Shutdown notify
*
***/

namespace {
class ShutdownNotify : public IShutdownNotify {
    void onShutdownConsole(bool firstTry) override;
};
} // namespace
static ShutdownNotify s_cleanup;

//===========================================================================
void ShutdownNotify::onShutdownConsole(bool firstTry) {
    if (firstTry)
        setState(SERVICE_STOPPED);

    lock_guard<mutex> lk{s_mut};
    if (!s_stopped)
        return shutdownIncomplete();

    // reset so app rerun can work
    s_stopped = false;
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
        s_taskq = taskCreateQueue("Service Dispatcher", 1);
        taskPush(s_taskq, s_dispatchTask);
    }
}
