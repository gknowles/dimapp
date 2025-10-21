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


/****************************************************************************
*
*   Translate strings
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


/****************************************************************************
*
*   Translate values
*
***/

namespace {
const EnumMap s_svcTypes[] = {
    { (int) WinSvcConf::Type::kOwn, SERVICE_WIN32_OWN_PROCESS },

    // Must come after kOwn, as a kShared that happens to be running under
    // a dedicated svchost gets set to kOwn|kShared
    { (int) WinSvcConf::Type::kShared, SERVICE_WIN32_SHARE_PROCESS },

    { (int) WinSvcConf::Type::kKernelDriver, SERVICE_KERNEL_DRIVER },
    {
        (int) WinSvcConf::Type::kFileSysDriver,
        SERVICE_FILE_SYSTEM_DRIVER
    },
    {
        (int) WinSvcConf::Type::kRecognizerDriver,
        SERVICE_RECOGNIZER_DRIVER
    },
    { (int) WinSvcConf::Type::kAdapter, SERVICE_ADAPTER },

    // Must come after kOwn and kShared, as kUserOwn is kOwn or'd with
    // SERVICE_USER_SERVICE, similarly for kUserShared.
    { (int) WinSvcConf::Type::kUserOwn, SERVICE_USER_OWN_PROCESS },
    { (int) WinSvcConf::Type::kUserShared, SERVICE_USER_SHARE_PROCESS },

    // Must come after kOwn and kShared, similar to kUserOwn above but with
    // SERVICE_PKG_SERVICE. Unknown if PKG and USER are mutually exclusive and
    // what it means if they aren't.
    {
        (int) WinSvcConf::Type::kPackageOwn,
        SERVICE_WIN32_OWN_PROCESS | SERVICE_PKG_SERVICE
    },
    {
        (int) WinSvcConf::Type::kPackageShared,
        SERVICE_WIN32_SHARE_PROCESS | SERVICE_PKG_SERVICE
    },
};
const EnumMap s_svcStarts[] = {
    { (int) WinSvcConf::Start::kAuto, SERVICE_AUTO_START },
    { (int) WinSvcConf::Start::kAutoDelayed, SERVICE_AUTO_START },
    { (int) WinSvcConf::Start::kBoot, SERVICE_BOOT_START },
    { (int) WinSvcConf::Start::kDemand, SERVICE_DEMAND_START },
    { (int) WinSvcConf::Start::kDisabled, SERVICE_DISABLED },
    { (int) WinSvcConf::Start::kSystem, SERVICE_SYSTEM_START },
};
const EnumMap s_svcErrCtrls[] = {
    { (int) WinSvcConf::ErrCtrl::kIgnore, SERVICE_ERROR_IGNORE },
    { (int) WinSvcConf::ErrCtrl::kNormal, SERVICE_ERROR_NORMAL },
    { (int) WinSvcConf::ErrCtrl::kSevere, SERVICE_ERROR_SEVERE },
    { (int) WinSvcConf::ErrCtrl::kCritical, SERVICE_ERROR_CRITICAL },
};
const EnumMap s_svcSidTypes[] = {
    { (int) WinSvcConf::SidType::kNone, SERVICE_SID_TYPE_NONE },
    {
        (int) WinSvcConf::SidType::kUnrestricted,
        SERVICE_SID_TYPE_UNRESTRICTED
    },
    {
        (int) WinSvcConf::SidType::kRestricted,
        SERVICE_SID_TYPE_RESTRICTED
    },
};
const EnumMap s_svcLaunchProts[] = {
    {
        (int) WinSvcConf::LaunchProt::kNone,
        SERVICE_LAUNCH_PROTECTED_NONE
    },
    {
        (int) WinSvcConf::LaunchProt::kWindows,
        SERVICE_LAUNCH_PROTECTED_WINDOWS
    },
    {
        (int) WinSvcConf::LaunchProt::kWindowsLight,
        SERVICE_LAUNCH_PROTECTED_WINDOWS_LIGHT
    },
    {
        (int) WinSvcConf::LaunchProt::kAntimalwareLight,
        SERVICE_LAUNCH_PROTECTED_ANTIMALWARE_LIGHT
    },
};
} // namespace

//===========================================================================
template<typename E, typename T>
static E getValue(const T & tbl, DWORD osvalue, E def = {}) {
    for (auto && v : tbl) {
        if (v.osvalue == osvalue)
            return static_cast<E>(v.value);
    }
    return def;
}

//===========================================================================
template<int N>
static WinSvcConf::Type getValue(
    const EnumMap tbl[N],
    DWORD osvalue,
    WinSvcConf::Type def
) {
    auto value = def;
    for (auto&& em : tbl) {
        if ((osvalue & em.osvalue) == em.osvalue) {
            value = static_cast<WinSvcConf::Type>(em.value);
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
            out.push_back(toString(wstring_view{ptr, size_t(eptr - ptr)}));
            ptr = eptr + 1;
        }
    }
    return out;
}


/****************************************************************************
*
*   Translate trigger types
*
***/

namespace {
struct EnumMapGuid {
    int value;
    DWORD osvalue;
    const GUID * osvalue2;
};

const EnumMapGuid s_svcTriggerTypes[] = {
    {
        (int) WinSvcConf::Trigger::Type::kCustom,
        SERVICE_TRIGGER_TYPE_CUSTOM,
    },
    {
        (int) WinSvcConf::Trigger::Type::kDeviceArrival,
        SERVICE_TRIGGER_TYPE_DEVICE_INTERFACE_ARRIVAL,
    },
    {
        (int) WinSvcConf::Trigger::Type::kDomainJoin,
        SERVICE_TRIGGER_TYPE_DOMAIN_JOIN,
        &DOMAIN_JOIN_GUID,
    },
    {
        (int) WinSvcConf::Trigger::Type::kDomainLeave,
        SERVICE_TRIGGER_TYPE_DOMAIN_JOIN,
        &DOMAIN_LEAVE_GUID,
    },
    {
        (int) WinSvcConf::Trigger::Type::kFirewallPortOpen,
        SERVICE_TRIGGER_TYPE_FIREWALL_PORT_EVENT,
        &FIREWALL_PORT_OPEN_GUID,
    },
    {
        (int) WinSvcConf::Trigger::Type::kFirewallPortClose,
        SERVICE_TRIGGER_TYPE_FIREWALL_PORT_EVENT,
        &FIREWALL_PORT_CLOSE_GUID
    },
    {
        (int) WinSvcConf::Trigger::Type::kGroupPolicyMachine,
        SERVICE_TRIGGER_TYPE_GROUP_POLICY,
        &MACHINE_POLICY_PRESENT_GUID,
    },
    {
        (int) WinSvcConf::Trigger::Type::kGroupPolicyUser,
        SERVICE_TRIGGER_TYPE_GROUP_POLICY,
        &USER_POLICY_PRESENT_GUID,
    },
    {
        (int) WinSvcConf::Trigger::Type::kIpAddressArrival,
        SERVICE_TRIGGER_TYPE_IP_ADDRESS_AVAILABILITY,
        &NETWORK_MANAGER_FIRST_IP_ADDRESS_ARRIVAL_GUID,
    },
    {
        (int) WinSvcConf::Trigger::Type::kIpAddressRemoval,
        SERVICE_TRIGGER_TYPE_IP_ADDRESS_AVAILABILITY,
        &NETWORK_MANAGER_LAST_IP_ADDRESS_REMOVAL_GUID,
    },
    {
        (int) WinSvcConf::Trigger::Type::kNetworkNamedPipe,
        SERVICE_TRIGGER_TYPE_NETWORK_ENDPOINT,
        &NAMED_PIPE_EVENT_GUID,
    },
    {
        (int) WinSvcConf::Trigger::Type::kNetworkRpc,
        SERVICE_TRIGGER_TYPE_NETWORK_ENDPOINT,
        &RPC_INTERFACE_EVENT_GUID,
    },
};
const EnumMap s_svcTriggerActions[] = {
    {
        (int) WinSvcConf::Trigger::Action::kServiceStart,
        SERVICE_TRIGGER_ACTION_SERVICE_START,
    },
    {
        (int) WinSvcConf::Trigger::Action::kServiceStop,
        SERVICE_TRIGGER_ACTION_SERVICE_STOP,
    },
};
const EnumMap s_svcTriggerDataTypes[] = {
    {
        (int) WinSvcConf::Trigger::DataType::kBinary,
        SERVICE_TRIGGER_DATA_TYPE_BINARY,
    },
    {
        (int) WinSvcConf::Trigger::DataType::kString,
        SERVICE_TRIGGER_DATA_TYPE_STRING,
    },
    {
        (int) WinSvcConf::Trigger::DataType::kLevel,
        SERVICE_TRIGGER_DATA_TYPE_LEVEL,
    },
    {
        (int) WinSvcConf::Trigger::DataType::kKeywordAny,
        SERVICE_TRIGGER_DATA_TYPE_KEYWORD_ANY,
    },
    {
        (int) WinSvcConf::Trigger::DataType::kKeywordAll,
        SERVICE_TRIGGER_DATA_TYPE_KEYWORD_ALL,
    },
};
} // namespace

//===========================================================================
WinSvcConf::Trigger::Type getValue(DWORD type, GUID * subtype) {
    for (auto&& v : s_svcTriggerTypes) {
        if (type == v.osvalue && (!v.osvalue2 || *v.osvalue2 == *subtype))
            return static_cast<WinSvcConf::Trigger::Type>(v.value);
    }
    return WinSvcConf::Trigger::Type::kInvalid;
}

//===========================================================================
pair<DWORD, GUID> getOsValues(
    WinSvcConf::Trigger::Type type,
    const string & subtype
) {
    for (auto i = 0u; i < size(s_svcTriggerTypes); ++i) {
        if ((int) type != s_svcTriggerTypes[i].value)
            continue;

        auto & v = s_svcTriggerTypes[i];
        if (v.osvalue2)
            return {v.osvalue, *v.osvalue2};
        if (subtype.empty())
            return {v.osvalue, {}};
        Guid stype;
        if (!parse(&stype, subtype)) {
            // reportError
            return {};
        }
        return {v.osvalue, bit_cast<GUID>(stype)};
    }
    return {};
}


/****************************************************************************
*
*   Helpers
*
***/

//===========================================================================
static error_code reportBadRange(
    const FuncInfo & info,
    string_view func,
    string_view arg2,
    string_view detail
) {
    if (info.logErrors) {
        auto os = logMsgError();
        os << func << "(" << info.svcName;
        if (!arg2.empty())
            os << ", " << arg2;
        os << "): " << detail;
    }
    return make_error_code(errc::result_out_of_range);
}

//===========================================================================
static error_code reportQuery2BadRange(
    const FuncInfo & info,
    string_view arg2,
    string_view detail
) {
    return reportBadRange(info, "QueryServiceConfig2W", arg2, detail);
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
        // According to Microsoft docs the maximum size of the buffer for
        // QueryServiceConfigW or QueryServiceConfig2W is 8k bytes.
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
static error_code changeAllConf2(
    const FuncInfo & info,
    const WinSvcConf & conf
) {
    if (conf.desc) {
        auto wdesc = toWstring(*conf.desc);
        SERVICE_DESCRIPTIONW sd = {};
        sd.lpDescription = wdesc.data();
        if (!ChangeServiceConfig2W(info.svc, SERVICE_CONFIG_DESCRIPTION, &sd))
            return reportChange2Error(info, "DESCRIPTION");
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

    if (conf.startType == WinSvcConf::Start::kAutoDelayed) {
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

    if (conf.failureFlag ==
        WinSvcConf::FailureFlag::kCrashOrNonZeroExitCode
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

    if (conf.sidType != WinSvcConf::SidType::kInvalid
        && conf.sidType != WinSvcConf::SidType::kDefault
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

    if (conf.triggers) {
        vector<SERVICE_TRIGGER> trigs(conf.triggers->size());
        list<GUID> subtypes;
        list<vector<SERVICE_TRIGGER_SPECIFIC_DATA_ITEM>> itemLists;
        for (auto&& t : *conf.triggers) {
            auto & ot = trigs.emplace_back();
            auto [type, stype] = getOsValues(t.type, t.subTypeGuid);
            ot.dwTriggerType = type;
            if (stype != GUID{}) {
                subtypes.push_back(stype);
                ot.pTriggerSubtype = &subtypes.back();
            }
            ot.dwAction = getOsValue(s_svcTriggerActions, t.action);
            if (!t.dataItems.empty()) {
                ot.cDataItems = (DWORD) t.dataItems.size();
                auto & items = itemLists.emplace_back();
                for (auto&& itm : t.dataItems) {
                    auto & oitm = items.emplace_back();
                    oitm.dwDataType = getOsValue(
                        s_svcTriggerDataTypes,
                        itm.type
                    );
                    oitm.cbData = (DWORD) itm.data.size();
                    oitm.pData = (PBYTE) itm.data.data();
                }
            }
        }
        SERVICE_TRIGGER_INFO ti = {};
        ti.cTriggers = (DWORD) trigs.size();
        ti.pTriggers = trigs.data();
        if (!ChangeServiceConfig2W(
            info.svc,
            SERVICE_CONFIG_TRIGGER_INFO,
            &ti
        )) {
            return reportChange2Error(info, "TRIGGER");
        }
    }

    if (conf.preferredNode !=
        (unsigned) WinSvcConf::PreferredNode::kInvalid
    ) {
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

    if (conf.launchProt != WinSvcConf::LaunchProt::kInvalid) {
        SERVICE_LAUNCH_PROTECTED_INFO lpi = {};
        lpi.dwLaunchProtected = getOsValue(s_svcLaunchProts, conf.launchProt);
        if (!ChangeServiceConfig2W(
            info.svc,
            SERVICE_CONFIG_LAUNCH_PROTECTED,
            &lpi
        )) {
            return reportChange2Error(info, "LAUNCH_PROTECTED");
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
*   Create and Delete
*
***/

//===========================================================================
error_code Dim::winSvcCreate(const WinSvcConf & sconf) {
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
    } else if (*conf.account == WinSvcConf::kLocalSystem) {
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
        if (conf.startType != WinSvcConf::Start::kBoot
            && conf.startType != WinSvcConf::Start::kSystem
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


/****************************************************************************
*
*   Access rights
*
***/

//===========================================================================
error_code Dim::winSvcGrantLogonAsService (string_view account) {
    if (account == WinSvcConf::kLocalSystem) {
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
        logMsgError() << "LsaAddAccountRights(" << account
            << ", SERVICE_LOGON): " << err;
        return err.code();
    }

    return {};
}


/****************************************************************************
*
*   Query
*
***/

//===========================================================================
static pair<error_code, QUERY_SERVICE_CONFIGW *> queryConf(
    const FuncInfo & info
) {
    DWORD needed = 0;
    for (;;) {
        auto&& [cfg, count] = reserve<QUERY_SERVICE_CONFIGW>(info, needed);
        if (count > 8192) {
            // According to the QueryServiceConfigW docs, the maximum allowed
            // size of this buffer is 8k.
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
static error_code queryConf(
    WinSvcConf * out,
    const FuncInfo & info
) {
    *out = {};
    auto [ec, cfg] = queryConf(info);
    if (!cfg)
        return ec;

    out->serviceType = getValue(
        s_svcTypes,
        cfg->dwServiceType,
        WinSvcConf::Type::kInvalid
    );
    if (out->serviceType == WinSvcConf::Type::kInvalid) {
        return reportBadRange(
            info,
            "QueryServiceConfigW",
            {},
            format("unknown service type ({})", cfg->dwServiceType)
        );
    }
    if (cfg->dwServiceType & SERVICE_INTERACTIVE_PROCESS)
        out->interactive = true;
    if (cfg->dwServiceType & SERVICE_USERSERVICE_INSTANCE)
        out->instance = true;
    if (cfg->dwServiceType & SERVICE_WIN32)
        out->sharedServiceSplitEnabled = true;

    out->startType = getValue(
        s_svcStarts,
        cfg->dwStartType,
        WinSvcConf::Start::kInvalid
    );
    if (out->startType == WinSvcConf::Start::kInvalid) {
        return reportBadRange(
            info,
            "QueryServiceConfigW",
            {},
            format("unknown service start type ({})", cfg->dwStartType)
        );
    }

    out->errorControl = getValue(
        s_svcErrCtrls,
        cfg->dwErrorControl,
        WinSvcConf::ErrCtrl::kInvalid
    );
    if (out->errorControl == WinSvcConf::ErrCtrl::kInvalid) {
        return reportBadRange(
            info,
            "QueryServiceConfigW",
            {},
            format("unknown error control mode ({})", cfg->dwErrorControl)
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
                return reportQuery2BadRange(
                    info,
                    "FAILURE_ACTIONS",
                    format("unknown action type ({})", to_underlying(sc.Type))
                );
            }
            fa.delay = static_cast<milliseconds>(sc.Delay);
        }
    }

    // Check for kAutoDelayed
    if (out->startType == WinSvcConf::Start::kAuto) {
        if (auto&& [ec, cfg] = queryConf2<SERVICE_DELAYED_AUTO_START_INFO>(
            info,
            SERVICE_CONFIG_DELAYED_AUTO_START_INFO,
            "DELAYED_START"
        ); !ec) {
            return ec;
        } else {
            if (cfg->fDelayedAutostart)
                out->startType = WinSvcConf::Start::kAutoDelayed;
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
            ? WinSvcConf::FailureFlag::kCrashOrNonZeroExitCode
            : WinSvcConf::FailureFlag::kCrashOnly;
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
            WinSvcConf::SidType::kInvalid
        );
        if (out->sidType == WinSvcConf::SidType::kInvalid) {
            return reportQuery2BadRange(
                info,
                "SID_INFO",
                format("unknown sid type ({})", cfg->dwServiceSidType)
            );
        }
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
            auto & ot = out->triggers->emplace_back();
            auto & t = cfg->pTriggers[i];
            ot.type = getValue(t.dwTriggerType, t.pTriggerSubtype);
            if (ot.type == WinSvcConf::Trigger::Type::kInvalid) {
                return reportQuery2BadRange(
                    info,
                    "TRIGGER",
                    format("unknown trigger type ({})", t.dwTriggerType)
                );
            }
            if (ot.type == WinSvcConf::Trigger::Type::kCustom
                || ot.type == WinSvcConf::Trigger::Type::kDeviceArrival
            ) {
                if (t.pTriggerSubtype) {
                    auto stype = *(Guid *) t.pTriggerSubtype;
                    ot.subTypeGuid = toString(stype);
                }
            }
            ot.action = getValue(
                s_svcTriggerActions,
                t.dwAction,
                WinSvcConf::Trigger::Action::kInvalid
            );
            if (ot.action == WinSvcConf::Trigger::Action::kInvalid) {
                return reportQuery2BadRange(
                    info,
                    "TRIGGER",
                    format("unknown trigger action ({})", t.dwAction)
                );
            }
            for (auto j = 0u; j < cfg->cTriggers; ++j) {
                auto & otd = ot.dataItems.emplace_back();
                auto & td = t.pDataItems[j];
                otd.type = getValue(
                    s_svcTriggerDataTypes,
                    td.dwDataType,
                    WinSvcConf::Trigger::DataType::kInvalid
                );
                otd.data.assign((const char *) td.pData, td.cbData);
            }
        }
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
                (unsigned) WinSvcConf::PreferredNode::kNone;
        } else {
            out->preferredNode = cfg->usPreferredNode;
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
            WinSvcConf::LaunchProt::kInvalid
        );
        if (out->launchProt == WinSvcConf::LaunchProt::kInvalid) {
            return reportQuery2BadRange(
                info,
                "LAUNCH_PROTECTED",
                format(
                    "unknown launch protected ({})",
                    toString(out->launchProt)
                )
            );
        }
    }

    return {};
}

//===========================================================================
static error_code parseStat(
    WinSvcStat * out,
    const FuncInfo & info,
    const string_view func,
    const SERVICE_STATUS_PROCESS & ssp
) {
    *out = {};
    out->serviceType = getValue(
        s_svcTypes,
        ssp.dwServiceType,
        WinSvcConf::Type::kInvalid
    );
    if (out->serviceType == WinSvcConf::Type::kInvalid) {
        return reportBadRange(
            info,
            func,
            {},
            format("unknown service type ({})", ssp.dwServiceType)
        );
    }
    {
        using enum WinSvcStat::State;
        struct {
            WinSvcStat::State value;
            DWORD osvalue;
        } states[] = {
            { kContinuePending, SERVICE_CONTINUE_PENDING },
            { kPausePending,    SERVICE_PAUSE_PENDING },
            { kPaused,          SERVICE_PAUSED },
            { kRunning,         SERVICE_RUNNING },
            { kStartPending,    SERVICE_START_PENDING },
            { kStopPending,     SERVICE_STOP_PENDING },
            { kStopped,         SERVICE_STOPPED },
        };
        out->state = getValue(
            states,
            ssp.dwCurrentState,
            WinSvcStat::State::kInvalid
        );
        if (out->state == WinSvcStat::State::kInvalid) {
            return reportBadRange(
                info,
                func,
                {},
                format("unknown service state ({})", ssp.dwCurrentState)
            );
        }
    }
    if (auto ca = ssp.dwControlsAccepted) {
        using enum WinSvcStat::Accept;
        struct {
            WinSvcStat::Accept flag;
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
                out->accepted |= (unsigned) ff.flag;
        }
    }
    if (auto wec = ssp.dwWin32ExitCode;
        wec == ERROR_SERVICE_SPECIFIC_ERROR
    ) {
        out->exitCode = ssp.dwServiceSpecificExitCode;
    } else {
        out->exitCode = wec;
    }
    out->checkPoint = ssp.dwCheckPoint;
    out->waitHint = ssp.dwWaitHint;
    out->processId = ssp.dwProcessId;
    if (ssp.dwServiceFlags & SERVICE_RUNS_IN_SYSTEM_PROCESS) {
        out->flags = WinSvcStat::Flags::kInSystemProcess;
    } else {
        out->flags = WinSvcStat::Flags::kNormal;
    }
    return {};
}

//===========================================================================
static error_code queryStat(
    WinSvcStat * out,
    const FuncInfo & info
) {
    *out = {};
    DWORD needed = 0;
    for (;;) {
        auto&& [ssp, count] = reserve<SERVICE_STATUS_PROCESS>(info, needed);
        if (count > 8192) {
            // The maximum allowed size of this buffer is 8k.
            count = 8192;
        }
        if (QueryServiceStatusEx(
            info.svc,
            SC_STATUS_PROCESS_INFO,
            (BYTE *) ssp,
            (DWORD) count,
            &needed
        )) {
            return parseStat(out, info, "QueryServiceStatusEx", *ssp);
        }
        WinError err;
        if (err != ERROR_INSUFFICIENT_BUFFER)
            return reportError(info, "QueryServiceStatusEx", {});
    }
}

//===========================================================================
error_code Dim::winSvcQuery(
    WinSvcConf * conf,
    WinSvcStat * stat,
    string_view svcName
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
    vector<WinSvcStat> * out,
    const WinSvcFilter & filter
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
            WinSvcStat st;
            if (auto ec = parseStat(&st, info, "EnumServicesStatusExW", ssp))
                return ec;
            st.serviceName = toString(sys.lpServiceName);
            st.displayName = toString(sys.lpDisplayName);
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


/****************************************************************************
*
*   Control
*
***/

//===========================================================================
static error_code waitForChange(
    WinSvcStat * out,
    const FuncInfo & info
) {
    assert(out->state != WinSvcStat::State::kInvalid);
    auto oldState = out->state;
    auto oldCheckPoint = out->checkPoint;
    auto startTime = GetTickCount64();

    auto minWait = 100u; // initial minimum wait time (milliseconds)
    auto maxWait = 10'000u; // 10 seconds
    for (;;) {
        auto time = clamp(out->waitHint / 10, minWait, maxWait);
        this_thread::sleep_for(chrono::milliseconds(time));

        if (auto ec = queryStat(out, info))
            return ec;
        if (out->state != oldState)
            return {};
        if (minWait < maxWait)
            minWait += 100u;

        auto now = GetTickCount64();
        if (out->checkPoint > oldCheckPoint) {
            oldCheckPoint = out->checkPoint;
            startTime = now;
        } else if (out->waitHint < now - startTime) {
            return WinError(ERROR_SERVICE_REQUEST_TIMEOUT).code();
        }
    }
}

//===========================================================================
// Attempt to start service.
// If first attempt and service was already running:
//   If StopPending: Wait for transition to new state.
//   If Stopped: Start second attempt.
// If wait and StartPending: Wait for transition to new state.
// If StopPending or Stopped: return ERROR_SERVICE_NOT_ACTIVE
// return NO_ERROR
error_code Dim::winSvcStart(
    WinSvcStat * out, // Set to nullptr if status not wanted
    string_view svcName,
    const vector<string> & args,
    bool wait
) {
    using enum WinSvcStat::State;
    WinSvcStat st;
    if (!out)
        out = &st;

    auto kInfoName = "START";
    auto info = FuncInfo(svcName);
    if (auto ec = openScm(&info, SC_MANAGER_CONNECT, kInfoName))
        return ec;
    DWORD access = SERVICE_START | wait * SERVICE_QUERY_STATUS;
    if (auto ec = openSvc(&info, access, kInfoName))
        return ec;

    vector<const wchar_t *> wptrs(args.size());
    vector<wstring> wargs(args.size());
    for (auto&& arg : args)
        wargs.push_back(toWstring(arg));
    for (auto&& warg : wargs)
        wptrs.push_back(warg.c_str());

    auto attempt = 0;

TRY_START:
    attempt += 1;
    auto wasRunning = false;
    if (!StartServiceW(info.svc, (DWORD) wptrs.size(), wptrs.data())) {
        WinError err;
        if (err == ERROR_SERVICE_ALREADY_RUNNING) {
            wasRunning = true;
        } else {
            return reportError(info, kInfoName);
        }
    }
    if (auto ec = queryStat(out, info))
        return ec;

    if (attempt == 1 && wasRunning) {
        if (out->state == kStopPending) {
            if (auto ec = waitForChange(out, info))
                return ec;
        }
        if (out->state == kStopped) {
            goto TRY_START;
        } else if (out->state == kStartPending) {
            // proceed to final pending
        } else {
            return {};
        }
    }

    if (wait && out->state == kStartPending) {
        if (auto ec = waitForChange(out, info))
            return ec;
    }

    if (out->state == kStopped || out->state == kStopPending) {
        return WinError(ERROR_SERVICE_NOT_ACTIVE).code();
    }
    return {};
}

//===========================================================================
error_code Dim::winSvcStop(
    WinSvcStat * out,
    string_view svcName,
    WinSvcCtrl::Reason reason,
    string_view comment,
    bool wait
) {
    WinSvcStat st;
    if (!out)
        out = &st;

    auto kInfoName = "STOP";
    auto info = FuncInfo(svcName);
    if (auto ec = openScm(&info, SC_MANAGER_CONNECT, kInfoName))
        return ec;
    DWORD access = SERVICE_STOP | wait * SERVICE_QUERY_STATUS;
    if (auto ec = openSvc(&info, access, kInfoName))
        return ec;

    assert(comment.size() <= 127);
    SERVICE_CONTROL_STATUS_REASON_PARAMSW oparams = {};
    oparams.dwReason = bit_cast<unsigned>(reason);
    wstring wcmt;
    if (!wcmt.empty()) {
        wcmt = toWstring(comment);
        oparams.pszComment = wcmt.data();
    }
    WinError err = NO_ERROR;
    if (!ControlServiceExW(
        info.svc,
        SERVICE_CONTROL_STOP,
        SERVICE_CONTROL_STATUS_REASON_INFO,
        &oparams
    )) {
        err.set();
        if (err == ERROR_SERVICE_NOT_ACTIVE) {
            // Already stopped. This error still populates ServiceStatus.
        } else if (err == ERROR_SERVICE_CANNOT_ACCEPT_CTRL) {
            // Was already stopping. This error still populates ServiceStatus.
        } else {
            return reportError(info, kInfoName);
        }
    }
    if (auto ec = parseStat(out, info, kInfoName, oparams.ServiceStatus))
        return ec;

    if (!wait)
        return {};

    auto wasPending = false;
    for (;;) {
        if (out->state == WinSvcStat::State::kStopped) {
            return {};
        } else if (out->state == WinSvcStat::State::kStopPending) {
            wasPending = true;
        } else if (wasPending) {
            // The stop is no longer pending, but it's not stopped. The service
            // must have been successfully stopped and subsequently restarted
            // by something else.
            return {};
        }

        if (auto ec = waitForChange(out, info))
            return ec;
    }
}

//===========================================================================
static error_code control(
    WinSvcStat * out,
    string_view svcName,
    bool wait,
    const char infoName[],
    unsigned control,
    WinSvcStat::State action,
    WinSvcStat::State pendingAction
) {
    WinSvcStat st;
    if (!out)
        out = &st;

    auto info = FuncInfo(svcName);
    if (auto ec = openScm(&info, SC_MANAGER_CONNECT, infoName))
        return ec;
    DWORD access = SERVICE_PAUSE_CONTINUE | wait * SERVICE_QUERY_STATUS;
    if (auto ec = openSvc(&info, access, infoName))
        return ec;

    SERVICE_CONTROL_STATUS_REASON_PARAMSW oparams = {};
    WinError err = NO_ERROR;
    if (!ControlServiceExW(
        info.svc,
        control,
        SERVICE_CONTROL_STATUS_REASON_INFO,
        &oparams
    )) {
        err.set();
    }
    if (err == NO_ERROR
        || err == ERROR_INVALID_SERVICE_CONTROL
        || err == ERROR_SERVICE_CANNOT_ACCEPT_CTRL
        || err == ERROR_SERVICE_NOT_ACTIVE
    ) {
        if (auto ec = parseStat(out, info, infoName, oparams.ServiceStatus))
            return ec;
    }
    if (err)
        return err.code();

    if (wait && out->state == pendingAction) {
        if (auto ec = waitForChange(out, info))
            return ec;
    }
    return {};
}

//===========================================================================
error_code Dim::winSvcPause(
    WinSvcStat * out, // Set to nullptr if status not wanted
    string_view svcName,
    bool wait
) {
    using enum WinSvcStat::State;
    return control(
        out,
        svcName,
        wait,
        "PAUSE",
        SERVICE_CONTROL_PAUSE,
        kPaused,
        kPausePending
    );
}

//===========================================================================
error_code Dim::winSvcContinue(
    WinSvcStat * out, // Set to nullptr if status not wanted
    string_view svcName,
    bool wait
) {
    using enum WinSvcStat::State;
    return control(
        out,
        svcName,
        wait,
        "CONTINUE",
        SERVICE_CONTROL_CONTINUE,
        kRunning,
        kContinuePending
    );
}
