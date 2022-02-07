// Copyright Glen Knowles 2018 - 2022.
// Distributed under the Boost Software License, Version 1.0.
//
// svcctrl-t.cpp - dim test svcctrl
#include "pch.h"
#pragma hdrstop

using namespace std;
using namespace Dim;


/****************************************************************************
*
*   Declarations
*
***/

#define EXPECT(...)                                                         \
    if (!bool(__VA_ARGS__)) \
        failed(#__VA_ARGS__)


/****************************************************************************
*
*   Variables
*
***/


/****************************************************************************
*
*   Helpers
*
***/

//===========================================================================
static void failed(
    const char msg[], 
    source_location sloc = source_location::current()
) {
    logMsgError(sloc) << "Line " << sloc.line() 
        << ": EXPECT(" << msg << ") failed";
}

//===========================================================================
void internalTest() {
    static constexpr auto svcName = "svcctrl-test"sv;
    error_code ec;
    WinServiceStatus st;
    WinServiceConfig conf;

    vector<WinServiceStatus> states;
    WinServiceFilter filter;
    filter.names.insert(string(svcName));
    ec = winSvcFind(&states, filter);
    EXPECT(!ec);

    if (envProcessRights() != kEnvUserAdmin) {
        logMsgWarn() << "Run without admin rights, most tests skipped.";
        return;
    }

    if (!states.empty()) {
        ec = winSvcDelete(svcName);
        EXPECT(!ec);
    }

    conf = {};
    conf.serviceName = svcName;
    conf.displayName = conf.serviceName + " display";
    conf.desc = "Test service description.";
    conf.progWithArgs = Cli::toCmdlineL(envExecPath(), "arg1");
    conf.sidType = WinServiceConfig::SidType::kRestricted;
    conf.deps = { "Tcpip", "Afd" };
    conf.failureFlag = WinServiceConfig::FailureFlag::kCrashOrNonZeroExitCode;
    conf.failureReset = 24h;
    conf.failureActions = {
        { WinServiceConfig::Action::kRestart, 10s },
        { WinServiceConfig::Action::kRestart, 60s },
        { WinServiceConfig::Action::kRestart, 10min },
    };
    ec = winSvcCreate(conf);
    EXPECT(!ec);
    ec = winSvcQuery(svcName, &conf);
    EXPECT(!ec);
    // EXPECT(ec.value() == 1060);
    ec = winSvcDelete(svcName);
    EXPECT(!ec);

#if 0
    WinServiceConfig conf;
    conf.serviceName("Tismet")
        .displayName("Tismet Server")
        .desc("Provides efficient storage, processing, and access to time "
            "series metrics for praphing and monitoring applications.")
        .progWithArgs(Cli::toCmdlineL(envExecPath(), "serve"));
    conf.account(conf.kLocalService)
        .sidType(conf.SidType::kRestricted)
        .privs({
            "SeChangeNotifyPrivilege",
            // "SeManageVolumePrivilege", // SetFileValidData
            // "SeLockMemoryPrivilege",   // VirtualAlloc with MEM_LARGE_PAGES
        });
    conf.deps({ "Tcpip", "Afd" });
    conf.failureFlag(conf.FailureFlag::kCrashOrNonZeroExitCode)
        .failureReset(24h)
        .addFailureRestart(10s)
        .addFailureRestart(60s)
        .addFailureRestart(10min);
    conf.failureActions();
    conf.addCustomTrigger()
        .stopService()
        .eventProviderGuid("00000000-0000-0000-0000-000000000000")
        .addBinaryData("lksjdf"sv);
    conf.addDeviceTrigger()
        .stopService()
        .deviceInterfaceGuid(GUID_DEVINTERFACE_DISK)
        .addId("USBSTOR\\GenDisk");    // one or more
    conf.addNetworkStartTrigger();
    conf.addNetworkStopTrigger();
    conf.addDomainJoinTrigger()
        .stopService();    
    conf.addDomainLeaveTrigger()
        .startService();
    conf.addPortOpenTrigger()
        .addPort(portNum, protocolName, imagePath = "System", serviceName = {});
    conf.addPortCloseTrigger()
        .addPort(portNum, protocolName, imagePath = "System", serviceName = {});
    conf.addMachinePolicyTrigger();
    conf.addUserPolicyTrigger();
    conf.addNamedPipeTrigger()
        .pipeName("my/pipe/name");  // The \\.\pipe\ prefix is optional
    conf.addRpcTrigger()
        .interfaceGuid("00000000-0000-0000-0000-000000000000");
    conf.triggers();
#endif
}


/****************************************************************************
*
*   External
*
***/

//===========================================================================
void app(int argc, char * argv[]) {
    Cli cli;
    cli.helpNoArgs();
    auto & test = cli.opt<bool>("test.")
        .desc("Run internal unit tests");
    if (!cli.parse(cerr, argc, argv))
        return appSignalUsageError();

    if (*test)
        internalTest();
    testSignalShutdown();
}


/****************************************************************************
*
*   External
*
***/

//===========================================================================
int main(int argc, char * argv[]) {
    return appRun(app, argc, argv);
}
