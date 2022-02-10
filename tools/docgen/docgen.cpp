// Copyright Glen Knowles 2020 - 2022.
// Distributed under the Boost Software License, Version 1.0.
//
// docgen.cpp - docgen
#include "pch.h"
#pragma hdrstop

using namespace std;
using namespace Dim;


/****************************************************************************
*
*   Tuning parameters
*
***/

const VersionInfo kVersion = { 1, 1, 2 };


/****************************************************************************
*
*   ShutdownNotify
*
***/

namespace {
class ShutdownNotify : public IShutdownNotify {
    void onShutdownClient(bool firstTry) override;
};
} // namespace
static ShutdownNotify s_cleanup;

//===========================================================================
void ShutdownNotify::onShutdownClient(bool firstTry) {
    if (firstTry)
        execCancelWaiting();
}


/****************************************************************************
*
*   Application
*
***/

//===========================================================================
static void app(int argc, char * argv[]) {
    Cli cli;
    cli.helpNoArgs()
        .helpCmd();

    shutdownMonitor(&s_cleanup);
    if (!cli.parse(argc, argv))
        return appSignalUsageError();
    appSignalShutdown(cli.exec());
}


/****************************************************************************
*
*   Externals
*
***/

//===========================================================================
int main(int argc, char * argv[]) {
    return appRun(app, argc, argv, kVersion);
}
