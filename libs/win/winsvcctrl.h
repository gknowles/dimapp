// Copyright Glen Knowles 2017 - 2025.
// Distributed under the Boost Software License, Version 1.0.
//
// winsvcctrl.h - dim windows platform
#pragma once

#include "cppconf/cppconf.h"

#include "basic/basic.h"

#include <chrono>
#include <optional>
#include <string>
#include <system_error>
#include <unordered_set>
#include <vector>

namespace Dim {


/****************************************************************************
*
*   Service configuration
*
***/

struct WinSvcConf {
    // Well known service accounts
    static constexpr char kLocalSystem[] = "LocalSystem";
    static constexpr char kLocalService[] = "NT AUTHORITY\\LocalService";
    static constexpr char kNetworkService[] = "NT AUTHORITY\\NetworkService";

    std::string serviceName;        // REQUIRED

    enum class Type {
        kInvalid,
        kOwn,
        kShared,
        kUserOwn,
        kUserShared,
        kPackageOwn,
        kPackageShared,
        kKernelDriver,
        kFileSysDriver,
        kRecognizerDriver,
        kAdapter,

        kDefault = kOwn,
    };
    std::optional<Type> serviceType;

    // Can be enabled if the service account is kLocalSystem and the service
    // type is kOwn, kShared, kUserOwn, or kUserShared. Ignored if serviceType
    // is not set.
    // https://docs.microsoft.com/windows/win32/services/interactive-services
    bool interactive {};

    // User (or package?) specific instance of kUserOwn or kUserShared (or
    // kPackageOwn or kPackageShared?) service. The service name will be <name
    // of service template>_<LUID>.
    bool instance {}; // Can be queried, can't be changed.

    // Beginning with Windows 10 (version 1703) shared services may be split
    // off to their own SvcHost process. They are split by default if you have
    // over 3.5GB memory, see SvcHostSplitDisable in services
    // HKLM\SYSTEM\CurrentControlSet\Services entry.
    // https://docs.microsoft.com/windows/application-management/svchost-service-refactoring
    bool sharedServiceSplitEnabled {}; // Can be queried, can't be changed.

    enum class Start {
        kInvalid,
        kAuto,
        kAutoDelayed,
        kBoot,
        kDemand,    // aka Manual
        kDisabled,
        kSystem,

        kDefault = kAuto,
    };
    std::optional<Start> startType;

    enum class ErrCtrl {
        kInvalid,
        kIgnore,
        kNormal,
        kSevere,
        kCritical,

        kDefault = kNormal,
    };
    std::optional<ErrCtrl> errorControl;

    std::optional<std::string> progWithArgs; // Default: path to this program
    std::optional<std::string> loadOrderGroup;      // Default: none
    std::optional<unsigned> loadOrderTag;           // Default: none
    std::optional<std::vector<std::string>> deps;   // Default: none
    std::optional<std::string> account;     // Default: NT Service\<svcName>
    std::optional<std::string> password;    // Default: none
    std::optional<std::string> displayName; // Default: <svcName>
    std::optional<std::string> desc;        // Default: none

    enum class FailureFlag {
        kInvalid,
        kCrashOnly,
        kCrashOrNonZeroExitCode,

        kDefault = kCrashOnly,
    };
    std::optional<FailureFlag> failureFlag; // Default: kCrashOnly

    std::optional<std::chrono::seconds> failureReset; // Default: none
    std::optional<std::string> rebootMsg; // FailAction::kReboot
    std::optional<std::string> failureProgWithArgs; // FailAction::kRunCommand

    struct Action {
        bool operator==(const Action & other) const = default;

        enum class Type {
            kInvalid,
            kNone,
            kRestart,
            kReboot,        // requires SE_SHUTDOWN_NAME privilege
            kRunCommand,
        };
        using enum Type;

        Type type {};
        std::chrono::milliseconds delay;
    };
    std::optional<std::vector<Action>> failureActions;  // Default: none

    static constexpr unsigned kNoPreferredNode = (unsigned) -1;
    std::optional<unsigned> preferredNode;  // Default: kNoPreferredNode

    std::optional<std::chrono::milliseconds> preshutdownTimeout;
                                            // Default: none

    std::optional<std::vector<std::string>> privs;

    enum class SidType {
        kInvalid,
        kNone,
        kUnrestricted,
        kRestricted,

        kDefault = kNone,
    };
    std::optional<SidType> sidType;

    struct Trigger {
        bool operator==(const Trigger & other) const = default;

        enum class Type {
            kInvalid,
            kCustom,
            kDeviceArrival,
            kDomainJoin,
            kDomainLeave,
            kFirewallPortOpen,
            kFirewallPortClose,
            kGroupPolicyMachine,
            kGroupPolicyUser,
            kIpAddressArrival,
            kIpAddressRemoval,
            kNetworkNamedPipe,
            kNetworkRpc,
        };
        Type type {};

        enum class Action {
            kInvalid,
            kServiceStart,
            kServiceStop,

            kDefault = kInvalid,
        };
        Action action {};

        // For kCustom, the guid of the event provider
        // https://docs.microsoft.com/windows/win32/api/evntprov/nf-evntprov-eventregister
        //
        // For kDeviceInterfaceArrival, the device interface class
        // https://docs.microsoft.com/windows-hardware/drivers/install/overview-of-device-interface-classes
        std::string subTypeGuid {};

        enum class DataType {
            kInvalid,
            kBinary,
            kString,    // null terminate string, or multistring
            kLevel,
            kKeywordAny,
            kKeywordAll,

            kDefault = kInvalid,
        };
        struct DataItem {
            bool operator==(const DataItem & other) const = default;

            DataType type;
            std::string data;   // limited to maximum of 1024 bytes
        };
        std::vector<DataItem> dataItems;
    };
    std::optional<std::vector<Trigger>> triggers;

    enum class LaunchProt {
        kInvalid,
        kNone,
        kWindows,
        kWindowsLight,
        kAntimalwareLight,

        kDefault = kNone,
    };
    // Only allowed if executable is signed.
    std::optional<LaunchProt> launchProt;
};

std::error_code winSvcCreate(const WinSvcConf & sconf);
std::error_code winSvcDelete(std::string_view svcName);

// Update all service parameters to match the configuration, as if it had just
// been created via winSvcCreate.
std::error_code winSvcReplace(const WinSvcConf & sconf);

// Updates the service parameters explicitly specified.
std::error_code winSvcUpdate(const WinSvcConf & sconf);

// Output the delta between two configurations, which could be used in a
// following call to winSvcUpdate().
std::error_code winSrvMakeUpdate(
    WinSvcConf * out,
    const WinSvcConf & target,
    const WinSvcConf & base
);

std::error_code winSvcGrantLogonAsService(std::string_view account);


/****************************************************************************
*
*   Service status
*
***/

struct WinSvcStat {
    enum class State {
        kInvalid,
        kContinuePending,
        kPausePending,
        kPaused,
        kRunning,
        kStartPending,
        kStopPending,
        kStopped,

        kDefault = kRunning,
    };
    enum class Accept : unsigned {
        fInvalid,
        fNetBindChange          = 1 << 0,
        fParamChange            = 1 << 1,
        fPauseContinue          = 1 << 2,
        fPreshutdown            = 1 << 3,
        fShutdown               = 1 << 4,
        fStop                   = 1 << 5,
        fHardwareProfileChange  = 1 << 6,
        fPowerEvent             = 1 << 7,
        fSessionChange          = 1 << 8,
    };
    enum class Flags {
        kInvalid,
        kNormal             = 1 << 0,
        kInSystemProcess    = 1 << 1,

        kDefault = kNormal,
    };

    std::string serviceName;
    std::string displayName;
    WinSvcConf::Type serviceType{};
    bool interactive{};

    State state = State::kInvalid;
    unsigned accepted{}; // combination of Accept flags
    unsigned exitCode{};
    unsigned checkPoint{};
    unsigned waitHint{};
    unsigned processId{};
    Flags flags{};
};

std::error_code winSvcQuery(
    WinSvcConf * conf,    // Set to nullptr if config data not wanted
    WinSvcStat * stat,    // Set to nullptr if status not wanted
    std::string_view svcName
);

// Returns the service names of services that match all the specified
// configuration and status parameters. If the filter has an empty set of
// names, types, etc it will match all names, types, etc respectively. To be
// found a service must match all filter conditions (names, types, etc).
struct WinSvcFilter {
    std::unordered_set<std::string> names;
    std::unordered_set<WinSvcConf::Type> types;
    std::unordered_set<WinSvcStat::State> states;
    std::unordered_set<unsigned> processIds;
};
std::error_code winSvcFind(
    std::vector<WinSvcStat> * out,
    const WinSvcFilter & filter = {}
);


/****************************************************************************
*
*   Service control
*
***/

struct WinSvcCtrl {
    enum class General : uint32_t {
        kUnplanned = 1,

        // With kCustom, major and minor codes must be within the [min, max]
        // bounds reserved for custom values included in the enums below.
        kCustom = 2,

        kPlanned = 4,
    };
    enum class Major : uint32_t {
        kOther = 1,
        kHardware = 2,
        kOperatingSystem = 3,
        kSoftware = 4,
        kApplication = 5,
        kNone = 6,

        kMinCustom = 64,
        kMaxCustom = 255,
    };
    enum class Minor : uint32_t {
        kOther = 1,
        kMaintenance = 2,
        kInstallation = 3,
        kUpgrade = 4,
        kReconfig = 5,
        kHung = 6,
        kUnstable = 7,
        kDisk = 8,
        kNetworkCard = 9,
        kEnvironment = 10,
        kHardwareDriver = 11,
        kOtherDriver = 12,
        kServicePack = 13,
        kSoftwareUpdate = 14,
        kSecurityFix = 15,
        kSecurity = 16,
        kNetworkConnectivity = 17,
        kWMI = 18,   // Microsoft Management Instrumentation framework
        kServicePackUninstall = 19,
        kSoftwareUpdateUninstall = 20,
        kSecurityFixUninstall = 21,
        kMMC = 22,   // Microsoft Management Console framework
        kNone = 23,
        kMemoryLimit = 24,  // Misspelled as "MEMOTYLIMIT" in winsvc.h

        kMinCustom = 1,
        kMaxCustom = 65'535,
    };
    struct Reason {
        General general : 4 = General::kUnplanned;
        uint32_t gap : 4 = 0;
        Major major : 8 = Major::kNone;
        Minor minor : 16 = Minor::kNone;
    };
};

// Aims for one legitimate start attempt to be made, either by this function or
// some other parallel attempt.
std::error_code winSvcStart(
    WinSvcStat * out, // Set to nullptr if status not wanted
    std::string_view svcName,
    const std::vector<std::string> & args = {},
    bool wait = true    // wait for start pending to complete?
);

std::error_code winSvcStop(
    WinSvcStat * out, // Set to nullptr if status not wanted
    std::string_view svcName,
    WinSvcCtrl::Reason reason = {},
    std::string_view comment = {}, // Truncated to 127 characters
    bool wait = true
);
std::error_code winSvcPause(
    WinSvcStat * out, // Set to nullptr if status not wanted
    std::string_view svcName,
    bool wait = true
);
std::error_code winSvcContinue(
    WinSvcStat * out, // Set to nullptr if status not wanted
    std::string_view svcName,
    bool wait = true
);
} // namespace
