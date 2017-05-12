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
*   Tuning parameters
*
***/


/****************************************************************************
*
*   Declarations
*
***/


/****************************************************************************
*
*   Variables
*
***/

static SERVICE_STATUS_HANDLE s_hstat;


/****************************************************************************
*
*   Helpers
*
***/

//===========================================================================
DWORD WINAPI svcCtrlHandler(
    DWORD control,
    DWORD type,
    void * data,
    void * context
) {
    return ERROR_CALL_NOT_IMPLEMENTED;
}

//===========================================================================
void WINAPI ServiceMain(DWORD argc, char ** argv) {
    s_hstat = RegisterServiceCtrlHandlerEx(
        "",
        &svcCtrlHandler,
        NULL
    );
    SERVICE_STATUS ss = {
        SERVICE_WIN32_OWN_PROCESS,
        SERVICE_START_PENDING,
        SERVICE_ACCEPT_STOP,
        NO_ERROR,
        0, // service specific exit code
        0, // check point
        30'000
    };
    SetServiceStatus(s_hstat, &ss);
}


/****************************************************************************
*
*   Service dispatch task
*
***/

namespace {

class ServiceDispatchTask : public ITaskNotify {
public:
    void onTask() override;
};

} // namespace
static ServiceDispatchTask s_dispatchTask;
static TaskQueueHandle s_taskq;

//===========================================================================
void ServiceDispatchTask::onTask() {
    SERVICE_TABLE_ENTRY st[] = {
        { (char *) "", &ServiceMain },
        { NULL, NULL },
    };
    if (!StartServiceCtrlDispatcher(st)) {
        WinError err;
        if (err != ERROR_FAILED_SERVICE_CONTROLLER_CONNECT) 
            logMsgCrash() << "StartServiceCtrlDispatcher: " << err;
    }

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
        return shutdownIncomplete();
}


/****************************************************************************
*
*   Public API
*
***/

//===========================================================================
void Dim::winServiceInitialize() {
    if (~appFlags() & fAppWithService)
        return;

    shutdownMonitor(&s_cleanup);
    s_taskq = taskCreateQueue("Service Dispatcher", 1);
    taskPush(s_taskq, s_dispatchTask);
}
