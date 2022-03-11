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

static auto & s_perfVer = fperf("app.version");
static auto & s_perfPatch = uperf("app.version (patch)");
static auto & s_perfBuild = uperf("app.version (build)");
static auto & s_perfBuildAge = fperf("app.build age", getBuildAge);


/****************************************************************************
*
*   Public API
*
***/

//===========================================================================
void Dim::iAppPerfInitialize() {
    auto ver = envExecVersion();
    auto fver = ver.major + ver.minor / pow(10, digits10(ver.minor));
    s_perfVer = (float) fver;
    s_perfPatch = ver.patch;
    s_perfBuild = ver.build;

    s_buildDate = envExecBuildTime();
}
