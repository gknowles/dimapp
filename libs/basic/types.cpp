// Copyright Glen Knowles 2018 - 2021.
// Distributed under the Boost Software License, Version 1.0.
//
// types.cpp - dim basic
#include "pch.h"
#pragma hdrstop

using namespace std;
using namespace Dim;


/****************************************************************************
*
*   Run modes
*
***/

static TokenTable::Token s_runModes[] = {
    { kRunStopped,  "stopped" },
    { kRunStarting, "starting" },
    { kRunRunning,  "running" },
    { kRunStopping, "stopping" },
};
static TokenTable s_runModeTbl{s_runModes};

//===========================================================================
const char * Dim::toString(RunMode mode, const char def[]) {
    return s_runModeTbl.findName(mode, def);
}

//===========================================================================
RunMode Dim::fromString(string_view src, RunMode def) {
    return s_runModeTbl.find(src, def);
}


/****************************************************************************
*
*   VersionInfo
*
***/

//===========================================================================
string Dim::toString(const VersionInfo & ver) {
    ostringstream os;
    os << ver.major;
    if (ver.build) {
        os << '.' << ver.minor << '.' << ver.patch << '.' << ver.build;
    } else if (ver.patch) {
        os << '.' << ver.minor << '.' << ver.patch;
    } else if (ver.minor) {
        os << '.' << ver.minor;
    }
    return os.str();
}
