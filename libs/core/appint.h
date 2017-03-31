// Copyright Glen Knowles 2015 - 2017.
// Distributed under the Boost Software License, Version 1.0.
//
// appint.h - dim core
//
// Initialize and destroy methods called by appRun()

#pragma once

#include <cstdint>

namespace Dim {

// Shutdown
void iShutdownDestroy();

// Task
void iTaskInitialize();
void iTaskDestroy();

// Timer
void iTimerInitialize();
void iTimerDestroy();

// Types
int64_t iClockGetTicks();

} // namespace