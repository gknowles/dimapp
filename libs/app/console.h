// Copyright Glen Knowles 2016 - 2017.
// Distributed under the Boost Software License, Version 1.0.
//
// console.h - dim app
#pragma once

#include "cppconf/cppconf.h"

namespace Dim {

enum ConsoleAttr {
    kConsoleNormal,
    kConsoleGreen,
    kConsoleHighlight,
    kConsoleError,
};

class ConsoleScopedAttr {
    int m_attr;

public:
    ConsoleScopedAttr(ConsoleAttr attr);
    ~ConsoleScopedAttr();
};

// Also enables/disables line buffering.
void consoleEnableEcho(bool enable = true);

// When enabled a control-c sent to the console triggers a graceful shutdown, 
// by calling appSignalShutdown(EX_IOERR), instead of immediate termination.
void consoleCatchCtrlC(bool enable = true);

// If the standard IO has been redirected these functions will reset them 
// back to point to the console, if the process has a console.
void consoleResetStdin();
void consoleResetStdout();
void consoleResetStderr();

} // namespace
