// Copyright Glen Knowles 2015 - 2017.
// Distributed under the Boost Software License, Version 1.0.
//
// platformint.h - dim core
//
// Initialize and destroy methods called by appRun()

#pragma once

namespace Dim {

// Platform
void iPlatformInitialize();

// Console
void iConsoleInitialize();

// File
void iFileInitialize();

// Socket
void iSocketInitialize();

} // namespace
