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
*   Private
*
***/

namespace {

struct EnumMap {
    int value;
    DWORD osvalue;
};

} // namespace

const EnumMap s_svcTypes[] = {
    { (int) WinServiceConfig::Type::kOwn, SERVICE_WIN32_OWN_PROCESS },
    { (int) WinServiceConfig::Type::kShare, SERVICE_WIN32_SHARE_PROCESS },
    { (int) WinServiceConfig::Type::kKernel, SERVICE_KERNEL_DRIVER },
    { (int) WinServiceConfig::Type::kFileSys, SERVICE_FILE_SYSTEM_DRIVER },
    { (int) WinServiceConfig::Type::kRecognizer, SERVICE_RECOGNIZER_DRIVER },
    { (int) WinServiceConfig::Type::kAdapter, SERVICE_ADAPTER },
    { (int) WinServiceConfig::Type::kUserOwn, SERVICE_USER_OWN_PROCESS },
    { (int) WinServiceConfig::Type::kUserShare, SERVICE_USER_SHARE_PROCESS },
};
const EnumMap s_svcStarts[] = {
    { (int) WinServiceConfig::Start::kAuto, SERVICE_AUTO_START },
    { (int) WinServiceConfig::Start::kAutoDelayed, SERVICE_AUTO_START },
    { (int) WinServiceConfig::Start::kBoot, SERVICE_BOOT_START },
    { (int) WinServiceConfig::Start::kDemand, SERVICE_DEMAND_START },
    { (int) WinServiceConfig::Start::kDisabled, SERVICE_DISABLED },
    { (int) WinServiceConfig::Start::kSystem, SERVICE_SYSTEM_START },
};
const EnumMap s_svcErrCtrls[] = {
    { (int) WinServiceConfig::ErrCtrl::kIgnore, SERVICE_ERROR_IGNORE },
    { (int) WinServiceConfig::ErrCtrl::kNormal, SERVICE_ERROR_NORMAL },
    { (int) WinServiceConfig::ErrCtrl::kSevere, SERVICE_ERROR_SEVERE },
    { (int) WinServiceConfig::ErrCtrl::kCritical, SERVICE_ERROR_CRITICAL },
};


/****************************************************************************
*
*   Helpers
*
***/

//===========================================================================
template<typename E, typename T>
static DWORD getOsValue(const T & tbl, E value, DWORD def = 0) {
    if (value == E{} && !def)
        value = decltype(value)::kDefault;
    for (auto && v : tbl) {
        if (v.value == (int) value)
            return v.osvalue;
    }
    return def;
}

//===========================================================================
template<typename E, typename T>
static E getValue(const T & tbl, DWORD osvalue, E def = {}) {
    for (auto && v : tbl) {
        if (v.osvalue == osvalue)
            return (E) v.value;
    }
    return def;
}


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

    auto svcType = getOsValue(s_svcTypes, conf.serviceType);
    auto svcStart = getOsValue(s_svcStarts, conf.startType);
    auto svcErrCtrl = getOsValue(s_svcErrCtrls, conf.errorControl);

    DWORD tagId = conf.loadOrderTag;
    if (conf.loadOrderTag) {
        if (!conf.loadOrderGroup) {
            logMsgError() << "winInstallService: "
                "load order tag without load order group";
            return false;
        }
        if (conf.startType != WinServiceConfig::Start::kBoot
            || conf.startType != WinServiceConfig::Start::kSystem
        ) {
            logMsgError() << "winInstallService: load order tag requires "
                "start type of boot or system";
            return false;
        }
    }

    auto s = CreateService(
        scm,
        conf.serviceName,
        conf.displayName,
        SERVICE_ALL_ACCESS,
        svcType,
        svcStart,
        svcErrCtrl,
        conf.progWithArgs,
        conf.loadOrderGroup,
        tagId ? &tagId : NULL,
        depv.data(),
        conf.account,
        conf.password
    );
    if (!s) {
        logMsgError() << "CreateService(" << conf.serviceName << "): "
            << WinError{};
        return false;
    }
    Finally s_f([=]() { CloseServiceHandle(s); });
    Finally s_d([=]() { DeleteService(s); });

    if (conf.startType == WinServiceConfig::Start::kAutoDelayed) {
        SERVICE_DELAYED_AUTO_START_INFO dsi = {};
        dsi.fDelayedAutostart = true;
        if (!ChangeServiceConfig2(
            s,
            SERVICE_CONFIG_DELAYED_AUTO_START_INFO,
            &dsi
        )) {
            logMsgError() << "ChangeServiceConfig2(DELAYED_START): "
                << WinError{};
        }
    }

    if (conf.desc) {
        SERVICE_DESCRIPTION sd = {};
        sd.lpDescription = (char *) conf.desc;
        if (!ChangeServiceConfig2(s, SERVICE_CONFIG_DESCRIPTION, &sd)) {
            logMsgError() << "ChangeServiceConfig2(DESCRIPTION): "
                << WinError{};
            return false;
        }
    }

    if (conf.failureFlag) {
        SERVICE_FAILURE_ACTIONS_FLAG faf = {};
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
    logMsgInfo() << conf.serviceName << " service created.";
    return true;
}
