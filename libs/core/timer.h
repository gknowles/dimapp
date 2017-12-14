// Copyright Glen Knowles 2015 - 2017.
// Distributed under the Boost Software License, Version 1.0.
//
// timer.h - dim core
#pragma once

#include "cppconf/cppconf.h"

#include "core/types.h"

#include <functional>
#include <memory>

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

TimePoint timerUpdate(
    ITimerNotify * notify,
    Duration wait,
    bool onlyIfSooner = false
);
void timerCloseWait(ITimerNotify * notify);


} // namespace
