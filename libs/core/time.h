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

namespace Dim {


/****************************************************************************
*
*   Clock
*
***/

const int64_t kClockTicksPerSecond = 10'000'000;

struct Clock {
    typedef int64_t rep;
    typedef std::ratio<1, kClockTicksPerSecond> period;
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


/****************************************************************************
*
*   Time8601Str
*
*   Time format as defined by RFC3339, an internet profile of ISO 8601 that
*   covers a subset of time points and no coverage of durations.
*
*   YYYY-MM-DDThh:mm:ss[.nnnnnnn]Z
*   YYYY-MM-DDThh:mm:ss[.nnnnnnn]+hh:mm
*
***/

class Time8601Str {
    char m_data[33];
public:
    Time8601Str();
    Time8601Str(TimePoint time, unsigned precision = 0, int tzMinutes = 0);

    Time8601Str & set(
        TimePoint time, 
        unsigned precision = 0, 
        int tzMinutes = 0
    );

    const char * c_str() const { return m_data; }
    std::string_view view() const;
};


/****************************************************************************
*
*   TimeZone
*
***/

// Returns the local timezone delta from UTC, uses time to determine if
// daylight savings time is in effect.
int timeZoneMinutes(TimePoint time);


} // namespace