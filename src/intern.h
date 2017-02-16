// intern.h - dim core
#pragma once

#include <cstdint>

namespace Dim {

// System
void iSystemInitialize();

//---------------------------------------------------------------------------
// AppSocket
void iAppSocketInitialize();

// Console
void iConsoleInitialize();

// File
void iFileInitialize();

// Hash
void iHashInitialize();

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
