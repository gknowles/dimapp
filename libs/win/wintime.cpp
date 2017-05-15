// Copyright Glen Knowles 2015 - 2017.
// Distributed under the Boost Software License, Version 1.0.
//
// wintime.cpp - dim windows platform
#include "pch.h"
#pragma hdrstop

using namespace std;
using namespace Dim;


/****************************************************************************
*
*   Time
*
***/

//===========================================================================
int64_t Dim::iClockGetTicks() {
    LARGE_INTEGER out;
    FILETIME ft;
    GetSystemTimeAsFileTime(&ft);
    out.HighPart = ft.dwHighDateTime;
    out.LowPart = ft.dwLowDateTime;
    return out.QuadPart;
}

//===========================================================================
bool Dim::iTimeGetDesc(tm & tm, TimePoint time) {
    LARGE_INTEGER in;
    in.QuadPart = time.time_since_epoch().count();
    FILETIME ft;
    ft.dwHighDateTime = in.HighPart;
    ft.dwLowDateTime = in.LowPart;
    SYSTEMTIME st;
    if (!FileTimeToSystemTime(&ft, &st)) {
        tm = {};
        return false;
    }

    if (st.wYear > 9999) {
        tm.tm_year = 9999 - 1900;
        tm.tm_mon = 11;
        tm.tm_mday = 31;
        tm.tm_wday = 5;
    } else {
        tm.tm_year = st.wYear - 1900;
        tm.tm_mon = st.wMonth - 1;
        tm.tm_mday = st.wDay;
        tm.tm_wday = st.wDayOfWeek;
    }
    tm.tm_hour = st.wHour;
    tm.tm_min = st.wMinute;
    tm.tm_sec = st.wSecond;

    tm.tm_isdst = -1;
    tm.tm_yday = -1;
    return true;
}

//===========================================================================
int Dim::timeZoneMinutes(TimePoint time) {
    LARGE_INTEGER in;
    in.QuadPart = time.time_since_epoch().count();
    FILETIME ft;
    ft.dwHighDateTime = in.HighPart;
    ft.dwLowDateTime = in.LowPart;

    FILETIME local;
    if (!FileTimeToLocalFileTime(&ft, &local))
        return 0;
    LARGE_INTEGER out;
    out.HighPart = local.dwHighDateTime;
    out.LowPart = local.dwLowDateTime;
    int64_t diff = (out.QuadPart - in.QuadPart) / kClockTicksPerSecond / 60;
    return (int) diff;
}
