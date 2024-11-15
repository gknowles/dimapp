// Copyright Glen Knowles 2018 - 2024.
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

struct FuncInfo : public NoCopy {
    SC_HANDLE scm = {};
    SC_HANDLE svc = {};
    string svcName;
    bool logErrors = true;
    mutable vector<std::byte> buf;

    FuncInfo(string_view svcName);
    ~FuncInfo();
};

} // namespace

const EnumMap s_svcTypes[] = {
    { (int) WinServiceConfig::Type::kOwn, SERVICE_WIN32_OWN_PROCESS },

    // Must come after kOwn, as a kShared that happens to be running under
    // a dedicated svchost gets set to kOwn|kShared
    { (int) WinServiceConfig::Type::kShared, SERVICE_WIN32_SHARE_PROCESS },

    { (int) WinServiceConfig::Type::kKernel, SERVICE_KERNEL_DRIVER },
    { (int) WinServiceConfig::Type::kFileSys, SERVICE_FILE_SYSTEM_DRIVER },
    { (int) WinServiceConfig::Type::kRecognizer, SERVICE_RECOGNIZER_DRIVER },
    { (int) WinServiceConfig::Type::kAdapter, SERVICE_ADAPTER },

    // Must come after kOwn and kShared, as kUserOwn is kOwn or'd with
    // SERVICE_USER_SERVICE, similarly for kUserShared.
    { (int) WinServiceConfig::Type::kUserOwn, SERVICE_USER_OWN_PROCESS },
    { (int) WinServiceConfig::Type::kUserShared, SERVICE_USER_SHARE_PROCESS },
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
const EnumMap s_svcTriggerTypes[] = {
    { (int) WinServiceConfig::Trigger::Type::kCustom,
        SERVICE_TRIGGER_TYPE_CUSTOM },
};
const EnumMap s_svcLaunchProts[] = {
    { (int) WinServiceConfig::LaunchProt::kNone,
        SERVICE_LAUNCH_PROTECTED_NONE },
    { (int) WinServiceConfig::LaunchProt::kWindows,
        SERVICE_LAUNCH_PROTECTED_WINDOWS },
    { (int) WinServiceConfig::LaunchProt::kWindowsLight,
        SERVICE_LAUNCH_PROTECTED_WINDOWS_LIGHT },
    { (int) WinServiceConfig::LaunchProt::kAntimalwareLight,
        SERVICE_LAUNCH_PROTECTED_ANTIMALWARE_LIGHT },
};

const EnumMap s_svcStates[] = {
    { (int) WinServiceStatus::State::kContinuePending,
        SERVICE_CONTINUE_PENDING },
    { (int) WinServiceStatus::State::kPausePending, SERVICE_PAUSE_PENDING },
    { (int) WinServiceStatus::State::kPaused, SERVICE_PAUSED },
    { (int) WinServiceStatus::State::kRunning, SERVICE_RUNNING },
    { (int) WinServiceStatus::State::kStartPending, SERVICE_START_PENDING },
    { (int) WinServiceStatus::State::kStopPending, SERVICE_STOP_PENDING },
    { (int) WinServiceStatus::State::kStopped, SERVICE_STOPPED },
};

/****************************************************************************
*
*   Helpers
*
***/

#pragma warning(push)
#pragma warning(disable: 4996)

//===========================================================================
inline static string to_string(const wstring & str) {
	wstring_convert<codecvt_utf8<wchar_t>> wcvt;
	return wcvt.to_bytes(str);
}

//===========================================================================
inline static wstring to_wstring(const string & str) {
	wstring_convert<codecvt_utf8<wchar_t>> wcvt;
	return wcvt.from_bytes(str);
}

#pragma warning(pop)


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
template<int N>
static WinServiceConfig::Type getValue(
    const EnumMap tbl[N],
    DWORD osvalue,
    WinServiceConfig::Type def
) {
    auto value = def;
    for (auto&& em : tbl) {
        if ((osvalue & em.osvalue) == em.osvalue) {
            value =
                static_cast<WinServiceConfig::Type>(em.value);
            // Keep searching after a match, so a following kUserOwn or
            // kUserShare will take precedence over kOwn and kShare.
        }
    }
    return value;
}

//===========================================================================
template<typename E, typename T>
static DWORD getOsValue(const T & tbl, E value, bool enableDefault = true) {
    if (enableDefault && value == E{})
        value = decltype(value)::kDefault;
    for (auto && v : tbl) {
        if (v.value == (int) value)
            return v.osvalue;
    }
    return 0;
}

//===========================================================================
template<typename T>
static CharBuf toMultistring(const T & strings) {
    CharBuf out;
    for (auto && str : strings)
        out.append(str).pushBack('\0');
    out.pushBack('\0');
    return out;
}

//===========================================================================
static vector<string> splitMultistring(const wchar_t src[]) {
    vector<string> out;
    if (src) {
        auto ptr = src;
        while (*ptr) {
            auto eptr = wcschr(ptr, '\0');
            out.push_back(
                toString(wstring_view{ptr, size_t(eptr - ptr)})
            );
            ptr = eptr + 1;
        }
    }
    return out;
}

//===========================================================================
static error_code reportBadRange(
    const FuncInfo & info,
    string_view func,
    string_view arg2,
    string_view detail
) {
    if (info.logErrors) {
    }
    return make_error_code(errc::result_out_of_range);
}

//===========================================================================
static error_code reportError(
    const FuncInfo & info,
    string_view func,
    string_view arg2 = {}
) {
    WinError err;
    if (info.logErrors) {
        auto os = logMsgError();
        os << func << "(" << info.svcName;
        if (!arg2.empty())
            os << ", " << arg2;
        os << "): " << err;
    }
    return err.code();
}

//===========================================================================
static error_code reportChange2Error(
    const FuncInfo & info,
    string_view arg2
) {
    return reportError(info, "ChangeServiceConfig2W", arg2);
}

//===========================================================================
static error_code openScm(
    FuncInfo * info,
    DWORD access,
    const char infoName[]
) {
    info->scm = OpenSCManagerW(NULL, NULL, access);
    if (!info->scm)
        return reportError(*info, "OpenSCManagerW", infoName);
    return {};
}

//===========================================================================
static error_code openSvc(
    FuncInfo * info,
    DWORD access,
    const char infoName[]
) {
    info->svc = OpenServiceW(
        info->scm,
        toWstring(info->svcName).c_str(),
        access
    );
    if (!info->svc)
        return reportError(*info, "OpenServiceW", infoName);
    return {};
}

//===========================================================================
template<typename T>
static pair<T *, size_t> reserve(const FuncInfo & info, size_t bytes) {
    auto alignment = alignof(T);
    if (info.buf.empty()) {
        // According to Microsoft docs the maximum size of the buffer needed
        // for QueryServiceConfigW or QueryServiceConfig2W is 8k bytes.
        info.buf.resize(8192 + alignment);
    }
    if (bytes + alignment > info.buf.size()) {
        info.buf.resize(bytes + alignment);
    } else {
        bytes = info.buf.size() - alignment;
    }
    void * ptr = info.buf.data();
    auto count = info.buf.size();
    ptr = align(alignment, bytes, ptr, count);
    assert(ptr);
    return {reinterpret_cast<T *>(ptr), count};
}

//===========================================================================
template<typename T>
static pair<T *, size_t> reserve(const FuncInfo & info) {
    return reserve<T>(info, sizeof (T));
}

//===========================================================================
static pair<error_code, QUERY_SERVICE_CONFIGW *> queryConf(
    const FuncInfo & info
) {
    DWORD needed = 0;
    for (;;) {
        auto&& [cfg, count] =
            reserve<QUERY_SERVICE_CONFIGW>(info, needed);
        if (count > 8192) {
            // The maximum allowed size of this buffer is 8k.
            count = 8192;
        }
        if (QueryServiceConfigW(info.svc, cfg, (DWORD) count, &needed))
            return {{}, cfg};
        WinError err;
        if (err != ERROR_INSUFFICIENT_BUFFER)
            return {reportError(info, "QueryServiceConfigW", {}), nullptr};
    }
}

//===========================================================================
template<typename T>
static pair<error_code, T *> queryConf2(
    const FuncInfo & info,
    DWORD infoLvl,
    const char infoName[]
) {
    DWORD needed = 0;
    for (;;) {
        auto&& [cfg, count] = reserve<T>(info, needed);
        if (count > 8192) {
            // The maximum allowed size of this buffer is 8k.
            count = 8192;
        }
        if (QueryServiceConfig2W(
            info.svc,
            infoLvl,
            (BYTE *) cfg,
            (DWORD) count,
            &needed
        )) {
            return {{}, cfg};
        }
        WinError err;
        if (err != ERROR_INSUFFICIENT_BUFFER) {
            auto ec = reportError(info, "QueryServiceConfig2W", infoName);
            return {ec, nullptr};
        }
    }
}

//===========================================================================
static error_code changeAllConf2(
    const FuncInfo & info,
    const WinServiceConfig & conf
) {
    if (conf.startType == WinServiceConfig::Start::kAutoDelayed) {
        SERVICE_DELAYED_AUTO_START_INFO dsi = {};
        dsi.fDelayedAutostart = true;
        if (!ChangeServiceConfig2W(
            info.svc,
            SERVICE_CONFIG_DELAYED_AUTO_START_INFO,
            &dsi
        )) {
            return reportChange2Error(info, "DELAYED_START");
        }
    }

    if (conf.desc) {
        auto wdesc = toWstring(*conf.desc);
        SERVICE_DESCRIPTIONW sd = {};
        sd.lpDescription = wdesc.data();
        if (!ChangeServiceConfig2W(info.svc, SERVICE_CONFIG_DESCRIPTION, &sd))
            return reportChange2Error(info, "DESCRIPTION");
    }

    if (conf.failureFlag ==
            WinServiceConfig::FailureFlag::kCrashOrNonZeroExitCode
    ) {
        SERVICE_FAILURE_ACTIONS_FLAG faf = {};
        faf.fFailureActionsOnNonCrashFailures = true;
        if (!ChangeServiceConfig2W(
            info.svc,
            SERVICE_CONFIG_FAILURE_ACTIONS_FLAG,
            &faf
        )) {
            return reportChange2Error(info, "FAILURE_FLAG");
        }
    }

    if (conf.failureActions) {
        vector<SC_ACTION> facts;
        for (auto && fa : *conf.failureActions) {
            auto & sc = facts.emplace_back();
            sc.Delay = (DWORD) fa.delay.count();
            switch (fa.type) {
            case fa.kInvalid: [[fallthrough]];
            case fa.kNone: sc.Type = SC_ACTION_NONE; break;
            case fa.kRestart: sc.Type = SC_ACTION_RESTART; break;
            case fa.kReboot: sc.Type = SC_ACTION_REBOOT; break;
            case fa.kRunCommand: sc.Type = SC_ACTION_RUN_COMMAND; break;
            }
        }
        SERVICE_FAILURE_ACTIONSW fa = {};
        fa.dwResetPeriod = conf.failureReset
            ? (DWORD) conf.failureReset->count()
            : INFINITE;
        wstring wreboot, wcmd;
        if (conf.rebootMsg) {
            wreboot = toWstring(*conf.rebootMsg);
            fa.lpRebootMsg = wreboot.data();
        }
        if (!conf.failureProgWithArgs.empty()) {
            wcmd = toWstring(conf.failureProgWithArgs);
            fa.lpCommand = wcmd.data();
        }
        fa.cActions = (DWORD) size(facts);
        fa.lpsaActions = data(facts);
        if (!ChangeServiceConfig2W(
            info.svc,
            SERVICE_CONFIG_FAILURE_ACTIONS,
            &fa
        )) {
            return reportChange2Error(info, "FAILURE_ACTIONS");
        }
    }

    if (conf.preferredNode != -1) {
        SERVICE_PREFERRED_NODE_INFO ni = {};
        ni.usPreferredNode = (USHORT) conf.preferredNode;
        if (!ChangeServiceConfig2W(
            info.svc,
            SERVICE_CONFIG_PREFERRED_NODE,
            &ni
        )) {
            return reportChange2Error(info, "PREFERRED_NODE");
        }
    }

    if (conf.preshutdownTimeout) {
        SERVICE_PRESHUTDOWN_INFO pi = {};
        auto ms = conf.preshutdownTimeout->count();
        pi.dwPreshutdownTimeout = (DWORD) ms;
        if (!ChangeServiceConfig2W(
            info.svc,
            SERVICE_CONFIG_PRESHUTDOWN_INFO,
            &pi
        )) {
            return reportChange2Error(info, "PRESHUTDOWN_INFO");
        }
    }

    if (conf.sidType != WinServiceConfig::SidType::kInvalid
        && conf.sidType != WinServiceConfig::SidType::kDefault
    ) {
        SERVICE_SID_INFO si = {};
        si.dwServiceSidType = getOsValue(s_svcSidTypes, conf.sidType);
        if (!ChangeServiceConfig2W(
            info.svc,
            SERVICE_CONFIG_SERVICE_SID_INFO,
            &si
        )) {
            return reportChange2Error(info, "STD_INFO");
        }
    }

    if (conf.privs) {
        SERVICE_REQUIRED_PRIVILEGES_INFOW pi = {};
        auto privs = toMultistring(*conf.privs);
        auto wprivs = toWstring(privs.view());
        pi.pmszRequiredPrivileges = wprivs.data();
        if (!ChangeServiceConfig2W(
            info.svc,
            SERVICE_CONFIG_REQUIRED_PRIVILEGES_INFO,
            &pi
        )) {
            return reportChange2Error(info, "REQUIRED_PRIVILEGES");
        }
    }

    return {};
}


/****************************************************************************
*
*   FuncInfo
*
***/

//===========================================================================
FuncInfo::FuncInfo(string_view svcName)
    : svcName{string(svcName)}
{}

//===========================================================================
FuncInfo::~FuncInfo() {
    if (this->svc)
        CloseServiceHandle(this->svc);
    if (this->scm)
        CloseServiceHandle(this->scm);
}


/****************************************************************************
*
*   Public API
*
***/

//===========================================================================
error_code Dim::winSvcCreate(const WinServiceConfig & sconf) {
    auto conf{sconf};

    if (!conf.progWithArgs)
        conf.progWithArgs = envExecPath();
    if (!conf.displayName)
        conf.displayName = conf.serviceName;
    assert(!conf.serviceName.empty());

    auto info = FuncInfo(conf.serviceName);
    if (auto ec = openScm(&info, SC_MANAGER_CREATE_SERVICE, "CREATE"))
        return ec;

    string acct;
    if (!conf.account || conf.account->empty()) {
        acct = "NT Service\\"s + conf.serviceName;
        conf.account = acct.c_str();
    } else if (*conf.account == WinServiceConfig::kLocalSystem) {
        conf.account->clear();
    }

    auto svcType = getOsValue(s_svcTypes, conf.serviceType);
    if (conf.interactive)
        svcType |= SERVICE_INTERACTIVE_PROCESS;
    auto startType = getOsValue(s_svcStarts, conf.startType);
    auto errCtrl = getOsValue(s_svcErrCtrls, conf.errorControl);

    DWORD tagId = conf.loadOrderTag.value_or(0);
    if (conf.loadOrderTag) {
        if (!conf.loadOrderGroup) {
            logMsgError() << "winInstallService: "
                "load order tag without load order group";
            return make_error_code(errc::invalid_argument);
        }
        if (conf.startType != WinServiceConfig::Start::kBoot
            && conf.startType != WinServiceConfig::Start::kSystem
        ) {
            logMsgError() << "winInstallService: load order tag without "
                "start type of boot or system";
            return make_error_code(errc::invalid_argument);
        }
    }

    optional<wstring> deps;
    if (conf.deps)
        deps = toWstring(toMultistring(*conf.deps).view());

    info.svc = CreateServiceW(
        info.scm,
        toWstring(conf.serviceName).c_str(),
        toWstring(*conf.displayName).c_str(),
        SERVICE_ALL_ACCESS,
        svcType,
        startType,
        errCtrl,
        toWstring(*conf.progWithArgs).c_str(),
        conf.loadOrderGroup
            ? toWstring(*conf.loadOrderGroup).c_str()
            : nullptr,
        tagId ? &tagId : nullptr,
        deps ? deps->c_str() : nullptr,
        conf.account
            ? toWstring(*conf.account).c_str()
            : nullptr,
        conf.password ? toWstring(*conf.password).c_str() : nullptr
    );
    if (!info.svc)
        return reportError(info, "CreateServiceW");
    Finally sDel([svc = info.svc]() { DeleteService(svc); });

    if (auto ec = changeAllConf2(info, conf))
        return ec;
    if (auto ec = winSvcGrantLogonAsService(*conf.account))
        return ec;

    sDel = {};
    logMsgInfo() << conf.serviceName << " service created.";
    return {};
}

//===========================================================================
error_code Dim::winSvcDelete(std::string_view svcName) {
    assert(!svcName.empty());

    auto info = FuncInfo{svcName};
    if (auto ec = openScm(&info, SC_MANAGER_CONNECT, "DELETE_SERVICE"))
        return ec;
    if (auto ec = openSvc(&info, DELETE, "DELETE"))
        return ec;

    if (!DeleteService(info.svc))
        return reportError(info, "DeleteService");

    logMsgInfo() << svcName << " service deleted.";
    return {};
}

//===========================================================================
static error_code queryConf(
    WinServiceConfig * out,
    const FuncInfo & info
) {
    *out = {};
    auto [ec, cfg] = queryConf(info);
    if (!cfg)
        return ec;

    out->serviceType = getValue(
        s_svcTypes,
        cfg->dwServiceType,
        WinServiceConfig::Type::kInvalid
    );
    if (out->serviceType == WinServiceConfig::Type::kInvalid) {
        return reportBadRange(
            info,
            "QueryServiceConfigW",
            {},
            "unknown service type"
        );
    }
    if (cfg->dwServiceType & SERVICE_INTERACTIVE_PROCESS)
        out->interactive = true;

    out->startType = getValue(
        s_svcStarts,
        cfg->dwStartType,
        WinServiceConfig::Start::kInvalid
    );
    if (out->startType == WinServiceConfig::Start::kInvalid) {
        return reportBadRange(
            info,
            "QueryServiceConfigW",
            {},
            "unknown service start type"
        );
    }

    out->errorControl = getValue(
        s_svcErrCtrls,
        cfg->dwErrorControl,
        WinServiceConfig::ErrCtrl::kInvalid
    );
    if (out->errorControl == WinServiceConfig::ErrCtrl::kInvalid) {
        return reportBadRange(
            info,
            "QueryServiceConfigW",
            {},
            "unknown error control mode"
        );
    }

    out->progWithArgs = toString(cfg->lpBinaryPathName);

    if (cfg->lpLoadOrderGroup) {
        out->loadOrderGroup = toString(cfg->lpLoadOrderGroup);
    } else {
        out->loadOrderGroup = string{};
    }

    out->loadOrderTag = cfg->dwTagId;

    out->deps = splitMultistring(cfg->lpDependencies);

    out->account = toString(cfg->lpServiceStartName);

    out->displayName = toString(cfg->lpDisplayName);

    // Check for kAutoDelayed
    if (out->startType == WinServiceConfig::Start::kAuto) {
        if (auto&& [ec, cfg] = queryConf2<SERVICE_DELAYED_AUTO_START_INFO>(
            info,
            SERVICE_CONFIG_DELAYED_AUTO_START_INFO,
            "DELAYED_START"
        ); !ec) {
            return ec;
        } else {
            if (cfg->fDelayedAutostart)
                out->startType = WinServiceConfig::Start::kAutoDelayed;
        }
    }

    // Description
    if (auto&& [ec, cfg] = queryConf2<SERVICE_DESCRIPTION>(
        info,
        SERVICE_CONFIG_DESCRIPTION,
        "DESCRIPTION"
    ); !ec) {
        return ec;
    } else {
        out->desc = toString(cfg->lpDescription);
    }

    // Failure actions
    if (auto&& [ec, cfg] = queryConf2<SERVICE_FAILURE_ACTIONSW>(
        info,
        SERVICE_CONFIG_FAILURE_ACTIONS,
        "FAILURE_ACTIONS"
    ); !ec) {
        return ec;
    } else {
        if (cfg->dwResetPeriod != INFINITE)
            out->failureReset = (seconds) cfg->dwResetPeriod;
        out->rebootMsg = toString(cfg->lpRebootMsg);
        out->failureProgWithArgs = toString(cfg->lpCommand);
        out->failureActions.emplace();
        for (auto i = 0u; i < cfg->cActions; ++i) {
            auto & sc = cfg->lpsaActions[i];
            auto & fa = out->failureActions->emplace_back();
            switch (sc.Type) {
            case SC_ACTION_NONE: fa.type = fa.kNone; break;
            case SC_ACTION_RESTART: fa.type = fa.kRestart; break;
            case SC_ACTION_REBOOT: fa.type = fa.kReboot; break;
            case SC_ACTION_RUN_COMMAND: fa.type = fa.kRunCommand; break;
            default:
                return reportBadRange(
                    info,
                    "QueryServiceConfig2W",
                    "FAILURE_ACTIONS",
                    format("unknown action type ({})", to_underlying(sc.Type))
                );
            }
            fa.delay = static_cast<milliseconds>(sc.Delay);
        }
    }

    // Failure flag
    if (auto&& [ec, cfg] = queryConf2<SERVICE_FAILURE_ACTIONS_FLAG>(
        info,
        SERVICE_CONFIG_FAILURE_ACTIONS_FLAG,
        "FAILURE_FLAG"
    ); !ec) {
        return ec;
    } else {
        out->failureFlag = cfg->fFailureActionsOnNonCrashFailures
            ? WinServiceConfig::FailureFlag::kCrashOrNonZeroExitCode
            : WinServiceConfig::FailureFlag::kCrashOnly;
    }

    // Preferred node
    if (auto&& [ec, cfg] = queryConf2<SERVICE_PREFERRED_NODE_INFO>(
        info,
        SERVICE_CONFIG_PREFERRED_NODE,
        "PREFERRED_NODE"
    ); !ec) {
        return ec;
    } else {
        if (cfg->fDelete) {
            out->preferredNode =
                (unsigned) WinServiceConfig::PreferredNode::kNone;
        } else {
            out->preferredNode = cfg->usPreferredNode;
        }
    }

    // Preshutdown info
    if (auto&& [ec, cfg] = queryConf2<SERVICE_PRESHUTDOWN_INFO>(
        info,
        SERVICE_CONFIG_PRESHUTDOWN_INFO,
        "PRESHUTDOWN_INFO"
    ); !ec) {
        return ec;
    } else {
        out->preshutdownTimeout = (milliseconds) cfg->dwPreshutdownTimeout;
    }

    // Required privileges info
    if (auto&& [ec, cfg] = queryConf2<SERVICE_REQUIRED_PRIVILEGES_INFO>(
        info,
        SERVICE_CONFIG_REQUIRED_PRIVILEGES_INFO,
        "REQUIRED_PRIVILEGES"
    ); !ec) {
        return ec;
    } else {
        out->privs = splitMultistring(cfg->pmszRequiredPrivileges);
    }

    // Service SID info
    if (auto&& [ec, cfg] = queryConf2<SERVICE_SID_INFO>(
        info,
        SERVICE_CONFIG_SERVICE_SID_INFO,
        "SID_INFO"
    ); !ec) {
        return ec;
    } else {
        out->sidType = getValue(
            s_svcSidTypes,
            cfg->dwServiceSidType,
            WinServiceConfig::SidType::kInvalid
        );
    }

    // Trigger info
    if (auto&& [ec, cfg] = queryConf2<SERVICE_TRIGGER_INFO>(
        info,
        SERVICE_CONFIG_TRIGGER_INFO,
        "TRIGGERS"
    ); !ec) {
        return ec;
    } else {
        out->triggers.emplace();
        for (auto i = 0u; i < cfg->cTriggers; ++i) {
            auto & trig = out->triggers->emplace_back();
            trig.type = getValue(
                s_svcTriggerTypes,
                cfg->pTriggers[i].dwTriggerType,
                WinServiceConfig::Trigger::Type::kInvalid
            );
        }
    }

    // Launch protected
    if (auto&& [ec, cfg] = queryConf2<SERVICE_LAUNCH_PROTECTED_INFO>(
        info,
        SERVICE_CONFIG_LAUNCH_PROTECTED,
        "LAUNCH_PROTECTED"
    ); !ec) {
        return ec;
    } else {
        out->launchProt = getValue(
            s_svcLaunchProts,
            cfg->dwLaunchProtected,
            WinServiceConfig::LaunchProt::kInvalid
        );
    }

    return {};
}

//===========================================================================
static error_code queryStat(
    WinServiceStatus * out,
    const FuncInfo & info
) {
    *out = {};
    return {};
}

//===========================================================================
error_code Dim::winSvcQuery(
    string_view svcName,
    WinServiceConfig * conf,
    WinServiceStatus * stat
) {
    assert(!svcName.empty());

    auto info = FuncInfo(svcName);

    if (auto ec = openScm(&info, SC_MANAGER_CONNECT, "QUERY"))
        return ec;
    DWORD access = bool(conf) * SERVICE_QUERY_CONFIG
        | bool(stat) * SERVICE_QUERY_STATUS;
    if (auto ec = openSvc(&info, access, "QUERY"))
        return ec;

    if (!conf && !stat)
        return {};
    if (conf) {
        if (auto ec = queryConf(conf, info))
            return ec;
    }
    if (stat) {
        if (auto ec = queryStat(stat, info))
            return ec;
    }
    return {};
}

//===========================================================================
error_code Dim::winSvcFind(
    vector<WinServiceStatus> * out,
    const WinServiceFilter & filter
) {
    out->clear();
    unordered_set<string> names;
    for (auto&& name : filter.names)
        names.insert(toLower(name));
    auto info = FuncInfo({});
    if (auto ec = openScm(&info, SC_MANAGER_ENUMERATE_SERVICE, "ENUMERATE"))
        return ec;

    DWORD needed = 0;
    DWORD found = 0;
    DWORD resumeHandle = 0;
    for (;;) {
        auto&& [sts, count] =
            reserve<ENUM_SERVICE_STATUS_PROCESS>(info, needed);
        auto success = EnumServicesStatusExW(
            info.scm,
            SC_ENUM_PROCESS_INFO,
            SERVICE_TYPE_ALL,
            SERVICE_STATE_ALL,
            (BYTE *) sts,
            (DWORD) count,
            &needed,
            &found,
            &resumeHandle,
            nullptr
        );
        if (!success) {
            WinError err;
            if (err != ERROR_MORE_DATA) {
                out->clear();
                return reportError(info, "EnumServicesStatusExW");
            }
        }
        for (auto i = 0u; i < found; ++i) {
            auto & sys = sts[i];
            auto & ssp = sys.ServiceStatusProcess;
            WinServiceStatus st;
            st.serviceName = toString(sys.lpServiceName);
            st.displayName = toString(sys.lpDisplayName);
            st.serviceType = getValue(
                s_svcTypes,
                ssp.dwServiceType,
                WinServiceConfig::Type::kInvalid
            );
            st.state = getValue(
                s_svcStates,
                ssp.dwCurrentState,
                WinServiceStatus::State::kInvalid
            );
            if (auto ca = ssp.dwControlsAccepted) {
                using enum WinServiceStatus::Control;
                struct {
                    WinServiceStatus::Control flag;
                    DWORD osflag;
                } flags[] = {
                    { fNetBindChange, SERVICE_ACCEPT_NETBINDCHANGE },
                    { fParamChange,   SERVICE_ACCEPT_PARAMCHANGE },
                    { fPauseContinue, SERVICE_ACCEPT_PAUSE_CONTINUE },
                    { fPreshutdown,   SERVICE_ACCEPT_PRESHUTDOWN },
                    { fShutdown,      SERVICE_ACCEPT_SHUTDOWN },
                    { fStop,          SERVICE_ACCEPT_STOP },
                    { fHardwareProfileChange,
                                      SERVICE_ACCEPT_HARDWAREPROFILECHANGE },
                    { fPowerEvent,    SERVICE_ACCEPT_POWEREVENT },
                    { fSessionChange, SERVICE_ACCEPT_SESSIONCHANGE },
                };
                for (auto&& ff : flags) {
                    if (ca & ff.osflag)
                        st.accepted |= (unsigned) ff.flag;
                }
            }
            if (auto wec = ssp.dwWin32ExitCode;
                wec == ERROR_SERVICE_SPECIFIC_ERROR
            ) {
                st.exitCode = ssp.dwServiceSpecificExitCode;
            } else {
                st.exitCode = wec;
            }
            st.checkPoint = ssp.dwCheckPoint;
            st.waitHint = ssp.dwWaitHint;
            st.processId = ssp.dwProcessId;
            if (ssp.dwServiceFlags & SERVICE_RUNS_IN_SYSTEM_PROCESS) {
                st.flags = WinServiceStatus::Flags::kInSystemProcess;
            } else {
                st.flags = WinServiceStatus::Flags::kNormal;
            }

            if ((names.empty() || names.contains(st.serviceName))
                && (filter.types.empty()
                    || filter.types.contains(st.serviceType))
                && (filter.states.empty() || filter.states.contains(st.state))
                && (filter.processIds.empty()
                    || filter.processIds.contains(st.processId))
            ) {
                out->push_back(st);
            }
        }
        if (success)
            return {};
    }
}

//===========================================================================
error_code Dim::winSvcGrantLogonAsService (string_view account) {
    if (account == WinServiceConfig::kLocalSystem) {
        // LocalSystem implicitly has log on as a service rights and can't
        // be looked up by name anyway.
        return {};
    }

    LSA_OBJECT_ATTRIBUTES oa = { sizeof oa };
    LSA_HANDLE h;
    ACCESS_MASK access = POLICY_LOOKUP_NAMES | POLICY_CREATE_ACCOUNT;
    auto status = LsaOpenPolicy(NULL, &oa, access, &h);
    if (status) {
        WinError err(LsaNtStatusToWinError(status));
        logMsgError() << "LsaOpenPolicy: " << err;
        return err.code();
    }
    Finally h_f([=]() { LsaClose(h); });

    auto wacct = toWstring(account);
    SID_NAME_USE use;
    SE_SID acct;
    DWORD acctLen = sizeof acct;
    wchar_t domain[256];
    auto domLen = (DWORD) size(domain);
    if (!LookupAccountNameW(
        nullptr,
        wacct.c_str(),
        &acct.Sid,
        &acctLen,
        domain,
        &domLen,
        &use
    )) {
        WinError err;
        logMsgError() << "LookupAccountNameW(" << account << "): " << err;
        return err.code();
    }

    wchar_t wlogon[] = SE_SERVICE_LOGON_NAME;
    auto logonLen = (USHORT) sizeof wlogon;
    LSA_UNICODE_STRING rights[] = {
        { logonLen - sizeof *wlogon, logonLen, wlogon },
    };
    status = LsaAddAccountRights(h, &acct.Sid, rights, (ULONG) size(rights));
    if (status) {
        WinError err(LsaNtStatusToWinError(status));
        logMsgError() << "LsaAddAccountRights(" << account << ", LOGON): "
            << err;
        return err.code();
    }

    return {};
}
