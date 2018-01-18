// Copyright Glen Knowles 2018.
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
static TokenTable s_runModeTbl{s_runModes, size(s_runModes)};

//===========================================================================
const char * toString(RunMode mode, const char def[]) {
    return tokenTableGetName(s_runModeTbl, type, def);
}

//===========================================================================
RunMode fromString(std::string_view src, RunMode def) {
    return tokenTableGetEnum(s_runModeTbl, src, def);
}
