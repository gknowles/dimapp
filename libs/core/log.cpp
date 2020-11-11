// Copyright Glen Knowles 2015 - 2018.
// Distributed under the Boost Software License, Version 1.0.
//
// log.cpp - dim core
#include "pch.h"
#pragma hdrstop

using namespace std;
using namespace Dim;


/****************************************************************************
*
*   Private
*
***/

namespace {

class DefaultLogger : public ILogNotify {
    void onLog(LogType type, string_view msg) override;
};

class LogMsgScope {
public:
    explicit LogMsgScope(LogType type);
    ~LogMsgScope();

    bool inProgress() const { return m_inProgress; }

private:
    LogType m_type;
    bool m_inProgress;
};

} // namespace

static shared_mutex s_mut;
static DefaultLogger s_fallback;
static vector<ILogNotify *> s_loggers;
static ILogNotify * s_defaultLogger{&s_fallback};
static ILogNotify * s_initialDefault;
static size_t s_initialNumLoggers;

static thread_local bool t_inProgress;


/****************************************************************************
*
*   Performance counters
*
***/

static PerfCounter<int> * s_perfs[] = {
    nullptr, // log invalid
    &iperf("log.debug"),
    &iperf("log.info"),
    &iperf("log.warn"),
    &iperf("log.error"),
    nullptr, // log fatal
};
static_assert(size(s_perfs) == kLogTypes);

static auto & s_perfRecurse = uperf("log.recursion");


/****************************************************************************
*
*   Helpers
*
***/

//===========================================================================
static void logMsg(LogType type, string_view msg) {
    LogMsgScope scope(type);

    if (auto perf = s_perfs[type])
        *perf += 1;
    if (scope.inProgress()) {
        s_perfRecurse += 1;
        return;
    }

    if (!msg.empty() && msg.back() == '\n')
        msg.remove_suffix(1);

    shared_lock lk{s_mut};
    if (s_loggers.empty()) {
        s_defaultLogger->onLog(type, msg);
    } else {
        for (auto && notify : s_loggers) {
            notify->onLog(type, msg);
        }
    }
}


/****************************************************************************
*
*   LogMsgScope
*
***/

//===========================================================================
LogMsgScope::LogMsgScope(LogType type)
    : m_type{type}
    , m_inProgress{t_inProgress}
{
    t_inProgress = true;
}

//===========================================================================
LogMsgScope::~LogMsgScope() {
    t_inProgress = false;
    if (m_type == kLogTypeFatal)
        abort();
}


/****************************************************************************
*
*   DefaultLogger
*
***/

//===========================================================================
void DefaultLogger::onLog(LogType type, string_view msg) {
    cout << msg << '\n';
    if (type == kLogTypeFatal)
        cout.flush();
}


/****************************************************************************
*
*   Log
*
***/

//===========================================================================
Detail::Log::Log(LogType type)
    : ostrstream(m_buf, size(m_buf) - 1)
    , m_type(type)
{}

//===========================================================================
Detail::Log::Log(Log && from) noexcept
    : ostrstream(m_buf, size(m_buf) - 1)
    , m_type(from.m_type)
{
    write(from.m_buf, from.pcount());
    from.m_type = kLogTypeInvalid;
}

//===========================================================================
Detail::Log::~Log() {
    if (m_type) {
        put(0);
        m_buf[size(m_buf) - 1] = 0;
        auto s = str();
        logMsg(m_type, s);
    }
}

//===========================================================================
Detail::LogFatal::~LogFatal()
{}


/****************************************************************************
*
*   Internal API
*
***/

//===========================================================================
void Dim::iLogInitialize() {
    unique_lock lk{s_mut};
    s_initialDefault = s_defaultLogger;
    s_initialNumLoggers = s_loggers.size();
    logStartStopwatch();
}

//===========================================================================
void Dim::iLogDestroy() {
    unique_lock lk{s_mut};
    s_defaultLogger = s_initialDefault;
    assert(s_loggers.size() >= s_initialNumLoggers);
    s_loggers.resize(s_initialNumLoggers);
}


/****************************************************************************
*
*   Public API
*
***/

//===========================================================================
// Log types
//===========================================================================
const TokenTable::Token s_logTypes[] = {
    { kLogTypeDebug, "debug" },
    { kLogTypeInfo,  "info" },
    { kLogTypeWarn,  "warn" },
    { kLogTypeError, "error" },
    { kLogTypeFatal, "fatal" },
};
const TokenTable s_logTypeTbl{s_logTypes};

//===========================================================================
const char * Dim::toString(LogType type, const char def[]) {
    return tokenTableGetName(s_logTypeTbl, type, def);
}

//===========================================================================
LogType Dim::fromString(std::string_view src, LogType def) {
    return tokenTableGetEnum(s_logTypeTbl, src, def);
}

//===========================================================================
// Monitor log messages
//===========================================================================
void Dim::logDefaultMonitor(ILogNotify * notify) {
    unique_lock lk{s_mut};
    s_defaultLogger = notify ? notify : &s_fallback;
}

//===========================================================================
void Dim::logMonitor(ILogNotify * notify) {
    unique_lock lk{s_mut};
    s_loggers.push_back(notify);
}

//===========================================================================
void Dim::logMonitorClose(ILogNotify * notify) {
    unique_lock lk{s_mut};
    s_loggers.erase(
        remove(s_loggers.begin(), s_loggers.end(), notify),
        s_loggers.end()
    );
}

//===========================================================================
// Query log info
//===========================================================================
int Dim::logGetMsgCount(LogType type) {
    assert(type < kLogTypes);
    if (auto perf = s_perfs[type])
        return *perf;
    return 0;
}

//===========================================================================
// Log messages
//===========================================================================
Detail::Log Dim::logMsgDebug() {
    return kLogTypeDebug;
}

//===========================================================================
Detail::Log Dim::logMsgInfo() {
    return kLogTypeInfo;
}

//===========================================================================
Detail::Log Dim::logMsgWarn() {
    return kLogTypeWarn;
}

//===========================================================================
Detail::Log Dim::logMsgError() {
    return kLogTypeError;
}

//===========================================================================
Detail::LogFatal Dim::logMsgFatal() {
    return kLogTypeFatal;
}

//===========================================================================
void Dim::logParseError(
    string_view msg,
    string_view name,
    size_t pos,
    string_view content) {

    auto lineNum = 1 + count(content.begin(), content.begin() + pos, '\n');
    logMsgError() << name << "(" << lineNum << "): " << msg;

    bool leftTrunc = false;
    bool rightTrunc = false;
    size_t first = content.find_last_of('\n', pos);
    first = (first == string::npos) ? 0 : first + 1;
    if (pos - first > 50) {
        leftTrunc = true;
        first = pos - 50;
    }
    size_t last = content.find_first_of('\n', pos);
    last = content.find_last_not_of(" \t\r\n", last);
    last = (last == string::npos) ? content.size() : last + 1;
    if (last - first > 78) {
        rightTrunc = true;
        last = first + 78;
    }
    size_t len = last - first;
    auto line = string(content.substr(first, len));
    for (char & ch : line) {
        if (iscntrl((unsigned char)ch))
            ch = ch == '\t' ? ' ' : '.';
    }
    logMsgInfo() << string(leftTrunc * 3, '.') << line
            << string(rightTrunc * 3, '.') << '\n'
        << string(pos - first + leftTrunc * 3, ' ') << '^';
}

//===========================================================================
void Dim::logHexDebug(string_view data) {
    const unsigned char * ptr = (const unsigned char *) data.data();
    for (unsigned pos = 0; pos < data.size(); pos += 16, ptr += 16) {
        auto os = logMsgDebug();
        os << setw(6) << pos << ':';
        for (unsigned i = 0; i < 16; ++i) {
            if (i % 4 == 0) os.put(' ');
            if (pos + i < data.size()) {
                hexByte(os, ptr[i]);
            } else {
                os << "  ";
            }
        }
        os << "  ";
        for (unsigned i = 0; i < 16; ++i) {
            if (pos + i < data.size())
                os.put(isprint(ptr[i]) ? (char) ptr[i] : '.');
        }
    }
}


/****************************************************************************
*
*   Stopwatch
*
***/

static mutex s_watchMut;
static Duration s_watchTotal;
static TimePoint s_watchStart;
static TimePoint s_watchLap;
static bool s_watchRunning;

//===========================================================================
void Dim::logStartStopwatch() {
    scoped_lock lk{s_watchMut};
    if (s_watchRunning)
        return;
    s_watchRunning = true;
    s_watchStart = Clock::now();
    s_watchLap = {};
}

//===========================================================================
void Dim::logResetStopwatch() {
    scoped_lock lk{s_watchMut};
    if (s_watchRunning)
        return;
    s_watchTotal = {};
}

//===========================================================================
void Dim::logPauseStopwatch(string_view prefix) {
    scoped_lock lk{s_watchMut};
    if (!s_watchRunning)
        return;
    s_watchRunning = false;
    s_watchTotal += Clock::now() - s_watchStart;

    if (!prefix.empty()) {
        chrono::duration<double> elapsed = s_watchTotal;
        logMsgInfo() << prefix << ": " << elapsed.count() << " seconds.";
    }
}

//===========================================================================
void Dim::logStopwatch(string_view prefix) {
    scoped_lock lk{s_watchMut};
    if (!s_watchRunning)
        return;

    if (!prefix.empty()) {
        chrono::duration<double> elapsed = s_watchTotal
            + Clock::now() - s_watchStart;
        logMsgInfo() << prefix << ": " << elapsed.count() << " seconds.";
    }
}

//===========================================================================
void Dim::logStopwatchLap(std::string_view prefix) {
    scoped_lock lk{s_watchMut};
    if (!s_watchRunning)
        return;
    auto start = s_watchLap ? s_watchLap : s_watchStart;
    s_watchLap = Clock::now();

    if (!prefix.empty()) {
        chrono::duration<double> elapsed = s_watchLap - start;
        logMsgInfo() << prefix << ": " << elapsed.count() << " seconds.";
    }
}
