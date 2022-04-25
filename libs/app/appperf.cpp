// Copyright Glen Knowles 2022.
// Distributed under the Boost Software License, Version 1.0.
//
// appperf.cpp - dim app
#include "pch.h"
#pragma hdrstop

using namespace std;
using namespace Dim;


/****************************************************************************
*
*   Performance counters
*
***/

static TimePoint s_buildDate;

//===========================================================================
static float getBuildAge() {
    auto dur = timeNow() - s_buildDate;
    auto secs = (float) duration_cast<chrono::duration<double>>(dur).count();
    return secs;
    
}

static auto & s_perfBuildAge = fperf(
    "app.build age", 
    getBuildAge, 
    PerfFormat::kDuration
);


/****************************************************************************
*
*   Public API
*
***/

//===========================================================================
void Dim::iAppPerfInitialize() {
    auto ver = appVersion();
    if (ver.major) 
        uperf("app.version (major)") = ver.major;
    if (ver.minor) 
        uperf("app.version (minor)") = ver.minor;
    if (ver.patch) 
        uperf("app.version (patch)") = ver.patch;
    if (ver.build) 
        uperf("app.version (build)") = ver.build;

    s_buildDate = envExecBuildTime();
}
