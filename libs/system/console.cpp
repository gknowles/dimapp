// Copyright Glen Knowles 2018.
// Distributed under the Boost Software License, Version 1.0.
//
// console.cpp - dim system
#include "pch.h"
#pragma hdrstop

using namespace std;
using namespace Dim;


/****************************************************************************
*
*   ConsoleLogger
*
***/

namespace {
class ConsoleLogger : public ILogNotify {
    void onLog(LogType type, string_view msg) override;
};
} // namespace
static ConsoleLogger s_consoleLogger;

//===========================================================================
void ConsoleLogger::onLog(LogType type, string_view msg) {
    char stkbuf[256];
    unique_ptr<char[]> heapbuf;
    auto buf = stkbuf;
    if (msg.size() >= size(stkbuf)) {
        heapbuf.reset(new char[msg.size() + 1]);
        buf = heapbuf.get();
    }
    memcpy(buf, msg.data(), msg.size());
    buf[msg.size()] = '\n';
    if (type == kLogTypeError) {
        ConsoleScopedAttr attr(kConsoleError);
        cout.write(buf, msg.size() + 1);
    } else if (type == kLogTypeWarn) {
        ConsoleScopedAttr attr(kConsoleWarn);
        cout.write(buf, msg.size() + 1);
    } else {
        cout.write(buf, msg.size() + 1);
    }
}


/****************************************************************************
*
*   Public API
*
***/

//===========================================================================
ILogNotify * Dim::consoleBasicLogger() {
    return &s_consoleLogger;
}
