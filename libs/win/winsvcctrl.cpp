// Copyright Glen Knowles 2018.
// Distributed under the Boost Software License, Version 1.0.
//
// winsvcctrl.cpp - dim windows platform
#include "pch.h"
#pragma hdrstop

using namespace std;
using namespace Dim;


/****************************************************************************
*
*   Variables
*
***/


/****************************************************************************
*
*   Helpers
*
***/


/****************************************************************************
*
*   Public API
*
***/

//===========================================================================
bool Dim::winSvcInstall(
    const char path[],
    const char name[],
    const char displayName[],
    const char desc[],
    const vector<const char *> & deps,
    const char account[],
    const char password[]
) {
    if (!path)
        path = envExecPath().c_str();
    if (!displayName)
        displayName = name;
    assert(name && *name);
    auto scm = OpenSCManager(NULL, NULL, SC_MANAGER_CREATE_SERVICE);
    if (!scm) {
        logMsgError() << "OpenSCManager(CREATE_SERVICE): " << WinError{};
        return false;
    }
    Finally scm_f([=]() { CloseServiceHandle(scm); });

    CharBuf depv;
    for (auto && dep : deps) {
        depv.append(dep).append(1, '\0');
    }
    depv.append(1, '\0');

    auto s = CreateService(
        scm,
        name,
        displayName,
        SERVICE_ALL_ACCESS,
        SERVICE_WIN32_OWN_PROCESS,
        SERVICE_AUTO_START,
        SERVICE_ERROR_NORMAL,
        path,
        NULL,   // load order group
        NULL,   // load order group tag
        depv.data(),
        account,
        password
    );
    if (!s) {
        logMsgError() << "CreateService: " << WinError{};
        return false;
    }
    Finally s_f([=]() { CloseServiceHandle(s); });
    Finally s_d([=]() { DeleteService(s); });

    if (desc) {
        SERVICE_DESCRIPTION sd;
        sd.lpDescription = (char *) desc;
        if (!ChangeServiceConfig2(s, SERVICE_CONFIG_DESCRIPTION, &sd)) {
            logMsgError() << "ChangeServiceConfig2(DESCRIPTION): "
                << WinError{};
            return false;
        }
    }

    SERVICE_FAILURE_ACTIONS_FLAG faf;
    faf.fFailureActionsOnNonCrashFailures = true;
    if (!ChangeServiceConfig2(s, SERVICE_CONFIG_FAILURE_ACTIONS_FLAG, &faf)) {
        logMsgError() << "ChangeServiceConfig2(FAILURE_FLAG): " << WinError{};
        return false;
    }

    SC_ACTION facts[] = {
        { SC_ACTION_RESTART, 10 * 1000 }, // 10 seconds, in milliseconds
        { SC_ACTION_RESTART, 60 * 1000 }, // 1 minute, in milliseconds
        { SC_ACTION_RESTART, 10 * 60 * 1000 }, // 10 minutes, in milliseconds
    };
    SERVICE_FAILURE_ACTIONS fa = {};
    fa.dwResetPeriod = 24 * 60 * 60; // 1 day, in seconds
    fa.lpCommand = (char *) "";
    fa.cActions = (DWORD) size(facts);
    fa.lpsaActions = facts;
    if (!ChangeServiceConfig2(s, SERVICE_CONFIG_FAILURE_ACTIONS, &fa)) {
        logMsgError() << "ChangeServiceConfig2(FAILURE_ACTIONS): "
            << WinError{};
        return false;
    }

    s_d = {};
    logMsgInfo() << "Service created.";
    return true;
}

