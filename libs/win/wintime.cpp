// Copyright Glen Knowles 2015 - 2018.
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
int Dim::timeZoneMinutes(TimePoint time) {
    LARGE_INTEGER in;
    in.QuadPart = time.time_since_epoch().count();
    FILETIME ft;
    ft.dwHighDateTime = in.HighPart;
    ft.dwLowDateTime = in.LowPart;

    SYSTEMTIME st;
    if (!FileTimeToSystemTime(&ft, &st))
        return 0;
    SYSTEMTIME lst;
    if (!SystemTimeToTzSpecificLocalTime(NULL, &st, &lst))
        return 0;
    FILETIME local;
    if (!SystemTimeToFileTime(&lst, &local))
        return 0;

    LARGE_INTEGER out;
    out.HighPart = local.dwHighDateTime;
    out.LowPart = local.dwLowDateTime;
    int64_t diff = (out.QuadPart - in.QuadPart) / kClockTicksPerSecond / 60;
    return (int) diff;
}

//===========================================================================
bool Dim::timeFromDesc(TimePoint * time, const tm & tm) {
    auto src = tm;
    auto t = _mkgmtime(&src); // Microsoft extension, libc has timegm()
    if (t == -1) {
        *time = {};
        return false;
    }
    *time = Clock::from_time_t(t);
    return true;
}
