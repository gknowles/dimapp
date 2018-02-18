// Copyright Glen Knowles 2017 - 2018.
// Distributed under the Boost Software License, Version 1.0.
//
// winplatform.cpp - dim windows platform
#include "pch.h"
#pragma hdrstop

using namespace std;
using namespace Dim;


/****************************************************************************
*
*   Performance counters
*
***/

static PROCESS_MEMORY_COUNTERS s_procMem;

//===========================================================================
static void updateMemoryCounters() {
    PROCESS_MEMORY_COUNTERS pmc;
    if (GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc)))
        s_procMem = pmc;
}

//===========================================================================
static float getWorkMem() {
    updateMemoryCounters();
    return (float) s_procMem.WorkingSetSize;
}

//===========================================================================
static float getPrivateMem() {
    return (float) s_procMem.PagefileUsage;
}

static Duration s_kernelTime;
static Duration s_userTime;
static TimePoint s_lastSnapshot;
static float s_kernelPct;
static float s_userPct;

//===========================================================================
static void updateProcessTimes() {
    FILETIME creation;
    FILETIME exit;
    FILETIME kernel;
    FILETIME user;
    auto now = Clock::now();
    if (!GetProcessTimes(GetCurrentProcess(), &creation, &exit, &kernel, &user))
        return;

    ULARGE_INTEGER tmp;
    tmp.HighPart = kernel.dwHighDateTime;
    tmp.LowPart = kernel.dwLowDateTime;
    auto ktime = Duration(tmp.QuadPart);
    tmp.HighPart = user.dwHighDateTime;
    tmp.LowPart = user.dwLowDateTime;
    auto utime = Duration(tmp.QuadPart);

    auto elapsed = now - s_lastSnapshot;
    auto cores = envCpus();
    if (!s_lastSnapshot || elapsed > 1s) {
        if (s_lastSnapshot) {
            auto factor = 100.0f / elapsed.count() / cores;
            s_kernelPct = factor * (float) (ktime - s_kernelTime).count();
            s_userPct = factor * (float) (utime - s_userTime).count();
        }
        s_lastSnapshot = now;
        s_kernelTime = ktime;
        s_userTime = utime;
    }
}

//===========================================================================
static float getKernelTime() {
    updateProcessTimes();
    return s_kernelPct;
}

//===========================================================================
static float getUserTime() {
    return s_userPct;
}

static auto & s_perfWorkMem = fperf("proc memory (working)", getWorkMem);
static auto & s_perfPrivateMem = fperf("proc memory (private)", getPrivateMem);
static auto & s_perfKernelTime = fperf("proc time (kernel)", getKernelTime);
static auto & s_perfUserTime = fperf("proc time (user)", getUserTime);


/****************************************************************************
*
*   Web console
*
***/

//===========================================================================
// User info
//===========================================================================
namespace {
class WebAccount : public IHttpRouteNotify {
    void onHttpRequest(unsigned reqId, HttpRequest & msg) override;
};
} // namespace

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
};
const TokenTable s_sidTypeTbl(s_sidTypes);

//===========================================================================
static void addSidRow(JBuilder & out, SID_AND_ATTRIBUTES & sa) {
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
            logMsgCrash() << "LookupAccountSid(NULL): " << err;
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
        logMsgCrash() << "LookupAccountSid: " << WinError{};
    }
    name.resize(nameLen);
    dom.resize(domLen);

    out.object();
    out.member("attrs");
    out.array();
    unsigned found = {};
    for (auto && attr : s_attrTbl) {
        if (attr.id & sa.Attributes) {
            found |= attr.id;
            out.value(attr.name);
        }
    }
    if (auto unknown = ~found & sa.Attributes) {
        auto unk = "UNKNOWN("s;
        unk += StrFrom<unsigned>(unknown);
        unk += ')';
        out.value(unk);
    }
    out.end();
    out.member("name", name);
    out.member("domain", dom);
    if (auto name = tokenTableGetName(s_sidTypeTbl, use)) {
        out.member("type", name);
    } else {
        auto unk = "UNKNOWN("s;
        unk += StrFrom<underlying_type_t<decltype(use)>>(use);
        unk += ')';
        out.member("type", unk);
    }
    out.end();
}

//===========================================================================
void WebAccount::onHttpRequest(unsigned reqId, HttpRequest & msg) {
    auto proc = GetCurrentProcess();
    HANDLE token;
    if (!OpenProcessToken(proc, TOKEN_QUERY, &token))
        logMsgCrash() << "OpenProcessToken: " << WinError{};
    DWORD len;
    BOOL result = GetTokenInformation(token, TokenUser, NULL, 0, &len);
    WinError err;
    if (result || err != ERROR_INSUFFICIENT_BUFFER) {
        logMsgCrash() << "GetTokenInformation(TokenUser, NULL): "
            << WinError{};
    }
    auto usr = unique_ptr<TOKEN_USER>((TOKEN_USER *) malloc(len));
    if (!GetTokenInformation(token, TokenUser, usr.get(), len, &len))
        logMsgCrash() << "GetTokenInformation(TokenUser): " << WinError{};
    result = GetTokenInformation(token, TokenGroups, NULL, 0, &len);
    err.set();
    if (result || err != ERROR_INSUFFICIENT_BUFFER) {
        logMsgCrash() << "GetTokenInformation(TokenGroups, NULL): "
            << WinError{};
    }
    auto grps = unique_ptr<TOKEN_GROUPS>((TOKEN_GROUPS *) malloc(len));
    if (!GetTokenInformation(token, TokenGroups, grps.get(), len, &len))
        logMsgCrash() << "GetTokenInformation(TokenGroups): " << WinError{};
    CloseHandle(token);

    HttpResponse res;
    JBuilder bld(res.body());
    bld.array();
    addSidRow(bld, usr->User);
    auto * ptr = grps->Groups,
        *eptr = ptr + grps->GroupCount;
    for (; ptr != eptr; ++ptr) {
        addSidRow(bld, *ptr);
    }
    bld.end();
    res.addHeader(kHttpContentType, "application/json");
    res.addHeader(kHttp_Status, "200");
    httpRouteReply(reqId, res);
}

//===========================================================================
// Register Admin UI
//===========================================================================
static WebAccount s_account;

//===========================================================================
static void registerWebAdmin() {
    httpRouteAdd(&s_account, "/srv/account.json");
}


/****************************************************************************
*
*   Internal API
*
***/

//===========================================================================
void Dim::iPlatformInitialize() {
    winErrorInitialize();
    winEnvInitialize();
    winIocpInitialize();
    winServiceInitialize();
    winGuiInitialize();
}

//===========================================================================
void Dim::iPlatformConfigInitialize() {
    winGuiConfigInitialize();

    if (appFlags() & fAppWithWebAdmin)
        registerWebAdmin();
}
