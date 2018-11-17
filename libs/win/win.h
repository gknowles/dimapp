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

    char const * serviceName {};
    Type serviceType {};
    Start startType {};
    ErrCtrl errorControl {};
    char const * progWithArgs {};
    char const * loadOrderGroup {};
    unsigned loadOrderTag {};
    std::vector<char const *> deps;
    char const * account {};
    char const * password {};
    char const * displayName {};
    char const * desc {};

    bool failureFlag {};
    Duration failureReset {};
    char const * rebootMsg {}; // used by FailAction::kReboot
    char const * failureProgWithArgs {}; // used by FailAction::kRunCommand
    std::vector<Action> failureActions;

    unsigned preferredNode {(unsigned) -1}; // -1 for none
    Duration preshutdownTimeout {};
    std::vector<char const *> privs;
    SidType sidType {};

    // NOTE: TRIGGERS NOT IMPLEMENTED
    std::vector<Trigger> triggers;
};

bool winSvcInstall(WinServiceConfig const & sconf);

} // namespace
