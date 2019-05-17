// Copyright Glen Knowles 2017 - 2018.
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

    char const * serviceName {};        // REQUIRED
    Type serviceType {};                // Default: kOwn
    Start startType {};                 // Default: kAuto
    ErrCtrl errorControl {};            // Default: kNormal
    char const * progWithArgs {};       // Default: path to this executable
    char const * loadOrderGroup {};     // Default: none
    unsigned loadOrderTag {};           // Default: none
    std::vector<char const *> deps;     // Default: none
    char const * account {};            // Default: LocalSystem
    char const * password {};
    char const * displayName {};        // Default: copies from service name
    char const * desc {};               // Default: none

    FailureFlag failureFlag {};         // Default: kCrashOnly
    Duration failureReset {};           // Default: 0 which translates to ???
    char const * rebootMsg {}; // used by FailAction::kReboot
    char const * failureProgWithArgs {}; // required by FailAction::kRunCommand
    std::vector<Action> failureActions; // Default: none

    // An explicit node number or one of the PreferredNode::Type values.
    unsigned preferredNode {(unsigned) PreferredNode::kInvalid};
                                        // Default: none

    Duration preshutdownTimeout {};
    std::vector<char const *> privs;
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

bool winSvcCreate(WinServiceConfig const & sconf);
bool winSvcDelete(std::string_view svcName);

// Update all service parameters to match the configuration, as if it had
// just been created via winSvcCreate.
bool winSvcReplace(WinServiceConfig const & sconf);

// Updates the service parameters explicitly specified. To explicitly update
// a vector to remove all values set it to a contain a single "empty" value
// (nullptr for deps and privs, member of type kInvalid for failureActions
// and triggers).
bool winSvcUpdate(WinServiceConfig const & sconf);

bool winSvcQuery(WinServiceConfig * sconf, std::string_view svcName);
bool winSvcStart(std::string_view svcName, bool wait = true);
bool winSvcStop(std::string_view svcName, bool wait = true);

// Returns the service names of services that match all the specified 
// configuration and status parameters. If 'conf' and 'stat' are both
// default constructed (no filter) the names of all services are returned.
std::vector<std::string> winSvcFind(
    WinServiceConfig const & conf,
    WinServiceStatus const & stat
);

} // namespace
