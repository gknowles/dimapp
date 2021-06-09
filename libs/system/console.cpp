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
    void onLog(const LogMsg & log) override;
};
} // namespace
static ConsoleLogger s_consoleLogger;

//===========================================================================
void ConsoleLogger::onLog(const LogMsg & log) {
    char stkbuf[256];
    unique_ptr<char[]> heapbuf;
    auto buf = stkbuf;
    if (log.msg.size() >= size(stkbuf)) {
        heapbuf.reset(new char[log.msg.size() + 1]);
        buf = heapbuf.get();
    }
    memcpy(buf, log.msg.data(), log.msg.size());
    buf[log.msg.size()] = '\n';
    if (log.type >= kLogTypeError) {
        ConsoleScopedAttr attr(kConsoleError);
        cout.write(buf, log.msg.size() + 1);
        if (log.type == kLogTypeFatal)
            cout.flush();
    } else if (log.type == kLogTypeWarn) {
        ConsoleScopedAttr attr(kConsoleWarn);
        cout.write(buf, log.msg.size() + 1);
    } else {
        cout.write(buf, log.msg.size() + 1);
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
