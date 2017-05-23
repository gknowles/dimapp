// Copyright Glen Knowles 2015 - 2017.
// Distributed under the Boost Software License, Version 1.0.
//
// types.cpp - dim core
#include "pch.h"
#pragma hdrstop

using namespace std;
using namespace Dim;


/****************************************************************************
*
*   Declarations
*
***/

constexpr auto kUnixTimeDiff = 11'644'473'600ull;


/****************************************************************************
*
*   Clock
*
***/

//===========================================================================
// static
Clock::time_point Clock::now() noexcept {
    return (time_point(duration(iClockGetTicks())));
}

//===========================================================================
// static
time_t Clock::to_time_t(const time_point & time) noexcept {
    auto ticks = time.time_since_epoch().count();
    return (time_t)(ticks / kClockTicksPerSecond - kUnixTimeDiff);
}

//===========================================================================
// static
Clock::time_point Clock::from_time_t(time_t t) noexcept {
    return (time_point(duration((t + kUnixTimeDiff) * kClockTicksPerSecond)));
}


/****************************************************************************
*
*   Time8601Str
*
***/

//===========================================================================
Time8601Str::Time8601Str() {
    m_data[0] = 0;
    m_data[sizeof(m_data) - 1] = sizeof(m_data) - 1;
}

//===========================================================================
Time8601Str::Time8601Str(TimePoint time, unsigned precision, int tzMinutes) {
    set(time, precision, tzMinutes);
}

//===========================================================================
string_view Time8601Str::view() const {
    return string_view(
        m_data, 
        sizeof(m_data) - 1 - m_data[sizeof(m_data) - 1]
    );
}

//===========================================================================
static char * add4Digit(char * out, int val, char suffix) {
    assert(val >= 0 && val <= 9999);
    out[3] = (val % 10) + '0';
    val /= 10;
    out[2] = (val % 10) + '0';
    val /= 10;
    out[1] = (val % 10) + '0';
    val /= 10;
    out[0] = (val % 10) + '0';
    if (suffix) {
        out[4] = suffix;
        return out + 5;
    }
    return out + 4;
}

//===========================================================================
static char * add2Digit(char * out, int val, char suffix) {
    assert(val >= 0 && val <= 99);
    out[1] = (val % 10) + '0';
    val /= 10;
    out[0] = (val % 10) + '0';
    if (suffix) {
        out[2] = suffix;
        return out + 3;
    }
    return out + 2;
}

//===========================================================================
Time8601Str & Time8601Str::set(
    TimePoint time, 
    unsigned precision, 
    int tzMinutes
) {
    assert(precision <= 7);
    time += (chrono::minutes) tzMinutes;
    tm desc;
    iTimeGetDesc(desc, time);
    char * out = add4Digit(m_data, desc.tm_year + 1900, '-');
    out = add2Digit(out, desc.tm_mon + 1, '-');
    out = add2Digit(out, desc.tm_mday, 'T');
    out = add2Digit(out, desc.tm_hour, ':');
    out = add2Digit(out, desc.tm_min, ':');
    out = add2Digit(out, desc.tm_sec, 0);
    if (precision) {
        *out++ = '.';
        int64_t frac = time.time_since_epoch().count() % kClockTicksPerSecond;
        while (precision--) {
            frac *= 10;
            auto dig = (unsigned char) (frac / kClockTicksPerSecond);
            *out++ = dig + '0';
            frac %= kClockTicksPerSecond;
        }
    }
    if (!tzMinutes) {
        *out++ = 'Z';
    } else {
        if (tzMinutes < 0) {
            *out++ = '-';
            tzMinutes = -tzMinutes;
        } else {
            *out++ = '+';
        }
        out = add2Digit(out, tzMinutes / 60, ':');
        out = add2Digit(out, tzMinutes % 60, 0);
    }
    *out = 0;
    m_data[sizeof(m_data) - 1] = sizeof(m_data) - 1 - (uint8_t) (out - m_data);
    return *this;
}
