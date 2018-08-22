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
        kDemand,
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

    const char * serviceName {nullptr};
    Type serviceType {};
    Start startType {};
    ErrCtrl errorControl {};
    const char * progWithArgs {nullptr};
    const char * loadOrderGroup {nullptr};
    unsigned loadOrderTag {};
    std::vector<const char *> deps;
    const char * account {nullptr};
    const char * password {nullptr};
    const char * displayName {nullptr};
    const char * desc {nullptr};

    bool failureFlag {false};
    Duration failureReset;
    const char * rebootMsg {nullptr}; // used by FailAction::kReboot
    const char * failureProgWithArgs {nullptr};
    std::vector<Action> failureActions;

    unsigned preferredNode = (unsigned) -1; // -1 for none
    Duration preshutdownTimeout;
    std::vector<const char *> privs;
    SidType sidType {};

    // NOTE: TRIGGERS NOT SUPPORTED
    std::vector<Trigger> triggers;
};

bool winSvcInstall(const WinServiceConfig & sconf);

} // namespace