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
*   Helpers
*
***/

/****************************************************************************
*
*   Clock
*
***/

const int64_t kClockTicksPerTimeT{10'000'000};

//===========================================================================
// static
Clock::time_point Clock::now() noexcept {
    return (time_point(duration(iClockGetTicks())));
}

//===========================================================================
// static
time_t Clock::to_time_t(const time_point & time) noexcept {
    return ((time_t)(time.time_since_epoch().count() / kClockTicksPerTimeT));
}

//===========================================================================
// static
Clock::time_point Clock::from_time_t(time_t tm) noexcept {
    return (time_point(duration(tm * kClockTicksPerTimeT)));
}
