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
unsigned Dim::envProcessId() {
    return GetCurrentProcessId();
}

//===========================================================================
TimePoint Dim::envProcessStartTime() {
    FILETIME creation;
    FILETIME exit;
    FILETIME kernel;
    FILETIME user;
    if (!GetProcessTimes(GetCurrentProcess(), &creation, &exit, &kernel, &user))
        return {};
    ULARGE_INTEGER tmp;
    tmp.HighPart = creation.dwHighDateTime;
    tmp.LowPart = creation.dwLowDateTime;
    TimePoint time{Duration{tmp.QuadPart}};
    return time;
}

//===========================================================================
DiskSpace Dim::envDiskSpace(std::string_view path) {
    ULARGE_INTEGER avail;
    ULARGE_INTEGER total;
    if (!GetDiskFreeSpaceExW(
        toWstring(path).c_str(),
        &avail,
        &total,
        nullptr
    )) {
        logMsgFatal() << "GetDiskFreeSpawnEx(" << path << "): " << WinError{};
    }
    DiskSpace out;
    out.avail = avail.QuadPart;
    out.total = total.QuadPart;
    return out;
}

//===========================================================================
Dim::VersionInfo Dim::envExecVersion() {
    auto wname = toWstring(envExecPath());
    DWORD unused;
    auto len = GetFileVersionInfoSizeW(wname.c_str(), &unused);
    auto buf = make_unique<char[]>(len);
    if (!GetFileVersionInfoW(
        wname.c_str(),
        0,  // ignored
        len,
        buf.get()
    )) {
        logMsgFatal() << "GetFileVersionInfo: " << WinError{};
    }
    VS_FIXEDFILEINFO * fi;
    UINT ulen;
    if (!VerQueryValueW(buf.get(), L"\\", (void **) &fi, &ulen))
        logMsgFatal() << "VerQueryValue: " << WinError{};

    VersionInfo vi = {};
    vi.major = HIWORD(fi->dwProductVersionMS);
    vi.minor = LOWORD(fi->dwProductVersionMS);
    vi.patch = HIWORD(fi->dwProductVersionLS);
    vi.build = LOWORD(fi->dwProductVersionLS);
    return vi;
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


/****************************************************************************
*
*   Accounts
*
***/

const TokenTable::Token s_attrs[] = {
    { (int) SE_GROUP_MANDATORY,           "MANDATORY" },
    { (int) SE_GROUP_ENABLED_BY_DEFAULT,  "ENABLED_BY_DEFAULT" },
    { (int) SE_GROUP_ENABLED,             "ENABLED" },
    { (int) SE_GROUP_OWNER,               "OWNER" },
    { (int) SE_GROUP_USE_FOR_DENY_ONLY,   "USE_FOR_DENY_ONLY" },
    { (int) SE_GROUP_INTEGRITY,           "INTEGRITY" },
    { (int) SE_GROUP_INTEGRITY_ENABLED,   "INTEGRITY_ENABLED" },
    { (int) SE_GROUP_LOGON_ID,            "LOGON_ID" },
    { (int) SE_GROUP_RESOURCE,            "RESOURCE" },
};
const TokenTable s_attrTbl(s_attrs);

const TokenTable::Token s_sidTypes[] = {
    { SidTypeUser,              "User" },
    { SidTypeGroup,             "Group" },
    { SidTypeDomain,            "Domain" },
    { SidTypeAlias,             "Alias" },
    { SidTypeWellKnownGroup,    "WellKnownGroup" },
    { SidTypeDeletedAccount,    "DeletedAccount" },
    { SidTypeInvalid,           "Invalid" },
    { SidTypeUnknown,           "Unknown" },
    { SidTypeComputer,          "Computer" },
    { SidTypeLabel,             "Label" },
    { SidTypeLogonSession,      "LogonSession" },
};
const TokenTable s_sidTypeTbl(s_sidTypes);

//===========================================================================
static void addSidRow(IJBuilder * out, SID_AND_ATTRIBUTES & sa) {
    DWORD nameLen = 0;
    DWORD domLen = 0;
    SID_NAME_USE use;
    if (!LookupAccountSid(
        NULL,
        sa.Sid,
        NULL, &nameLen,
        NULL, &domLen,
        &use
    )) {
        WinError err;
        if (err != ERROR_INSUFFICIENT_BUFFER)
            logMsgFatal() << "LookupAccountSid(NULL): " << err;
    }
    nameLen += 1;
    domLen += 1;
    string name(nameLen, 0);
    string dom(domLen, 0);
    if (!LookupAccountSid(
        NULL,
        sa.Sid,
        name.data(), &nameLen,
        dom.data(), &domLen,
        &use
    )) {
        logMsgFatal() << "LookupAccountSid: " << WinError{};
    }
    name.resize(nameLen);
    dom.resize(domLen);

    out->object();
    out->member("attrs");
    out->array();
    unsigned found = {};
    for (auto && attr : s_attrTbl) {
        if (attr.id & sa.Attributes) {
            found |= attr.id;
            out->value(attr.name);
        }
    }
    if (auto unknown = ~found & sa.Attributes) {
        auto unk = "UNKNOWN("s;
        unk += StrFrom<unsigned>(unknown);
        unk += ')';
        out->value(unk);
    }
    out->end();
    out->member("name", name);
    out->member("domain", dom);
    if (auto name = tokenTableGetName(s_sidTypeTbl, use)) {
        out->member("type", name);
    } else {
        auto unk = "UNKNOWN("s;
        unk += StrFrom<underlying_type_t<decltype(use)>>(use);
        unk += ')';
        out->member("type", unk);
    }
    out->end();
}

//===========================================================================
void Dim::envProcessAccount(IJBuilder * out) {
    auto proc = GetCurrentProcess();
    HANDLE token;
    if (!OpenProcessToken(proc, TOKEN_QUERY, &token))
        logMsgFatal() << "OpenProcessToken: " << WinError{};
    DWORD len;
    BOOL result = GetTokenInformation(token, TokenUser, NULL, 0, &len);
    WinError err;
    if (result || err != ERROR_INSUFFICIENT_BUFFER) {
        logMsgFatal() << "GetTokenInformation(TokenUser, NULL): "
            << WinError{};
    }
    auto usr = unique_ptr<TOKEN_USER>((TOKEN_USER *) malloc(len));
    if (!GetTokenInformation(token, TokenUser, usr.get(), len, &len))
        logMsgFatal() << "GetTokenInformation(TokenUser): " << WinError{};
    result = GetTokenInformation(token, TokenGroups, NULL, 0, &len);
    err.set();
    if (result || err != ERROR_INSUFFICIENT_BUFFER) {
        logMsgFatal() << "GetTokenInformation(TokenGroups, NULL): "
            << WinError{};
    }
    auto grps = unique_ptr<TOKEN_GROUPS>((TOKEN_GROUPS *) malloc(len));
    if (!GetTokenInformation(token, TokenGroups, grps.get(), len, &len))
        logMsgFatal() << "GetTokenInformation(TokenGroups): " << WinError{};
    CloseHandle(token);

    out->array();
    addSidRow(out, usr->User);
    auto * ptr = grps->Groups,
        *eptr = ptr + grps->GroupCount;
    for (; ptr != eptr; ++ptr) {
        addSidRow(out, *ptr);
    }
    out->end();
}
