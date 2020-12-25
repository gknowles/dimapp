// Copyright Glen Knowles 2020.
// Distributed under the Boost Software License, Version 1.0.
//
// docgen.cpp - docgen
#include "pch.h"
#pragma hdrstop

using namespace std;
using namespace Dim;


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
    cli.header("docgen v"s + kVersion + " (" __DATE__ ")")
        .helpNoArgs()
        .helpCmd()
        .versionOpt(kVersion, "docgen");

    shutdownMonitor(&s_cleanup);
    cli.exec(argc, argv);
    appSignalUsageError();
}


/****************************************************************************
*
*   Externals
*
***/

//===========================================================================
int main(int argc, char * argv[]) {
    return appRun(app, argc, argv);
}
