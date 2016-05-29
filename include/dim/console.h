// console.h - dim core
#pragma once

#include "dim/config.h"

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
    ConsoleScopedAttr (ConsoleAttr attr);
    ~ConsoleScopedAttr ();
};

void consoleEnableEcho (bool enable = true);
void consoleEnableCtrlC (bool enable = true);

} // namespace
