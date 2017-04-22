// Copyright Glen Knowles 2017.
// Distributed under the Boost Software License, Version 1.0.
//
// typesint.h - dim core
//
// Initialize and destroy methods called by appRun()

#pragma once

#include <cstdint>
#include <ctime>

namespace Dim {

// Time
int64_t iClockGetTicks();

// Day of year (tm_yday) and daylight savings time flag (tm_isdst) are not
// support and set to -1.
bool iTimeGetDesc(std::tm & tm, TimePoint time);

} // namespace
