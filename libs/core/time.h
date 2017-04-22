// Copyright Glen Knowles 2015 - 2017.
// Distributed under the Boost Software License, Version 1.0.
//
// time.h - dim core
#pragma once

#include "cppconf/cppconf.h"

#include <chrono>
#include <cstdint>
#include <ctime> // time_t
#include <ratio>
#include <type_traits>

// using namespace std::literals;
namespace Dim {


/****************************************************************************
*
*   Clock
*
***/

struct Clock {
    typedef int64_t rep;
    typedef std::ratio_multiply<std::ratio<100, 1>, std::nano> period;
    typedef std::chrono::duration<rep, period> duration;
    typedef std::chrono::time_point<Clock> time_point;
    static const bool is_monotonic = false;
    static const bool is_steady = false;

    static time_point now() noexcept;

    // C conversions
    static time_t to_time_t(const time_point & time) noexcept;
    static time_point from_time_t(time_t tm) noexcept;
};

typedef Clock::duration Duration;
typedef Clock::time_point TimePoint;


} // namespace
