// Copyright Glen Knowles 2018 - 2019.
// Distributed under the Boost Software License, Version 1.0.
//
// types.cpp - dim core
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
    return tokenTableGetName(s_runModeTbl, mode, def);
}

//===========================================================================
RunMode Dim::fromString(string_view src, RunMode def) {
    return tokenTableGetEnum(s_runModeTbl, src, def);
}
