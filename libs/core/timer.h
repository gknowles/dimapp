// Copyright Glen Knowles 2015 - 2019.
// Distributed under the Boost Software License, Version 1.0.
//
// timer.h - dim core
#pragma once

#include "cppconf/cppconf.h"

#include "core/types.h"

#include <functional>
#include <memory>
#include <utility> // std::move

namespace Dim {


/****************************************************************************
*
*   Constants
*
***/

constexpr auto kTimerInfinite = Duration::max();


/****************************************************************************
*
*   Implemented by clients
*
***/

class Timer;

class ITimerNotify {
public:
    virtual ~ITimerNotify();
    virtual Duration onTimer(TimePoint now) = 0;

private:
    friend class Timer;
    std::shared_ptr<Timer> m_timer;
};

class TimerProxy : public ITimerNotify {
public:
    TimerProxy(std::function<Duration(TimePoint)> && fn)
        : m_fn{std::move(fn)} {}

private:
    Duration onTimer(TimePoint now) override { return m_fn(now); }

    std::function<Duration(TimePoint)> m_fn;
};


/****************************************************************************
*
*   Public API
*
***/

// Returns time at which the timer will fire
TimePoint timerUpdate(
    ITimerNotify * notify,
    Duration wait,
    bool onlyIfSooner = false
);

// if the timer event is:
//  - not in progress: the timer is closed
//  - in progress and this call is from inside the handler: it is marked for
//      closure and closed when the handler returns.
//  - in progress: waits until the handler returns and then closes the timer.
void timerCloseWait(ITimerNotify * notify);


} // namespace
