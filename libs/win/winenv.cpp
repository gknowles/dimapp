// Copyright Glen Knowles 2017 - 2018.
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
static string s_execPath;


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
const string & Dim::envExecPath() {
    if (s_execPath.empty()) {
        DWORD num = 0;
        while (num == s_execPath.size()) {
            s_execPath.resize(s_execPath.size() + MAX_PATH);
            num = GetModuleFileName(
                NULL,
                s_execPath.data(),
                (DWORD) s_execPath.size()
            );
        }
        s_execPath.resize(num);
    }
    return s_execPath;
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
