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
#include <string_view>

namespace Dim {


/****************************************************************************
*
*   Clock
*
***/

constexpr int64_t kClockTicksPerSecond = 10'000'000;

struct Clock {
    using rep = int64_t;
    using period = std::ratio<1, kClockTicksPerSecond>;
    using duration = std::chrono::duration<rep, period>;
    class time_point : public std::chrono::time_point<Clock> {
    public:
        using std::chrono::time_point<Clock>::time_point;
        explicit operator bool() const noexcept;
    };

    static const bool is_monotonic = false;
    static const bool is_steady = false;

    static time_point now() noexcept;

    // C conversions
    static time_t to_time_t(const time_point & time) noexcept;
    static time_point from_time_t(time_t tm) noexcept;
};

//===========================================================================
inline Clock::time_point::operator bool() const noexcept {
    return *this != time_point{};
}

using Duration = Clock::duration;
using TimePoint = Clock::time_point;


/****************************************************************************
*
*   Duration to/from string
*
***/

enum class DurationFormat {
    kTwoPart,   // two units (s + ms, m + s, h + m, etc), no decimal
    kOnePart,   // one unit, three decimals
};
std::string toString(Duration val, DurationFormat fmt);
[[nodiscard]] bool parse(Dim::Duration * out, std::string_view src);


/****************************************************************************
*
*   Time8601Str
*
*   Time format as defined by RFC3339, an Internet profile of ISO 8601 that
*   covers a subset of time points and no coverage of durations.
*
*   YYYY-MM-DDThh:mm:ss[.nnnnnnn]Z
*   YYYY-MM-DDThh:mm:ss[.nnnnnnn]{+,-}hh:mm
*
***/

class Time8601Str {
    char m_data[33];
public:
    Time8601Str();
    Time8601Str(TimePoint time, unsigned precision = 0, int tzMinutes = 0);

    // asserts if precision > 7
    Time8601Str & set(
        TimePoint time,
        unsigned precision = 0,
        int tzMinutes = 0
    );

    const char * c_str() const { return m_data; }
    std::string_view view() const;
};

// In addition to the forms above, also accepts:
//  YYYY-MM-DD
//  YYYY-MM-DD{t, }hh:mm:ss[.nnnnnnn]{Z,z}
//  YYYY-MM-DD{t, }hh:mm:ss[.nnnnnnn]{+,-}hh:mm
bool timeParse8601(TimePoint * out, std::string_view str);


/****************************************************************************
*
*   TimeZone
*
***/

// Returns the local timezone delta from UTC, uses time to determine if
// daylight savings time is in effect.
int timeZoneMinutes(TimePoint time);


/****************************************************************************
*
*   Time conversions
*
***/

// Day of year (tm_yday) and daylight savings time flag (tm_isdst) are not
// supported and set to -1.
bool timeToDesc(tm * tm, TimePoint time);

bool timeFromDesc(TimePoint * time, const tm & tm);


} // namespace
