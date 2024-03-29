// Copyright Glen Knowles 2015 - 2023.
// Distributed under the Boost Software License, Version 1.0.
//
// types.cpp - dim core
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

constexpr auto kUnixTimeDiff = 11'644'473'600ull;


/****************************************************************************
*
*   Clock
*
***/

//===========================================================================
// static
Clock::time_point Clock::now() noexcept {
    return time_point{duration{iClockGetTicks()}};
}


/****************************************************************************
*
*   Duration to/from string
*
***/

using MillisecondsD = chrono::duration<double, milli>;
using SecondsD      = chrono::duration<double>;
using MinutesD      = chrono::duration<double, ratio<60>>;
using HoursD        = chrono::duration<double, ratio<60 * 60>>;
using DaysD         = chrono::duration<double, ratio<24 * 60 * 60>>;
using WeeksD        = chrono::duration<double, ratio<7 * 24 * 60 * 60>>;
using MonthsD       = chrono::duration<double, ratio<30 * 24 * 60 * 60>>;
using YearsD        = chrono::duration<double, ratio<365 * 24 * 60 * 60>>;

struct DurationScale {
    const char * suffix;
    double secs;
};
constexpr DurationScale s_durScales[] = {
    { "y",  365.2425 * 24 * 60 * 60 },
    { "w",  7 * 24 * 60 * 60 },
    { "d",  24 * 60 * 60 },
    { "h",  60 * 60 },
    { "m",  60 },
    { "s",  1 },
    { "ms", 0.001 },
};

//===========================================================================
// ms, s, m|min, h, d, w, mon, y
static bool interpret(Duration * out, string_view * units, double val) {
    auto ptr = units->data();
    auto eptr = ptr + units->size();
    if (ptr == eptr)
        return false;
    switch (*ptr++) {
    case 'm':
        if (ptr == eptr) {
            *out = duration_cast<Duration>((MinutesD) val);
            break;
        }
        switch (*ptr++) {
        case 's':
            *out = duration_cast<Duration>((MillisecondsD) val);
            break;
        case 'i':
            if (ptr == eptr || *ptr++ != 'n')
                return false;
            *out = duration_cast<Duration>((MinutesD) val);
            break;
        case 'o':
            if (ptr == eptr || *ptr++ != 'n')
                return false;
            *out = duration_cast<Duration>((MonthsD) val);
            break;
        default:
            return false;
        }
        break;
    case 's':
        *out = duration_cast<Duration>((SecondsD) val);
        break;
    case 'h':
        *out = duration_cast<Duration>((HoursD) val);
        break;
    case 'd':
        *out = duration_cast<Duration>((DaysD) val);
        break;
    case 'w':
        *out = duration_cast<Duration>((WeeksD) val);
        break;
    case 'y':
        *out = duration_cast<Duration>((YearsD) val);
        break;
    default:
        return false;
    }

    *units = string_view{ptr, size_t(eptr - ptr)};
    return true;
}

//===========================================================================
static string toStringOnePart(Duration val) {
    auto secs = duration_cast<SecondsD>(val).count();
    const DurationScale * scale = s_durScales;
    for (; scale < end(s_durScales) - 1; ++scale) {
        if (abs(secs) >= scale->secs)
            break;
    }
    auto out = format("{:.4g}{}", secs / scale->secs, scale->suffix);
    return out;
}

//===========================================================================
static string toStringTwoPart(Duration val) {
    auto secs = duration_cast<SecondsD>(val).count();
    const DurationScale * scale = s_durScales;
    for (; scale < end(s_durScales) - 1; ++scale) {
        if (abs(secs) >= scale->secs)
            break;
    }
    auto first = trunc(secs / scale->secs);
    auto out = format("{}{}", first, scale->suffix);
    if (scale < end(s_durScales) - 1) {
        secs -= first * scale->secs;
        secs += numeric_limits<double>::epsilon();
        auto second = trunc(abs(secs) / scale[1].secs);
        if (second)
            out += format(" {}{}", second, scale[1].suffix);
    }
    return out;
}

//===========================================================================
string Dim::toString(Duration val, DurationFormat fmt) {
    if (fmt == DurationFormat::kOnePart) {
        return toStringOnePart(val);
    } else if (fmt == DurationFormat::kTwoPart) {
        return toStringTwoPart(val);
    }
    return "<duration>";
}

//===========================================================================
bool Dim::parse(Duration * out, string_view src) {
    char buf[maxNumericChars<double>() + 1];
    buf[src.copy(buf, size(buf) - 1)] = 0;
    char * ptr;
    auto val = strtod(buf, &ptr);
    if (*ptr) {
        src = src.substr(ptr - buf);
        if (interpret(out, &src, val) && src.empty())
            return true;
    }
    *out = {};
    return false;
}


/****************************************************************************
*
*   Time8601Str
*
***/

//===========================================================================
Time8601Str::Time8601Str() {
    m_data[0] = 0;
    m_data[sizeof m_data - 1] = sizeof m_data - 1;
}

//===========================================================================
Time8601Str::Time8601Str(TimePoint time, unsigned precision)
    : Time8601Str(time, precision, timeZoneMinutes(time))
{}

//===========================================================================
Time8601Str::Time8601Str(TimePoint time, unsigned precision, int tzMinutes) {
    set(time, precision, tzMinutes);
}

//===========================================================================
string_view Time8601Str::view() const {
    return string_view(
        m_data,
        sizeof m_data - 1 - m_data[sizeof m_data - 1]
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
Time8601Str & Time8601Str::set() {
    auto now = timeNow();
    return set(now);
}

//===========================================================================
Time8601Str & Time8601Str::set(
    TimePoint time,
    unsigned precision,
    int tzMinutes
) {
    assert(precision <= 7);
    time += (minutes) tzMinutes;
    tm desc;
    timeToDesc(&desc, time);
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
    m_data[sizeof m_data - 1] = sizeof m_data - 1 - (uint8_t) (out - m_data);
    return *this;
}


/****************************************************************************
*
*   Parse 8601 time string
*
***/

//===========================================================================
bool parse4Digits(int * out, string_view & src, char suffix) {
    assert(src.size() >= 4u + (bool) suffix);
    const char * data = src.data();
    unsigned num = (unsigned char) *data++ - '0';
    if (num > 9)
        return false;
    int val = num;
    num = (unsigned char) *data++ - '0';
    if (num > 9)
        return false;
    val = 10 * val + num;
    num = (unsigned char) *data++ - '0';
    if (num > 9)
        return false;
    val = 10 * val + num;
    num = (unsigned char) *data++ - '0';
    if (num > 9)
        return false;
    val = 10 * val + num;
    if (!suffix) {
        *out = val;
        src.remove_prefix(4);
        return true;
    }
    if (*data != suffix)
        return false;
    *out = val;
    src.remove_prefix(5);
    return true;
}

//===========================================================================
bool parse2Digits(int * out, string_view & src, char suffix) {
    assert(src.size() >= 2u + (bool) suffix);
    unsigned num = (unsigned char) src[0] - '0';
    if (num > 9)
        return false;
    int val = num;
    num = (unsigned char) src[1] - '0';
    if (num > 9)
        return false;
    val = 10 * val + num;
    if (!suffix) {
        *out = val;
        src.remove_prefix(2);
        return true;
    }
    if (src[2] != suffix)
        return false;
    *out = val;
    src.remove_prefix(3);
    return true;
}

//===========================================================================
//  YYYY-MM-DD
//  YYYY-MM-DD{T,t, }hh:mm:ss[.nnnnnnn]{Z,z}
//  YYYY-MM-DD{T,t, }hh:mm:ss[.nnnnnnn]{+,-}hh:mm
bool Dim::timeParse8601(TimePoint * out, string_view str) {
    *out = {};
    tm desc = {};

    // date
    if (str.size() < 10
        || !parse4Digits(&desc.tm_year, str, '-')
        || !parse2Digits(&desc.tm_mon, str, '-')
        || !parse2Digits(&desc.tm_mday, str, 0)
    ) {
        return false;
    }
    desc.tm_year -= 1900;
    desc.tm_mon -= 1;
    if (str.empty())
        return timeFromDesc(out, desc);

    // time
    if (str[0] != 'T' && str[0] != ' ' && str[0] != 't')
        return false;
    str.remove_prefix(1);
    if (str.size() < 9
        || !parse2Digits(&desc.tm_hour, str, ':')
        || !parse2Digits(&desc.tm_min, str, ':')
        || !parse2Digits(&desc.tm_sec, str, 0)
    ) {
        return false;
    }

    // fractional seconds
    unsigned frac = 0;
    if (str[0] == '.') {
        size_t i = 1;
        for (;; ++i) {
            if (i >= str.size())
                break;
            unsigned num = (unsigned char) str[i] - '0';
            if (num > 9)
                break;
            if (i <= 7)
                frac = 10 * frac + num;
        }
        str.remove_prefix(i);
        for (; i <= 7; ++i)
            frac *= 10;
    }
    if (str.empty())
        return false;

    // timezone
    int tzMinutes = 0;
    for (;;) {
        char ch = str[0];
        str.remove_prefix(1);
        if (ch == 'Z' || ch == 'z') {
            if (!str.empty())
                return false;
            break;
        }
        if (str.size() != 5)
            return false;
        int sign;
        if (ch == '+') {
            sign = 1;
        } else if (ch == '-') {
            sign = -1;
        } else {
            return false;
        }
        int tzHours;
        if (!parse2Digits(&tzHours, str, ':')
            || !parse2Digits(&tzMinutes, str, 0)
        ) {
            return false;
        }
        tzMinutes = sign * (60 * tzHours + tzMinutes);
        break;
    }

    // combine date, time, fractional seconds, and timezone
    if (!timeFromDesc(out, desc))
        return false;
    Duration d = -(minutes) tzMinutes + (Duration) frac;
    *out += d;
    return true;
}


/****************************************************************************
*
*   Public API
*
***/

//===========================================================================
time_t Dim::to_time_t(const TimePoint & time) {
    return (time_t) timeToUnix(time);
}

//===========================================================================
TimePoint Dim::from_time_t(time_t t) {
    return timeFromUnix((uint64_t) t);
}

//===========================================================================
bool Dim::empty(const TimePoint & time) {
    return time == TimePoint{};
}

//===========================================================================
TimePoint Dim::timeNow() {
    return Clock::now();
}

//===========================================================================
std::ostream & Dim::operator<<(std::ostream & os, TimePoint time) {
    os << Time8601Str(time).c_str();
    return os;
}

//===========================================================================
bool Dim::timeToDesc(tm * tm, TimePoint time) {
    auto t = to_time_t(time);
    if (!gmtime_s(tm, &t)) {
        tm->tm_isdst = -1;
        tm->tm_yday = -1;
        return true;
    } else {
        *tm = {};
        return false;
    }
}

//===========================================================================
uint64_t Dim::timeToUnix(TimePoint time) {
    auto ticks = time.time_since_epoch().count();
    return (uint64_t)(ticks / kClockTicksPerSecond - kUnixTimeDiff);
}

//===========================================================================
TimePoint Dim::timeFromUnix(uint64_t unixTime) {
    return TimePoint{Duration{
        (unixTime + kUnixTimeDiff) * kClockTicksPerSecond
    }};
}
