// Copyright Glen Knowles 2015 - 2019.
// Distributed under the Boost Software License, Version 1.0.
//
// shutdown.cpp - dim core
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
    IShutdownNotify * notify;
    bool stopped{false};

    explicit CleanupInfo(IShutdownNotify * notify)
        : notify(notify) {}
};

class MainTimer : public ITimerNotify {
public:
    typedef void (IShutdownNotify::*StopFn)(bool firstTry);
    enum TimerMode {
        kMainStop,
        kClientStop,
        kServerStop,
        kConsoleStop,
        kFinalize,
        kDone,
    };

public:
    bool stopped() const;
    void incomplete();
    void resetTimer();

    // ITimerNotify
    Duration onTimer(TimePoint now) override;

private:
    bool stop(StopFn notify, bool retry);

    TimerMode m_mode{kMainStop};
    TimePoint m_shutdownStart;
    bool m_firstTry{true};
    bool m_incomplete{false};
};

} // namespace


/****************************************************************************
*
*   Variables
*
***/

static MainTimer s_mainTimer;

// cleaners are in the order (newest to oldest) that they will be executed.
static vector<CleanupInfo> s_cleaners;

static Duration s_shutdownTimeout{2min};
static mutex s_stopMut;
static condition_variable s_stopCv;
static bool s_disableTimeout{false};


/****************************************************************************
*
*   MainTimer
*
***/

//===========================================================================
Duration MainTimer::onTimer(TimePoint now) {
    bool next = true;
    switch (m_mode) {
    case kMainStop:
        m_shutdownStart = now;
        break;
    case kClientStop:
        next = stop(&IShutdownNotify::onShutdownClient, m_firstTry);
        break;
    case kServerStop:
        next = stop(&IShutdownNotify::onShutdownServer, m_firstTry);
        break;
    case kConsoleStop:
        next = stop(&IShutdownNotify::onShutdownConsole, m_firstTry);
        break;
    case kFinalize:
        s_cleaners.clear();
        m_mode = kDone;
        s_stopCv.notify_one();
        return kTimerInfinite;
    case kDone:
        break;
    }

    // some delay when rerunning the same step
    if (!next) {
        m_firstTry = false;
        return 10ms;
    }

    m_mode = TimerMode(m_mode + 1);
    m_firstTry = true;
    return 0ms;
}

//===========================================================================
bool MainTimer::stopped() const {
    return m_mode == kDone;
}

//===========================================================================
void MainTimer::incomplete() {
    m_incomplete = true;
    if (!s_disableTimeout
        && timeNow() - m_shutdownStart > s_shutdownTimeout
    ) {
        assert(!"shutdown timeout");
        terminate();
    }
}

//===========================================================================
void MainTimer::resetTimer() {
    m_shutdownStart = timeNow();
}

//===========================================================================
bool MainTimer::stop(StopFn notify, bool firstTry) {
    if (firstTry) {
        for (auto && v : s_cleaners)
            v.stopped = false;
    }
    bool stopped = true;
    for (auto && v : s_cleaners) {
        if (!v.stopped) {
            m_incomplete = false;
            (v.notify->*notify)(firstTry);
            if (m_incomplete) {
                // shutdownIncomplete() was called, handler will have to be
                // retried later.
                stopped = false;
                if (!firstTry)
                    return false;
            } else {
                v.stopped = true;
            }
        }
    }
    return stopped;
}


/****************************************************************************
*
*   Internal API
*
***/

//===========================================================================
void Dim::iShutdownDestroy() {
    s_mainTimer = {};
    timerUpdate(&s_mainTimer, 0ms);

    unique_lock lk{s_stopMut};
    while (!s_mainTimer.stopped())
        s_stopCv.wait(lk);
}


/****************************************************************************
*
*   Public API
*
***/

//===========================================================================
void Dim::shutdownMonitor(IShutdownNotify * cleaner) {
    s_cleaners.emplace(s_cleaners.begin(), cleaner);
}

//===========================================================================
void Dim::shutdownIncomplete() {
    s_mainTimer.incomplete();
}

//===========================================================================
void Dim::shutdownDelay() {
    s_mainTimer.resetTimer();
}

//===========================================================================
void Dim::shutdownDisableTimeout(bool disable) {
    s_disableTimeout = disable;
}
