// Copyright Glen Knowles 2017.
// Distributed under the Boost Software License, Version 1.0.
//
// winenv.cpp - dim windows platform
#include "pch.h"
#pragma hdrstop

using namespace std;
using namespace Dim;


/****************************************************************************
*
*   Variables
*
***/

static EnvMemoryConfig s_memCfg;
static unsigned s_numCpus;


/****************************************************************************
*
*   Private API
*
***/

//===========================================================================
void Dim::winEnvInitialize() {
    SYSTEM_INFO info;
    GetSystemInfo(&info);
    s_memCfg = {};
    s_memCfg.pageSize = info.dwPageSize;
    s_memCfg.allocAlign = info.dwAllocationGranularity;
    if (winEnablePrivilege(SE_LOCK_MEMORY_NAME)) {
        // We have the right to allocate large pages, now get how big they are
        // and see if they are supported at all (non-zero).
        s_memCfg.minLargeAlloc = GetLargePageMinimum();
    }
    s_numCpus = info.dwNumberOfProcessors;
}


/****************************************************************************
*
*   Public API
*
***/

//===========================================================================
string Dim::envExecPath() {
    string out;
    DWORD num = 0;
    while (num == out.size()) {
        out.resize(out.size() + MAX_PATH);
        num = GetModuleFileName(NULL, out.data(), (DWORD) out.size());
    }
    out.resize(num);
    return out;
}

//===========================================================================
const EnvMemoryConfig & Dim::envMemoryConfig() {
    assert(s_numCpus);
    return s_memCfg;
}

//===========================================================================
unsigned Dim::envCpus() {
    assert(s_numCpus);
    return s_numCpus;
}
