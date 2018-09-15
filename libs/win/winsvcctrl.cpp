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
const EnumMap s_svcSidTypes[] = {
    { (int) WinServiceConfig::SidType::kNone, SERVICE_SID_TYPE_NONE },
    { (int) WinServiceConfig::SidType::kUnrestricted,
        SERVICE_SID_TYPE_UNRESTRICTED },
    { (int) WinServiceConfig::SidType::kRestricted,
        SERVICE_SID_TYPE_RESTRICTED },
};


/****************************************************************************
*
*   Helpers
*
***/

//===========================================================================
template<typename E, typename T>
static E getValue(const T & tbl, DWORD osvalue, E def = {}) {
    for (auto && v : tbl) {
        if (v.osvalue == osvalue)
            return (E) v.value;
    }
    return def;
}

//===========================================================================
template<typename E, typename T>
static DWORD getOsValue(const T & tbl, E value) {
    if (value == E{})
        value = decltype(value)::kDefault;
    for (auto && v : tbl) {
        if (v.value == (int) value)
            return v.osvalue;
    }
    return 0;
}

//===========================================================================
template<typename T>
static CharBuf toMultiString(const T & strings) {
    CharBuf out;
    for (auto && str : strings)
        out.append(str).pushBack('\0');
    out.pushBack('\0');
    return out;
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

    auto deps = toMultiString(conf.deps);

    auto svcType = getOsValue(s_svcTypes, conf.serviceType);
    auto startType = getOsValue(s_svcStarts, conf.startType);
    auto errCtrl = getOsValue(s_svcErrCtrls, conf.errorControl);

    DWORD tagId = conf.loadOrderTag;
    if (conf.loadOrderTag) {
        if (!conf.loadOrderGroup) {
            logMsgError() << "winInstallService: "
                "load order tag without load order group";
            return false;
        }
        if (conf.startType != WinServiceConfig::Start::kBoot
            && conf.startType != WinServiceConfig::Start::kSystem
        ) {
            logMsgError() << "winInstallService: load order tag without "
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
        startType,
        errCtrl,
        conf.progWithArgs,
        conf.loadOrderGroup,
        tagId ? &tagId : NULL,
        deps.data(),
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
        if (!ChangeServiceConfig2(
            s,
            SERVICE_CONFIG_FAILURE_ACTIONS_FLAG,
            &faf
        )) {
            logMsgError() << "ChangeServiceConfig2(FAILURE_FLAG): "
                << WinError{};
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
    SERVICE_FAILURE_ACTIONSW fa = {};
    fa.dwResetPeriod =
        (DWORD) duration_cast<seconds>(conf.failureReset).count();
    wstring wreboot, wcmd;
    if (conf.rebootMsg) {
        wreboot = toWstring(conf.rebootMsg);
        fa.lpRebootMsg = (LPWSTR) wreboot.c_str();
    }
    if (conf.failureProgWithArgs) {
        wcmd = toWstring(conf.failureProgWithArgs);
        fa.lpCommand = (LPWSTR) wcmd.c_str();
    }
    fa.cActions = (DWORD) size(facts);
    fa.lpsaActions = data(facts);
    if (!ChangeServiceConfig2(s, SERVICE_CONFIG_FAILURE_ACTIONS, &fa)) {
        logMsgError() << "ChangeServiceConfig2(FAILURE_ACTIONS): "
            << WinError{};
        return false;
    }

    if (conf.preferredNode != -1) {
        SERVICE_PREFERRED_NODE_INFO ni = {};
        ni.usPreferredNode = (USHORT) conf.preferredNode;
        if (!ChangeServiceConfig2(s, SERVICE_CONFIG_PREFERRED_NODE, &ni)) {
            logMsgError() << "ChangeServiceConfig2(PREFERRED_NODE): "
                << WinError{};
            return false;
        }
    }

    if (conf.preshutdownTimeout.count()) {
        SERVICE_PRESHUTDOWN_INFO pi = {};
        pi.dwPreshutdownTimeout = (DWORD) duration_cast<milliseconds>(
            conf.preshutdownTimeout).count();
        if (!ChangeServiceConfig2(s, SERVICE_CONFIG_PRESHUTDOWN_INFO, &pi)) {
            logMsgError() << "ChangeServiceConfig2(PRESHUTDOWN_INFO): "
                << WinError{};
            return false;
        }
    }

    if (conf.sidType != WinServiceConfig::SidType::kInvalid
        && conf.sidType != WinServiceConfig::SidType::kDefault
    ) {
        SERVICE_SID_INFO si = {};
        si.dwServiceSidType = getOsValue(s_svcSidTypes, conf.sidType);
        if (!ChangeServiceConfig2(s, SERVICE_CONFIG_SERVICE_SID_INFO, &si)) {
            logMsgError() << "ChangeServiceConfig2(SID_INFO): " << WinError{};
            return false;
        }
    }

    if (!conf.privs.empty()) {
        SERVICE_REQUIRED_PRIVILEGES_INFO pi = {};
        auto privs = toMultiString(conf.privs);
        pi.pmszRequiredPrivileges = privs.data();
        if (!ChangeServiceConfig2(
            s,
            SERVICE_CONFIG_REQUIRED_PRIVILEGES_INFO,
            &pi
        )) {
            logMsgError() << "ChangeServiceConfig2(REQUIRED_PRIVILEGES): "
                << WinError{};
            return false;
        }
    }

    s_d = {};
    logMsgInfo() << conf.serviceName << " service created.";
    return true;
}
