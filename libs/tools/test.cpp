// Copyright Glen Knowles 2022.
// Distributed under the Boost Software License, Version 1.0.
//
// test.cpp - dim tools
#include "pch.h"
#pragma hdrstop

using namespace std;
using namespace Dim;


/****************************************************************************
*
*   Message log testing helpers
*
***/

namespace {
class Logger : public ILogNotify {
public:
    void setMsgs(const vector<pair<LogType, string>> & msgs);

    // Inherited via ILogNotify
    void onLog(const LogMsg & log) override;

private:
    vector<pair<LogType, string>> m_msgs;
    bool m_registered = false;
    mutex m_mut;
};
} // namespace
static Logger s_logger;

//===========================================================================
void Logger::setMsgs(const vector<pair<LogType, string>> & msgs) {
    unique_lock lk{m_mut};
    if (!m_registered) {
        m_registered = true;
        logMonitor(this);
    }
    if (!m_msgs.empty()) {
        logMsgError() << "testLogMessages: left " << m_msgs.size()
            << " unmatched messages";
    }
    m_msgs = msgs;
}

//===========================================================================
void Logger::onLog(const LogMsg & log) {
    if (!m_msgs.empty()) {
        auto & [type, msg] = m_msgs.front();
        if (log.type == type && log.msg == msg) {
            m_msgs.erase(m_msgs.begin());
            return;
        }
    }
    consoleBasicLogger()->onLog(log);
}

//===========================================================================
void Dim::testLogMsgs(const vector<pair<LogType, string>> & msgs) {
    s_logger.setMsgs(msgs);
}


/****************************************************************************
*
*   Test helper functions
*
***/

//===========================================================================
void Dim::testSignalShutdown() {
    if (int errs = logGetMsgCount(kLogTypeError)) {
        ConsoleScopedAttr attr(kConsoleError);
        cerr << "*** TEST FAILURES: " << errs
            << " (" << appBaseName() << ")" << endl;
        appSignalShutdown(EX_SOFTWARE);
    } else {
        cout << "All tests passed"
            << " (" << appBaseName() << ")" << endl;
        appSignalShutdown(EX_OK);
    }
}
