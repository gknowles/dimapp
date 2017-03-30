// Copyright Glen Knowles 2015 - 2017.
// Distributed under the Boost Software License, Version 1.0.
//
// platformint.h - dim core
#pragma once

#include <cstdint>

namespace Dim {

// System
void iPlatformInitialize();

//---------------------------------------------------------------------------
// AppSocket
void iAppSocketInitialize();

// Console
void iConsoleInitialize();

// File
void iFileInitialize();

// Http
void iHttpRouteInitialize();

// Shutdown
void iShutdownDestroy();

// Socket
void iSocketInitialize();

// Task
void iTaskInitialize();
void iTaskDestroy();

// Timer
void iTimerInitialize();
void iTimerDestroy();

// Types
int64_t iClockGetTicks();

} // namespace
