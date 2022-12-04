// Copyright Glen Knowles 2015 - 2022.
// Distributed under the Boost Software License, Version 1.0.
//
// appint.h - dim windows platform
//
// Initialize and destroy methods called by appRun()
#pragma once

namespace Dim {

// Platform
enum class PlatformInit {
    // Before all other initialization functions, on main thread.
    kBeforeAll,

    // Called on event thread, after perf, log, and thread are set up, but
    // before anything else.
    kBeforeAppVars,

    // Called on event thread. After app.xml config file has been processed and
    // directories, socket address, version info, and so on, have all been
    // specified.
    kAfterAppVars,
};
void iPlatformInitialize(PlatformInit phase);
void iPlatformDestroy();

// File
void iFileInitialize();

// Pipe
void iPipeInitialize();

// Socket
void iSocketInitialize();

} // namespace
