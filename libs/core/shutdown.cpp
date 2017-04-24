// Copyright Glen Knowles 2015 - 2017.
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
    bool stopped = false;

    CleanupInfo(IShutdownNotify * notify)
        : notify(notify) {}
};

enum ETimerMode {
    MAIN_STOP,
    CLIENT_STOP,
    SERVER_STOP,
    CONSOLE_STOP,
    FINALIZE,
    DONE
};

class MainTimer : public ITimerNotify {
public:
    typedef void (IShutdownNotify::*StopFn)(bool firstTry);

public:
    bool stopped() const;
    void incomplete(Duration grace);
    void resetTimer();

    // ITimerNotify
    Duration onTimer(TimePoint now) override;

private:
    bool stop(StopFn notify, bool retry);

    ETimerMode m_mode { MAIN_STOP };
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


/****************************************************************************
*
*   MainTimer
*
***/

//===========================================================================
Duration MainTimer::onTimer(TimePoint now) {
    bool next = true;
    switch (m_mode) {
    case MAIN_STOP:
        m_shutdownStart = now;
        break;
    case CLIENT_STOP:
        next = stop(&IShutdownNotify::onShutdownClient, m_firstTry);
        break;
    case SERVER_STOP:
        next = stop(&IShutdownNotify::onShutdownServer, m_firstTry);
        break;
    case CONSOLE_STOP:
        next = stop(&IShutdownNotify::onShutdownConsole, m_firstTry);
        break;
    case FINALIZE:
        s_cleaners.clear();
        m_mode = DONE;
        s_stopCv.notify_one();
        return kTimerInfinite;
    }

    // some delay when rerunning the same step (i.e. QueryDestroy failed)
    if (!next) {
        m_firstTry = false;
        return 10ms;
    }

    m_mode = ETimerMode(m_mode + 1);
    m_firstTry = true;
    return 0ms;
}

//===========================================================================
bool MainTimer::stopped() const {
    return m_mode == DONE;
}

//===========================================================================
void MainTimer::incomplete(Duration grace) {
    m_incomplete = true;
    if (Clock::now() - m_shutdownStart > s_shutdownTimeout + grace) {
        assert(0 && "shutdown timeout");
        terminate();
    }
}

//===========================================================================
void MainTimer::resetTimer() {
    m_shutdownStart = Clock::now();
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

    unique_lock<mutex> lk{s_stopMut};
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
    s_mainTimer.incomplete(0ms);
}

//===========================================================================
void Dim::shutdownDelay() {
    s_mainTimer.resetTimer();
}
