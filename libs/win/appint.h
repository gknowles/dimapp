// Copyright Glen Knowles 2015 - 2017.
// Distributed under the Boost Software License, Version 1.0.
//
// appint.h - dim windows platform
//
// Initialize and destroy methods called by appRun()
#pragma once

namespace Dim {

// Platform
void iPlatformInitialize();
void iPlatformConfigInitialize();

// Console
void iConsoleInitialize();

// File
void iFileInitialize();

// Pipe
void iPipeInitialize();

// Socket
void iSocketInitialize();

} // namespace
