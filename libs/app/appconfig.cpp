// Copyright Glen Knowles 2016 - 2017.
// Distributed under the Boost Software License, Version 1.0.
//
// appconfig.cpp - dim app
#include "pch.h"
#pragma hdrstop

using namespace std;
using namespace Dim;


/****************************************************************************
*
*   Private
*
***/


/****************************************************************************
*
*   Variables
*
***/

static FileMonitorHandle s_hDir;


/****************************************************************************
*
*   Shutdown
*
***/

namespace {

class Shutdown : public IShutdownNotify {
    void onShutdownConsole(bool retry) override;
};
static Shutdown s_cleanup;

//===========================================================================
void Shutdown::onShutdownConsole(bool retry) {
    fileMonitorStopSync(s_hDir);
}

} // namespace


/****************************************************************************
*
*   Internal API
*
***/

//===========================================================================
void Dim::iAppConfigInitialize () {
    shutdownMonitor(&s_cleanup);
}


/****************************************************************************
*
*   Public API
*
***/

//===========================================================================
void Dim::appConfigMonitor(std::string_view file, IAppConfigNotify * notify) {
}

//===========================================================================
void Dim::appConfigChange(
    std::string_view file, 
    IAppConfigNotify * notify // = nullptr
) {
}
