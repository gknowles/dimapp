// Copyright Glen Knowles 2015 - 2020.
// Distributed under the Boost Software License, Version 1.0.
//
// appint.h - dim windows platform
//
// Initialize and destroy methods called by appRun()
#pragma once

namespace Dim {

// Platform
enum class PlatformInit {
    kBeforeAppVars,
    kAfterAppVars,
};
void iPlatformInitialize(PlatformInit phase);

// Console
void iConsoleInitialize();
void iConsoleDestroy();

// File
void iFileInitialize();

// Pipe
void iPipeInitialize();

// Socket
void iSocketInitialize();

} // namespace
