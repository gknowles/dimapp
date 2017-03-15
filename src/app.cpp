// app.cpp - dim core
#include "pch.h"
#pragma hdrstop

using namespace std;
using namespace std::chrono;
using namespace Dim;


/****************************************************************************
*
*   Declarations
*
***/

namespace {

struct CleanupInfo {
    IAppShutdownNotify * notify;
    bool stopped = false;

    CleanupInfo(IAppShutdownNotify * notify)
        : notify(notify) {}
};

enum ETimerMode {
    MAIN_RUN,
    MAIN_STOP,
    CLIENT_STOP,
    SERVER_STOP,
    CONSOLE_STOP,
    DONE
};

class MainTimer : public ITimerNotify {
public:
    typedef bool (IAppShutdownNotify::*StopFn)(bool retry);

public:
    bool stopped() const;
    bool stopFailed(Duration grace);
    void resetShutdownTimer();

    // ITimerNotify
    Duration onTimer(TimePoint now) override;

private:
    bool stop(StopFn notify, bool retry);

    ETimerMode m_mode { MAIN_RUN };
    TimePoint m_shutdownStart;
    bool m_retry{false};
    bool m_notifyFinished{false};
};

} // namespace


/****************************************************************************
*
*   Variables
*
***/

static MainTimer s_mainTimer;
static int s_exitcode;

// cleaners are in the order (newest to oldest) that they will be executed.
static vector<CleanupInfo> s_cleaners;

static Duration s_shutdownTimeout{2min};
static mutex s_runMut;
static condition_variable s_stopped;
static RunMode s_runMode{kRunStopped};
static IAppNotify * s_app;


/****************************************************************************
*
*   MainTimer
*
***/

//===========================================================================
Duration MainTimer::onTimer(TimePoint now) {
    bool next = true;
    switch (m_mode) {
    case MAIN_RUN:
        s_runMode = kRunRunning;
        s_app->onAppRun();
        m_mode = MAIN_STOP;
        return kTimerInfinite;
    case MAIN_STOP:
        s_runMode = kRunStopping;
        m_shutdownStart = now;
        break;
    case CLIENT_STOP:
        next = stop(&IAppShutdownNotify::onAppStopClient, m_retry);
        break;
    case SERVER_STOP:
        next = stop(&IAppShutdownNotify::onAppStopServer, m_retry);
        break;
    case CONSOLE_STOP:
        next = stop(&IAppShutdownNotify::onAppStopConsole, m_retry);
        break;
    case DONE:
        s_cleaners.clear();
        s_stopped.notify_one();
        return kTimerInfinite;
    }

    // some delay when rerunning the same step (i.e. QueryDestroy failed)
    if (!next) {
        m_retry = true;
        return 10ms;
    }

    m_mode = ETimerMode(m_mode + 1);
    m_retry = false;
    return 0ms;
}

//===========================================================================
bool MainTimer::stopped() const {
    return m_mode == DONE;
}

//===========================================================================
bool MainTimer::stopFailed(Duration grace) {
    m_notifyFinished = false;
    if (Clock::now() - m_shutdownStart > s_shutdownTimeout + grace) {
        assert(0 && "shutdown timeout");
        terminate();
    }
    return false;
}

//===========================================================================
void MainTimer::resetShutdownTimer() {
    m_shutdownStart = Clock::now();
}

//===========================================================================
bool MainTimer::stop(StopFn notify, bool retry) {
    if (!retry) {
        for (auto && v : s_cleaners) 
            v.stopped = false;
    }
    bool stopped = true;
    for (auto && v : s_cleaners) {
        if (!v.stopped) {
            m_notifyFinished = true;
            if ((v.notify->*notify)(retry)) {
                // successful handlers better not have called appStopFailed()
                assert(m_notifyFinished);
                v.stopped = true;
            } else {
                // failed handlers MUST have used appStopFailed()
                assert(!m_notifyFinished);
                stopped = false;
                if (retry)
                    return stopFailed(5s);
            }
        }
    }
    return stopped;
}


/****************************************************************************
*
*   Public API
*
***/

//===========================================================================
RunMode Dim::appMode() {
    return s_runMode;
}

//===========================================================================
int Dim::appRun(IAppNotify & app, int argc, char * argv[]) {
    s_runMode = kRunStarting;
    iConsoleInitialize();
    iTaskInitialize();
    iTimerInitialize();
    iSystemInitialize();
    iFileInitialize();
    iSocketInitialize();
    iAppSocketInitialize();
    iHttpRouteInitialize();

    s_app = &app;
    s_app->m_argc = argc;
    s_app->m_argv = argv;
    s_mainTimer = {};
    timerUpdate(&s_mainTimer, 0ms);

    unique_lock<mutex> lk{s_runMut};
    while (!s_mainTimer.stopped())
        s_stopped.wait(lk);
    iTimerDestroy();
    iTaskDestroy();
    s_runMode = kRunStopped;
    return s_exitcode;
}

//===========================================================================
void Dim::appSignalShutdown(int exitcode) {
    if (exitcode > s_exitcode)
        s_exitcode = exitcode;
    timerUpdate(&s_mainTimer, 0ms);
}

//===========================================================================
void Dim::appSignalUsageError(string_view err, string_view detail) {
    if (!err.empty())
        return appSignalUsageError(EX_USAGE, err, detail);
    Cli cli;
    appSignalUsageError(cli.exitCode(), err, detail);
}

//===========================================================================
void Dim::appSignalUsageError(int code, string_view err, string_view detail) {
    if (code) {
        Cli cli;
        auto em = err.empty() ? cli.errMsg() : err;
        auto dm = detail.empty() ? cli.errDetail() : detail;
        if (!em.empty())
            logMsgError() << "Error: " << em;
        if (!dm.empty())
            logMsgInfo() << dm;
        auto os = logMsgInfo();
        cli.printUsageEx(os);
    }
    return appSignalShutdown(code);
}

//===========================================================================
void Dim::appMonitorShutdown(IAppShutdownNotify * cleaner) {
    s_cleaners.emplace(s_cleaners.begin(), cleaner);
}

//===========================================================================
bool Dim::appStopFailed() {
    return s_mainTimer.stopFailed(0ms);
}

//===========================================================================
void Dim::appDelayShutdown() {
    s_mainTimer.resetShutdownTimer();
}
