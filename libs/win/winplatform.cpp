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
    auto now = timeNow();
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

static auto & s_perfWorkMem = fperf("proc.memory (working)", getWorkMem);
static auto & s_perfPrivateMem = fperf("proc.memory (private)", getPrivateMem);
static auto & s_perfKernelTime = fperf("proc.time (kernel)", getKernelTime);
static auto & s_perfUserTime = fperf("proc.time (user)", getUserTime);


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

//===========================================================================
void WebAccount::onHttpRequest(unsigned reqId, HttpRequest & msg) {
    HttpResponse res;
    JBuilder bld(&res.body());
    envProcessAccount(&bld);
    res.addHeader(kHttpContentType, "application/json");
    res.addHeader(kHttp_Status, "200");
    httpRouteReply(reqId, move(res));
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
    winCrashInitialize();
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
