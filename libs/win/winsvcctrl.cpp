// Copyright Glen Knowles 2018.
// Distributed under the Boost Software License, Version 1.0.
//
// winsvcctrl.cpp - dim windows platform
#include "pch.h"
#pragma hdrstop

using namespace std;
using namespace std::chrono;
using namespace Dim;


/****************************************************************************
*
*   Public API
*
***/

//===========================================================================
bool Dim::winSvcInstall(const WinServiceConfig & sconf) {
    auto conf{sconf};

    if (!conf.progWithArgs)
        conf.progWithArgs = envExecPath().c_str();
    if (!conf.displayName)
        conf.displayName = conf.serviceName;
    assert(conf.serviceName && *conf.serviceName);
    auto scm = OpenSCManager(NULL, NULL, SC_MANAGER_CREATE_SERVICE);
    if (!scm) {
        logMsgError() << "OpenSCManager(CREATE_SERVICE): " << WinError{};
        return false;
    }
    Finally scm_f([=]() { CloseServiceHandle(scm); });

    CharBuf depv;
    for (auto && dep : conf.deps) {
        depv.append(dep).append(1, '\0');
    }
    depv.append(1, '\0');

    auto s = CreateService(
        scm,
        conf.serviceName,
        conf.displayName,
        SERVICE_ALL_ACCESS,
        SERVICE_WIN32_OWN_PROCESS,
        SERVICE_AUTO_START,
        SERVICE_ERROR_NORMAL,
        conf.progWithArgs,
        NULL,   // load order group
        NULL,   // load order group tag
        depv.data(),
        conf.account,
        conf.password
    );
    if (!s) {
        logMsgError() << "CreateService: " << WinError{};
        return false;
    }
    Finally s_f([=]() { CloseServiceHandle(s); });
    Finally s_d([=]() { DeleteService(s); });

    if (conf.desc) {
        SERVICE_DESCRIPTION sd;
        sd.lpDescription = (char *) conf.desc;
        if (!ChangeServiceConfig2(s, SERVICE_CONFIG_DESCRIPTION, &sd)) {
            logMsgError() << "ChangeServiceConfig2(DESCRIPTION): "
                << WinError{};
            return false;
        }
    }

    if (conf.failureFlag) {
        SERVICE_FAILURE_ACTIONS_FLAG faf;
        faf.fFailureActionsOnNonCrashFailures = true;
        if (!ChangeServiceConfig2(s, SERVICE_CONFIG_FAILURE_ACTIONS_FLAG, &faf)) {
            logMsgError() << "ChangeServiceConfig2(FAILURE_FLAG): " << WinError{};
            return false;
        }
    }

    vector<SC_ACTION> facts;
    for (auto && fa : conf.failureActions) {
        auto & sc = facts.emplace_back();
        sc.Delay = (DWORD) duration_cast<milliseconds>(fa.delay).count();
        switch (fa.type) {
        case fa.kInvalid: [[fallthrough]];
        case fa.kNone: sc.Type = SC_ACTION_NONE; break;
        case fa.kRestart: sc.Type = SC_ACTION_RESTART; break;
        case fa.kReboot: sc.Type = SC_ACTION_REBOOT; break;
        case fa.kRunCommand: sc.Type = SC_ACTION_RUN_COMMAND; break;
        }
    }
    SERVICE_FAILURE_ACTIONS fa = {};
    fa.dwResetPeriod =
        (DWORD) duration_cast<seconds>(conf.failureReset).count();
    fa.cActions = (DWORD) size(facts);
    fa.lpsaActions = data(facts);
    if (!ChangeServiceConfig2(s, SERVICE_CONFIG_FAILURE_ACTIONS, &fa)) {
        logMsgError() << "ChangeServiceConfig2(FAILURE_ACTIONS): "
            << WinError{};
        return false;
    }

    s_d = {};
    logMsgInfo() << "Service created.";
    return true;
}
