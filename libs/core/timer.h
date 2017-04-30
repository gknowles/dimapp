// Copyright Glen Knowles 2015 - 2017.
// Distributed under the Boost Software License, Version 1.0.
//
// timer.h - dim core
#pragma once

#include "cppconf/cppconf.h"

#include "core/types.h"

#include <memory>

namespace Dim {


/****************************************************************************
*
*   Constants
*
***/

const auto kTimerInfinite = Duration::max();


/****************************************************************************
*
*   Implemented by clients
*
***/

class ITimerNotify {
public:
    virtual ~ITimerNotify();
    virtual Duration onTimer(TimePoint now) = 0;

private:
    friend class Timer;
    std::shared_ptr<Timer> m_timer;
};


/****************************************************************************
*
*   Public Timer API
*
***/

TimePoint timerUpdate(
    ITimerNotify * notify,
    Duration wait,
    bool onlyIfSooner = false
);
void timerCloseWait(ITimerNotify * notify);


/****************************************************************************
*
*   TimerList
*
***/

const Duration kTimerDefaultMinWait = (std::chrono::seconds) 2;

class ITimerListNotify 
    : public ListBaseLink<ITimerListNotify> {
public:
    virtual ~ITimerListNotify() {}
    virtual void onTimer(TimePoint now) = 0;
private:
    friend class TimerList;
    TimePoint m_lastTouched;
};

class TimerList : ITimerNotify {
public:
    TimerList(Duration timeout, Duration minWait = kTimerDefaultMinWait);

    void setTimeout(
        Duration timeout, 
        Duration minWait = kTimerDefaultMinWait
    );

    void touch(ITimerListNotify * notify);
    void unlink(ITimerListNotify * notify);

private:
    Duration onTimer(TimePoint now) override;

    List<ITimerListNotify> m_nodes;
    Duration m_timeout;
    Duration m_minWait;
};

} // namespace
