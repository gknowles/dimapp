// Copyright Glen Knowles 2017 - 2023.
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
    if (GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof pmc))
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
    if (s_lastSnapshot + 1s > now)
        return;
    if (!GetProcessTimes(
        GetCurrentProcess(),
        &creation,
        &exit,
        &kernel,
        &user
    )) {
        return;
    }

    auto ktime = duration(kernel);
    auto utime = duration(user);
    auto elapsed = now - s_lastSnapshot;
    [[maybe_unused]] auto cores = envProcessors();
    if (empty(s_lastSnapshot) || elapsed > 1s) {
        if (!empty(s_lastSnapshot)) {
            auto factor = 100.0f / elapsed.count(); // / cores;
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
static float getAllTime() {
    updateProcessTimes();
    return s_userPct + s_kernelPct;
}

static TimePoint s_procStartTime;

//===========================================================================
static float getUptime() {
    auto dur = timeNow() - s_procStartTime;
    auto secs = (float) duration_cast<chrono::duration<double>>(dur).count();
    return secs;
}

static auto & s_perfWorkMem = fperf(
    "proc.memory (working)",
    getWorkMem,
    PerfFormat::kSiUnits
);
static auto & s_perfPrivateMem = fperf(
    "proc.memory (private)",
    getPrivateMem,
    PerfFormat::kSiUnits
);
static auto & s_perfAllTime = fperf("proc.cputime (all)", getAllTime);
static auto & s_perfKernelTime = fperf("proc.cputime (kernel)", getKernelTime);
static auto & s_perfUptime = fperf(
    "proc.uptime",
    getUptime,
    PerfFormat::kDuration
);

//===========================================================================
static void initPerfs() {
    s_procStartTime = envProcessStartTime();
}


/****************************************************************************
*
*   Internal API
*
***/

//===========================================================================
void Dim::iPlatformInitialize(PlatformInit phase) {
    switch (phase) {
    case PlatformInit::kBeforeAll:
        winConsoleInitialize();
        break;
    case PlatformInit::kBeforeAppVars:
        winErrorInitialize();
        winEnvInitialize();
        initPerfs();
        winDebugInitialize(phase);
        winCrashInitialize(phase);
        winIocpInitialize();
        winExecInitialize();
        winServiceInitialize();
        winGuiInitialize(phase);
        break;
    case PlatformInit::kAfterAppVars:
        winDebugInitialize(phase);
        winCrashInitialize(phase);
        winGuiInitialize(phase);
        break;
    }
}

//===========================================================================
void Dim::iPlatformDestroy()
{}
