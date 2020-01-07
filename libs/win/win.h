// Copyright Glen Knowles 2017 - 2020.
// Distributed under the Boost Software License, Version 1.0.
//
// win.h - dim windows platform
#pragma once

#include "cppconf/cppconf.h"

#include "core/time.h"

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

    enum class Type {
        kInvalid,
        kOwn,
        kShare,
        kKernel,
        kFileSys,
        kRecognizer,
        kAdapter,
        kUserOwn,
        kUserShare,

        kDefault = kOwn,
    };
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
    enum class ErrCtrl {
        kInvalid,
        kIgnore,
        kNormal,
        kSevere,
        kCritical,

        kDefault = kNormal,
    };
    enum class FailureFlag {
        kInvalid,
        kCrashOnly,
        kCrashOrNonZeroExitCode,

        kDefault = kCrashOnly,
    };
    struct Action {
        enum Type {
            kInvalid,
            kNone,
            kRestart,
            kReboot,
            kRunCommand,
        };
        Type type {};
        Duration delay;
    };
    struct PreferredNode {
        enum Type {
            kInvalid = -1,
            kNone = -2,

            kDefault = kNone,
        };
    };
    enum class SidType {
        kInvalid,
        kNone,
        kUnrestricted,
        kRestricted,

        kDefault = kNone,
    };
    struct Trigger {
        enum Type {
            kInvalid,
            kCustom,
            kDeviceInterfaceArrival,
            kDomainJoin,
            kFirewallPortEvent,
            kGroupPolicy,
            kIpAddressAvail,
            kNetworkEndpoint,
        };
        enum Action {
            kServiceStart,
            kServiceStop,
        };
    };

    const char * serviceName {};        // REQUIRED
    Type serviceType {};                // Default: kOwn
    Start startType {};                 // Default: kAuto
    ErrCtrl errorControl {};            // Default: kNormal
    const char * progWithArgs {};       // Default: path to this executable
    const char * loadOrderGroup {};     // Default: none
    unsigned loadOrderTag {};           // Default: none
    std::vector<const char *> deps;     // Default: none
    const char * account {};            // Default: NT Service\<serviceName>
    const char * password {};           // Default: none
    const char * displayName {};        // Default: <serviceName>
    const char * desc {};               // Default: none
    FailureFlag failureFlag {};         // Default: kCrashOnly
    Duration failureReset {};           // Default: 0 which translates to ???
    const char * rebootMsg {}; // used by FailAction::kReboot
    const char * failureProgWithArgs {}; // required by FailAction::kRunCommand
    std::vector<Action> failureActions; // Default: none

    // An explicit node number or one of the PreferredNode::Type values.
    unsigned preferredNode {(unsigned) PreferredNode::kInvalid};
                                        // Default: none

    Duration preshutdownTimeout {};
    std::vector<const char *> privs;
    SidType sidType {};

    // NOTE: SUPPORT FOR TRIGGERS NOT IMPLEMENTED
    std::vector<Trigger> triggers;
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
    };
    enum class Control : unsigned {
        kInvalid,
        kNetBindChange          = 1 << 0,
        kParamChange            = 1 << 1,
        kPauseContinue          = 1 << 2,
        kPreshutdown            = 1 << 3,
        kShutdown               = 1 << 4,
        kStop                   = 1 << 5,
        kHardwareProfileChange  = 1 << 6,
        kPowerEvent             = 1 << 7,
        kSessionChange          = 1 << 8,
    };
    enum class Flags : unsigned {
        kInvalid,
        kNormal             = 1 << 0,
        kInSystemProcess    = 1 << 1,
    };

    State state{};
    Control accepted{};
    unsigned exitCode{};
    unsigned processId{};
    Flags flags{};
};

bool winSvcCreate(const WinServiceConfig & sconf);
bool winSvcDelete(std::string_view svcName);

// Update all service parameters to match the configuration, as if it had just
// been created via winSvcCreate.
bool winSvcReplace(const WinServiceConfig & sconf);

// Updates the service parameters explicitly specified. To explicitly remove
// all values of a vector set it to a contain a single "empty" value (nullptr
// for deps and privs, member of type kInvalid for failureActions and
// triggers).
bool winSvcUpdate(const WinServiceConfig & sconf);

bool winSvcQuery(WinServiceConfig * sconf, std::string_view svcName);
bool winSvcStart(std::string_view svcName, bool wait = true);
bool winSvcStop(std::string_view svcName, bool wait = true);

// Returns the service names of services that match all the specified
// configuration and status parameters. If 'conf' and 'stat' are both default
// constructed (no filter) the names of all services are returned.
std::vector<std::string> winSvcFind(
    const WinServiceConfig & conf,
    const WinServiceStatus & stat
);

bool winSvcGrantLogonAsService(std::string_view account);


/****************************************************************************
*
*   Registry
*
***/

class WinRegTree {
public:
    enum class Registry {
        kClassesRoot,
        kCurrentConfig,
        kCurrentUser,
        kLocalMachine,
        kPerformanceData,
        kUsers,
    };
    enum class ValueType {
        kBinary,
        kDword,
        kDwordLittleEndian,
        kDwordBigEndian,
        kExpandSz,
        kLink,
        kMultiSz,
        kNone,
        kQword,
        kQwordLittleEndian,
        kSz,
    };
    struct Value {
        std::string name;
        ValueType type;
        std::string data;

        uint64_t qword() const;
        const std::string str() const;
        std::string expanded() const;
        std::vector<std::string_view> multi() const;
    };
    struct Key {
        std::string name;
        std::vector<Key> keys;
        std::vector<Value> values;
        TimePoint lastWrite;
    };

public:
    bool load (Registry key, const std::string & root);

private:
    Registry m_registry;
    Key m_root;
};

} // namespace
