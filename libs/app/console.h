// Copyright Glen Knowles 2016 - 2017.
// Distributed under the Boost Software License, Version 1.0.
//
// console.h - dim app
#pragma once

#include "config/config.h"

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

void consoleEnableEcho(bool enable = true);
void consoleEnableCtrlC(bool enable = true);

} // namespace
