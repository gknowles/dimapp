// Copyright Glen Knowles 2016 - 2018.
// Distributed under the Boost Software License, Version 1.0.
//
// console.h - dim app
#pragma once

#include "cppconf/cppconf.h"

#include "core/log.h"

#include <cstdint>

namespace Dim {

enum ConsoleAttr {
    kConsoleNormal,
    kConsoleGreen,
    kConsoleHighlight,
    kConsoleWarn,
    kConsoleError,
};

class ConsoleScopedAttr {
    int m_active{true};

public:
    ConsoleScopedAttr(ConsoleAttr attr);
    ~ConsoleScopedAttr();
};

// Is there a console attached?
bool consoleAttached();

// Also enables/disables line buffering.
void consoleEnableEcho(bool enable = true);

// Replace previous line with spaces and set the cursor at its beginning.
void consoleRedoLine();

// When enabled a control-c sent to the console triggers a graceful shutdown,
// by calling appSignalShutdown(EX_IOERR), instead of immediate termination.
void consoleCatchCtrlC(bool enable = true);

// If the standard IO has been redirected these functions will reset them
// back to point to the console, if the process has a console.
void consoleResetStdin();
void consoleResetStdout();
void consoleResetStderr();

// Detaches from current console and attaches to console that the specified
// process is attached to.
bool consoleAttach(intptr_t pid);

ILogNotify * consoleBasicLogger();

} // namespace
