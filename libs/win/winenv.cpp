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

//===========================================================================
ProcessRights Dim::envProcessRights() {
    char sid[SECURITY_MAX_SID_SIZE];
    DWORD cb = sizeof(sid);;
    if (!CreateWellKnownSid(WinBuiltinAdministratorsSid, NULL, sid, &cb)) {
        logMsgError() << "CreateWellKnownSid(Administrators): " << WinError{};
        return kEnvUserStandard;
    }
    BOOL isMember;
    if (!CheckTokenMembership(NULL, sid, &isMember)) {
        logMsgError() << "CheckTokenMembership(NULL): " << WinError{};
        return kEnvUserStandard;
    }
    if (isMember)
        return kEnvUserAdmin;


    auto proc = GetCurrentProcess();
    HANDLE token;
    if (!OpenProcessToken(proc, TOKEN_QUERY, &token))
        return kEnvUserStandard;

    TOKEN_LINKED_TOKEN lt;
    if (!GetTokenInformation(
        token,
        TokenLinkedToken,
        &lt,
        sizeof(lt),
        &cb
    )) {
        WinError err;
        if (err == ERROR_NO_SUCH_LOGON_SESSION
            || err == ERROR_PRIVILEGE_NOT_HELD
        ) {
            return kEnvUserStandard;
        }

        logMsgError() << "GetTokenInformation(TokenLinkedToken): " << err;
        return kEnvUserStandard;
    }

    if (!CheckTokenMembership(lt.LinkedToken, sid, &isMember)) {
        logMsgError() << "CheckTokenMembership(linked): " << WinError{};
        return kEnvUserStandard;
    }
    if (isMember)
        return kEnvUserRestrictedAdmin;

    return kEnvUserStandard;
}
