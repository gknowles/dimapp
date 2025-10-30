// Copyright Glen Knowles 2018 - 2025.
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
    string opName;
    bool logErrors = true;
    mutable vector<std::byte> buf;

    FuncInfo(string_view svcName, string_view opName);
    ~FuncInfo();
};

struct OsSvcConf {
    const wchar_t * svcName;
    DWORD svcType = 0;
    DWORD startType = 0;
    DWORD errCtrl = 0;
    const wchar_t * binPath = nullptr;
    const wchar_t * loadGroup = nullptr;
    DWORD * tagIdPtr = nullptr;
    const wchar_t * deps = nullptr;
    const wchar_t * account = nullptr;
    const wchar_t * password = nullptr;
    const wchar_t * dname = nullptr;

    const wchar_t * addPtr(const string & val);
    void setTagPtr(const optional<unsigned> & val);

private:
    list<wstring> strs;
    DWORD rawTagId = 0;
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

//===========================================================================
template<typename T>
static string toMultistring(const T & strings) {
    string out;
    for (auto && str : strings)
        out.append(str).push_back('\0');
    out.push_back('\0');
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
*   FuncInfo
*
***/

//===========================================================================
FuncInfo::FuncInfo(string_view svcName, string_view opName)
    : svcName{svcName}
    , opName(opName)
{}

//===========================================================================
FuncInfo::~FuncInfo() {
    if (this->svc)
        CloseServiceHandle(this->svc);
    if (this->scm)
        CloseServiceHandle(this->scm);
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
template<typename T>
static pair<T *, size_t> reserve(
    vector<const wchar_t *> * ptrs,
    const FuncInfo & info
) {
    struct WithPad {
        T val;
        wchar_t buf[1];
    };
    size_t cnt = 0;
    for (auto&& ptr : *ptrs)
        cnt += wcslen(ptr) + 1;
    cnt = offsetof(WithPad, buf) + cnt * sizeof(wchar_t);
    auto&& [val, bytes] = reserve<WithPad>(info, cnt);
    auto base = val->buf;
    for (auto&& ptr : *ptrs) {
        wcscpy(base, ptr);
        ptr = base;
        base += wcslen(base) + 1;
    }
    return {&val->val, bytes};
}


/****************************************************************************
*
*   OsSvcConf
*
***/

//===========================================================================
const wchar_t * OsSvcConf::addPtr(const string & val) {
    return this->strs.emplace_back(toWstring(val)).c_str();
}

//===========================================================================
void OsSvcConf::setTagPtr(const optional<unsigned> & val) {
    if (val) {
        this->rawTagId = (DWORD) *val;
        this->tagIdPtr = &this->rawTagId;
    } else {
        this->tagIdPtr = nullptr;
    }
}


/****************************************************************************
*
*   Translate values
*
***/

namespace {
const EnumMap s_svcTypes[] = {
    { (int) WinSvcConf::Type::kInvalid, SERVICE_NO_CHANGE },
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
    { (int) WinSvcConf::Start::kInvalid, SERVICE_NO_CHANGE },
    { (int) WinSvcConf::Start::kAuto, SERVICE_AUTO_START },
    { (int) WinSvcConf::Start::kAutoDelayed, SERVICE_AUTO_START },
    { (int) WinSvcConf::Start::kBoot, SERVICE_BOOT_START },
    { (int) WinSvcConf::Start::kDemand, SERVICE_DEMAND_START },
    { (int) WinSvcConf::Start::kDisabled, SERVICE_DISABLED },
    { (int) WinSvcConf::Start::kSystem, SERVICE_SYSTEM_START },
};
const EnumMap s_svcErrCtrls[] = {
    { (int) WinSvcConf::ErrCtrl::kInvalid, SERVICE_NO_CHANGE },
    { (int) WinSvcConf::ErrCtrl::kIgnore, SERVICE_ERROR_IGNORE },
    { (int) WinSvcConf::ErrCtrl::kNormal, SERVICE_ERROR_NORMAL },
    { (int) WinSvcConf::ErrCtrl::kSevere, SERVICE_ERROR_SEVERE },
    { (int) WinSvcConf::ErrCtrl::kCritical, SERVICE_ERROR_CRITICAL },
};
const EnumMap s_svcSidTypes[] = {
    {
        (int) WinSvcConf::SidType::kInvalid,
        SERVICE_NO_CHANGE
    },
    {
        (int) WinSvcConf::SidType::kNone,
        SERVICE_SID_TYPE_NONE
    },
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
        (int) WinSvcConf::LaunchProt::kInvalid,
        SERVICE_NO_CHANGE
    },
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
requires (!is_same_v<E, WinSvcConf::Type>)
static E getValue(const T & tbl, DWORD osvalue, E def = {}) {
    for (auto && v : tbl) {
        if (v.osvalue == osvalue)
            return static_cast<E>(v.value);
    }
    return def;
}

//===========================================================================
static WinSvcConf::Type getSvcType(
    DWORD osvalue,
    WinSvcConf::Type def = WinSvcConf::Type::kInvalid
) {
    auto value = def;
    for (auto&& em : s_svcTypes) {
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
requires (is_enum_v<E>)
static DWORD getOsValue(const T & tbl, E value) {
    for (auto && v : tbl) {
        if (v.value == (int) value)
            return v.osvalue;
    }
    return 0;
}

//===========================================================================
template<typename E, typename T>
static DWORD getOsValue(
    const T & tbl,
    optional<E> value,
    bool enableDefault = true
) {
    if (!value) {
        if (enableDefault) {
            value = E::kDefault;
        } else {
            return SERVICE_NO_CHANGE;
        }
    }
    return getOsValue(tbl, *value);
}

//===========================================================================
static OsSvcConf getOsConf(const WinSvcConf & conf) {
    OsSvcConf out;
    out.svcName = out.addPtr(conf.serviceName);
    out.svcType = getOsValue(s_svcTypes, conf.serviceType, false);
    if (conf.interactive && out.svcType != SERVICE_NO_CHANGE)
        out.svcType |= SERVICE_INTERACTIVE_PROCESS;
    out.startType = getOsValue(s_svcStarts, conf.startType, false);
    out.errCtrl = getOsValue(s_svcErrCtrls, conf.errorControl, false);
    out.binPath = out.addPtr(*conf.progWithArgs);
    out.loadGroup = out.addPtr(conf.loadOrderGroup.value_or({}));
    out.setTagPtr(conf.loadOrderTag);
    if (conf.deps)
        out.deps = out.addPtr(toMultistring(*conf.deps));
    out.account = out.addPtr(*conf.account);
    if (conf.password)
        out.password = out.addPtr(*conf.password);
    out.dname = out.addPtr(*conf.displayName);
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
static error_code openScm(FuncInfo * info, DWORD access) {
    info->scm = OpenSCManagerW(NULL, NULL, access);
    if (!info->scm)
        return reportError(*info, "OpenSCManagerW", info->opName);
    return {};
}

//===========================================================================
static error_code openSvc(FuncInfo * info, DWORD access) {
    info->svc = OpenServiceW(
        info->scm,
        toWstring(info->svcName).c_str(),
        access
    );
    if (!info->svc)
        return reportError(*info, "OpenServiceW", info->opName);
    return {};
}

//===========================================================================
// Checks for required but unspecified parameters and either sets them to
// defaults or, when that's not possible, returns an error.
static error_code prepareForCreate(WinSvcConf * out) {
    if (out->serviceName.empty()) {
        logMsgError() << "WinSvcConf: no service name";
        return make_error_code(errc::invalid_argument);
    }
    if (out->loadOrderTag) {
        if (!out->loadOrderGroup) {
            logMsgError() << "winInstallService: "
                "load order tag without load order group";
            return make_error_code(errc::invalid_argument);
        }
        if (out->startType != WinSvcConf::Start::kBoot
            && out->startType != WinSvcConf::Start::kSystem
        ) {
            logMsgError() << "winInstallService: load order tag without "
                "start type of boot or system";
            return make_error_code(errc::invalid_argument);
        }
    }

    if (!out->serviceType)
        out->serviceType = WinSvcConf::Type::kDefault;
    if (!out->startType)
        out->startType = WinSvcConf::Start::kDefault;
    if (!out->errorControl)
        out->errorControl = WinSvcConf::ErrCtrl::kDefault;
    if (!out->progWithArgs)
        out->progWithArgs = envExecPath();
    if (!out->displayName)
        out->displayName = out->serviceName;
    if (!out->deps)
        out->deps = {};
    if (!out->account || out->account->empty()) {
        out->account = "NT Service\\" + out->serviceName;
    } else if (toLower(*out->account) == toLower(WinSvcConf::kLocalSystem)) {
        out->account->clear();
    }
    if (!out->password)
        out->password = {};

    return {};
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

    if (conf.failureActions
        || conf.rebootMsg
        || conf.failureProgWithArgs
    ) {
        vector<SC_ACTION> facts;
        if (conf.failureActions) {
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
        if (conf.failureProgWithArgs) {
            wcmd = toWstring(*conf.failureProgWithArgs);
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
        auto wprivs = toWstring(privs);
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

    if (conf.preferredNode) {
        SERVICE_PREFERRED_NODE_INFO ni = {};
        if (conf.preferredNode == WinSvcConf::kNoPreferredNode) {
            ni.fDelete = true;
        } else {
            ni.usPreferredNode = (USHORT) *conf.preferredNode;
        }
        if (!ChangeServiceConfig2W(
            info.svc,
            SERVICE_CONFIG_PREFERRED_NODE,
            &ni
        )) {
            return reportChange2Error(info, "PREFERRED_NODE");
        }
    }

    if (conf.launchProt) {
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
*   Create and Delete
*
***/

//===========================================================================
error_code Dim::winSvcCreate(const WinSvcConf & sconf) {
    auto conf{sconf};

    if (auto ec = prepareForCreate(&conf))
        return ec;
    auto info = FuncInfo(conf.serviceName, "CREATE");
    if (auto ec = openScm(&info, SC_MANAGER_CREATE_SERVICE))
        return ec;

    auto c = getOsConf(conf);
    info.svc = CreateServiceW(
        info.scm,
        c.svcName,
        c.dname,
        DELETE  // to delete if following config fails
            | SERVICE_CHANGE_CONFIG // to set config2 options
            | SERVICE_START,        // to set failure actions
        c.svcType,
        c.startType,
        c.errCtrl,
        c.binPath,
        c.loadGroup,
        c.tagIdPtr,
        c.deps,
        c.account,
        c.password
    );
    if (!info.svc)
        return reportError(info, "CreateServiceW");
    Finally sDel([svc = info.svc]() { DeleteService(svc); });

    if (auto ec = changeAllConf2(info, conf))
        return ec;
    if (auto ec = winSvcGrantLogonAsService(*conf.account))
        return ec;

    sDel = {};
    return {};
}

//===========================================================================
error_code Dim::winSvcDelete(std::string_view svcName) {
    assert(!svcName.empty());

    auto info = FuncInfo{svcName, "DELETE"};
    if (auto ec = openScm(&info, SC_MANAGER_CONNECT))
        return ec;
    if (auto ec = openSvc(&info, DELETE))
        return ec;

    if (!DeleteService(info.svc))
        return reportError(info, "DeleteService");

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

    out->serviceType = getSvcType(cfg->dwServiceType);
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
        out->preferredNode = cfg->fDelete
            ? WinSvcConf::kNoPreferredNode
            : cfg->usPreferredNode;
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
        if (*out->launchProt == WinSvcConf::LaunchProt::kInvalid) {
            return reportQuery2BadRange(
                info,
                "LAUNCH_PROTECTED",
                format(
                    "unknown launch protected ({})",
                    toString(cfg->dwLaunchProtected)
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
    out->serviceType = getSvcType(ssp.dwServiceType);
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
    if (auto sf = ssp.dwServiceFlags) {
        using enum WinSvcStat::Flags;
        struct {
            WinSvcStat::Flags flag;
            DWORD osflag;
        } flags[] = {
            { fInSystemProcess, SERVICE_RUNS_IN_SYSTEM_PROCESS },
        };
        for (auto&& ff : flags) {
            if (sf & ff.osflag)
                out->flags |= (unsigned) ff.flag;
        }
    }
    return {};
}

//===========================================================================
static error_code queryStat(
    WinSvcStat * out,
    const FuncInfo & info
) {
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

    auto info = FuncInfo(svcName, "QUERY");
    if (auto ec = openScm(&info, SC_MANAGER_CONNECT))
        return ec;
    DWORD access = bool(conf) * SERVICE_QUERY_CONFIG
        | bool(stat) * SERVICE_QUERY_STATUS;
    if (auto ec = openSvc(&info, access))
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

    auto info = FuncInfo({}, "ENUMERATE");
    if (auto ec = openScm(&info, SC_MANAGER_ENUMERATE_SERVICE))
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
*   Update
*
***/

//===========================================================================
static error_code changeAllConf(
    const FuncInfo & info,
    const WinSvcConf & conf
) {
    auto c = getOsConf(conf);
    if (!ChangeServiceConfigW(
        info.svc,
        c.svcType,
        c.startType,
        c.errCtrl,
        c.binPath,
        c.loadGroup,
        c.tagIdPtr,
        c.deps,
        c.account,
        c.password,
        c.dname
    )) {
        return reportError(info, "ChangeServiceConfigW");
    }
    return {};
}

//===========================================================================
error_code Dim::winSvcReplace(const WinSvcConf & conf) {
    auto info = FuncInfo(conf.serviceName, "REPLACE");
    if (auto ec = openScm(&info, SC_MANAGER_CONNECT))
        return ec;
    if (auto ec = openSvc(&info, SERVICE_QUERY_CONFIG | SERVICE_CHANGE_CONFIG))
        return ec;

    WinSvcConf base = {};
    if (auto ec = queryConf(&base, info))
        return ec;
    WinSvcConf upd = {};
    if (auto ec = winSrvMakeUpdate(&upd, conf, base))
        return ec;
    if (auto ec = changeAllConf(info, conf))
        return ec;
    if (auto ec = changeAllConf2(info, conf))
        return ec;

    return {};
}

//===========================================================================
error_code Dim::winSvcUpdate(const WinSvcConf & conf) {
    auto info = FuncInfo(conf.serviceName, "UPDATE");
    if (auto ec = openScm(&info, SC_MANAGER_CONNECT))
        return ec;
    if (auto ec = openSvc(&info, SERVICE_CHANGE_CONFIG))
        return ec;

    if (auto ec = changeAllConf(info, conf))
        return ec;
    if (auto ec = changeAllConf2(info, conf))
        return ec;

    return {};
}

//===========================================================================
template<typename T>
static void dedup(optional<T> * out, const optional<T> & base) {
    if (*out == base) {
        // Values are the same, set to no change (an empty optional).
        *out = {};
    } else if (!*out) {
        // Values are different and there is no new value, set to default
        // so old value will be changed (likely via removal).
        *out = T{};
    }
}

//===========================================================================
error_code Dim::winSrvMakeUpdate(
    WinSvcConf * out,
    const WinSvcConf & targetRaw,
    const WinSvcConf & baseRaw
) {
    *out = targetRaw;
    auto base = baseRaw;
    if (auto ec = prepareForCreate(out))
        return ec;
    if (auto ec = prepareForCreate(&base))
        return ec;

    if (out->serviceName != base.serviceName) {
        // Not to be confused with the display name!
        //
        // The only way to "change" the service name is to delete it and create
        // a new one with the desired name. Well, you could also edit it in the
        // registry and then reboot the computer but...
        return make_error_code(errc::invalid_argument);
    }

    if (out->serviceType == base.serviceType
        && out->interactive == base.interactive
    ) {
        out->serviceType = {};
    }
    dedup(&out->startType, base.startType);
    dedup(&out->errorControl, base.errorControl);
    dedup(&out->progWithArgs, base.progWithArgs);
    dedup(&out->loadOrderGroup, base.loadOrderGroup);
    dedup(&out->loadOrderTag, base.loadOrderTag);
    dedup(&out->deps, base.deps);
    dedup(&out->account, base.account);
    dedup(&out->password, base.password);
    dedup(&out->displayName, base.displayName);
    dedup(&out->desc, base.desc);
    dedup(&out->failureFlag, base.failureFlag);
    dedup(&out->failureReset, base.failureReset);
    dedup(&out->rebootMsg, base.rebootMsg);
    dedup(&out->failureProgWithArgs, base.failureProgWithArgs);
    dedup(&out->failureActions, base.failureActions);
    dedup(&out->preferredNode, base.preferredNode);
    dedup(&out->preshutdownTimeout, base.preshutdownTimeout);
    dedup(&out->privs, base.privs);
    dedup(&out->sidType, base.sidType);
    dedup(&out->triggers, base.triggers);
    dedup(&out->launchProt, base.launchProt);
    return {};
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

    auto info = FuncInfo(svcName, "START");
    if (auto ec = openScm(&info, SC_MANAGER_CONNECT))
        return ec;
    DWORD access = SERVICE_START | wait * SERVICE_QUERY_STATUS;
    if (auto ec = openSvc(&info, access))
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
            return reportError(info, "StartServiceW");
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
            out->alreadyInState = true;
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

    auto info = FuncInfo(svcName, "STOP");
    if (auto ec = openScm(&info, SC_MANAGER_CONNECT))
        return ec;
    DWORD access = SERVICE_STOP | wait * SERVICE_QUERY_STATUS;
    if (auto ec = openSvc(&info, access))
        return ec;

    assert(comment.size() <= 127);
    SERVICE_CONTROL_STATUS_REASON_PARAMSW oparams = {};
    oparams.dwReason = (uint32_t) reason.general << 28
        | (uint32_t) reason.major << 16
        | (uint32_t) reason.minor;
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
        if (err == ERROR_SERVICE_NOT_ACTIVE
            || err == ERROR_SERVICE_CANNOT_ACCEPT_CTRL)
        {
            // Already stopped or stopping. This error still populates
            // ServiceStatus.
            out->alreadyInState = true;
        } else {
            return reportError(info, "ControlServiceExW");
        }
    }
    if (auto ec = parseStat(
        out,
        info,
        "ControlServiceExW",
        oparams.ServiceStatus
    )) {
        return ec;
    }

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
    const char opName[],
    unsigned control,
    WinSvcStat::State action,
    WinSvcStat::State pendingAction
) {
    WinSvcStat st;
    if (!out)
        out = &st;

    auto info = FuncInfo(svcName, opName);
    if (auto ec = openScm(&info, SC_MANAGER_CONNECT))
        return ec;
    DWORD access = SERVICE_PAUSE_CONTINUE | wait * SERVICE_QUERY_STATUS;
    if (auto ec = openSvc(&info, access))
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
        if (auto ec = parseStat(
            out,
            info,
            "ControlServiceExW",
            oparams.ServiceStatus
        )) {
            return ec;
        }
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
