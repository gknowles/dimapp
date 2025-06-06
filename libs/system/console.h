// Copyright Glen Knowles 2016 - 2025.
// Distributed under the Boost Software License, Version 1.0.
//
// console.h - dim system
#pragma once

#include "cppconf/cppconf.h"

#include "core/log.h"

#include <cstdint>

namespace Dim {

enum ConsoleAttr {
    kConsoleInvalid,
    kConsoleNormal, // normal white
    kConsoleCheer,  // bright green
    kConsoleNote,   // bright cyan
    kConsoleWarn,   // bright yellow
    kConsoleError,  // bright red
};
[[nodiscard]] std::string toString(ConsoleAttr attr);
[[nodiscard]] bool parse(ConsoleAttr * out, std::string_view src);

unsigned consoleRawAttr();
void consoleRawAttr(unsigned newAttr);

class ConsoleScopedAttr {
    int m_active {true};
    unsigned m_rawAttrs {0};

public:
    ConsoleScopedAttr(ConsoleAttr attr);
    ~ConsoleScopedAttr();
};

// Is there a console attached?
bool consoleAttached();

// Also enables/disables line buffering.
void consoleEnableEcho(bool enable = true);

// Width of current console buffer
unsigned consoleWidth();

// Replace previous line with spaces and set the cursor at its beginning.
void consoleRedoLine();

// When enabled a ctrl-c or ctrl-break sent to the console triggers a graceful
// shutdown, by calling appSignalShutdown(EX_IOERR), instead of immediate
// termination.
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
