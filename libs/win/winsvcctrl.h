// Copyright Glen Knowles 2017 - 2022.
// Distributed under the Boost Software License, Version 1.0.
//
// winsvcctrl.h - dim windows platform
#pragma once

#include "cppconf/cppconf.h"

#include <chrono>
#include <optional>
#include <string>
#include <system_error>
#include <unordered_set>
#include <vector>

namespace Dim {


/****************************************************************************
*
*   Service
*
***/

struct WinServiceConfig {
    // Well known service accounts
    static constexpr char kLocalSystem[] = "LocalSystem";
    static constexpr char kLocalService[] = "NT AUTHORITY\\LocalService";
    static constexpr char kNetworkService[] = "NT AUTHORITY\\NetworkService";

    std::string serviceName;                // REQUIRED

    enum class Type {
        kInvalid,
        kOwn,
        kShared,
        kKernel,
        kFileSys,
        kRecognizer,
        kAdapter,
        kUserOwn,
        kUserShared,

        kDefault = kOwn,
    };
    Type serviceType {};
    
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
    Start startType {};

    enum class ErrCtrl {
        kInvalid,
        kIgnore,
        kNormal,
        kSevere,
        kCritical,

        kDefault = kNormal,
    };
    ErrCtrl errorControl {};   

    std::optional<std::string> progWithArgs;               
                                            // Default: path to this executable
    std::optional<std::string> loadOrderGroup;     
                                            // Default: none
    std::optional<unsigned> loadOrderTag {};           
                                            // Default: none
    std::optional<std::vector<std::string>> deps;          
                                            // Default: none
    std::optional<std::string> account;     // Default: NT Service\<serviceName>
    std::optional<std::string> password;    // Default: none
    std::optional<std::string> displayName; // Default: <serviceName>
    std::optional<std::string> desc;        // Default: none

    enum class FailureFlag {
        kInvalid,
        kCrashOnly,
        kCrashOrNonZeroExitCode,

        kDefault = kCrashOnly,
    };
    FailureFlag failureFlag {};             // Default: kCrashOnly
    
    std::optional<std::chrono::seconds> failureReset {};               
                                            // Default: none
    std::optional<std::string> rebootMsg; // used with FailAction::kReboot

    // required by FailAction::kRunCommand
    std::string failureProgWithArgs; 

    struct Action {
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
    std::optional<std::vector<Action>> failureActions;     
                                            // Default: none

    enum class PreferredNode {
        kInvalid = -1,
        kNone = -2,

        kDefault = kNone,
    };
    // An explicit node number or one of the PreferredNode values.
    unsigned preferredNode {(unsigned) PreferredNode::kInvalid};
                                            // Default: none

    std::optional<std::chrono::milliseconds> preshutdownTimeout {};
                                            // Default: none

    std::optional<std::vector<std::string>> privs;

    enum class SidType {
        kInvalid,
        kNone,
        kUnrestricted,
        kRestricted,

        kDefault = kNone,
    };
    SidType sidType {};        

    // Can be enabled if the service account is kLocalSystem and the service 
    // type is kOwn, kShared, kUserOwn, or kUserShared.
    // https://docs.microsoft.com/windows/win32/services/interactive-services
    bool interactive {}; 

    struct Trigger {
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
        };
        std::vector<std::string> dataItems;
    };
    // NOTE: SUPPORT FOR TRIGGERS NOT IMPLEMENTED
    std::optional<std::vector<Trigger>> triggers;

    enum class LaunchProt {
        kInvalid,
        kNone,
        kWindows,
        kWindowsLight,
        kAntimalwareLight,

        kDefault = kNone,
    };
    // Only allowed if executable is be signed.
    LaunchProt launchProt {};           
};

struct WinServiceStatus {
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
    enum class Control : unsigned {
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
    WinServiceConfig::Type serviceType{};
    bool interactive{};

    State state{};
    unsigned accepted{}; // conbination of Control flags
    unsigned exitCode{};
    unsigned checkPoint{};
    unsigned waitHint{};
    unsigned processId{};
    Flags flags{};
};


std::error_code winSvcCreate(const WinServiceConfig & sconf);
std::error_code winSvcDelete(std::string_view svcName);

// Update all service parameters to match the configuration, as if it had just
// been created via winSvcCreate.
std::error_code winSvcReplace(const WinServiceConfig & sconf);

// Updates the service parameters explicitly specified.
std::error_code winSvcUpdate(const WinServiceConfig & sconf);

std::error_code winSvcQuery(
    std::string_view svcName,
    WinServiceConfig * conf = nullptr,
    WinServiceStatus * stat = nullptr
);

// Returns the service names of services that match all the specified
// configuration and status parameters. If 'conf' and 'stat' are both default
// constructed (no filter) the names of all services are returned.
struct WinServiceFilter {
    std::unordered_set<std::string> names;
    std::unordered_set<WinServiceConfig::Type> types;
    std::unordered_set<WinServiceStatus::State> states;
    std::unordered_set<unsigned> processIds;
};
std::error_code winSvcFind(
    std::vector<WinServiceStatus> * out,
    const WinServiceFilter & filter = {}
);

std::error_code winSvcGrantLogonAsService(std::string_view account);

std::error_code winSvcStart(std::string_view svcName, bool wait = true);
std::error_code winSvcStop(std::string_view svcName, bool wait = true);

} // namespace
