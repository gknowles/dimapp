// Copyright Glen Knowles 2015 - 2022.
// Distributed under the Boost Software License, Version 1.0.
//
// log.h - dim core
#pragma once

#include "cppconf/cppconf.h"

#include <source_location>
#include <spanstream>
#include <string_view>

namespace Dim {


/****************************************************************************
*
*   Declarations
*
***/

enum LogType {
    kLogTypeInvalid,
    kLogTypeDebug,
    kLogTypeInfo,
    kLogTypeWarn,
    kLogTypeError,
    kLogTypeFatal,
    kLogTypes
};
const char * toString(LogType type, const char def[]);
LogType fromString(std::string_view src, LogType def);

struct LogMsg {
    LogType type;
    std::source_location loc;
    std::string_view msg;
};

namespace Detail {

class Log : public std::ospanstream {
public:
    Log(LogType type, std::source_location loc);
    Log(Log && from) noexcept;
    ~Log();

private:
    LogType m_type;
    std::source_location m_loc;
    char m_buf[256];
};

class LogFatal : public Log {
public:
    using Log::Log;
    [[noreturn]] ~LogFatal();
};

} // namespace


/****************************************************************************
*
*   Log messages
*
*   Expected usage:
*   logMsgError() << "It went kablooie";
*
***/

Detail::Log logMsgDebug(
    std::source_location loc = std::source_location::current()
);
Detail::Log logMsgInfo(
    std::source_location loc = std::source_location::current()
);
Detail::Log logMsgWarn(
    std::source_location loc = std::source_location::current()
);
Detail::Log logMsgError(
    std::source_location loc = std::source_location::current()
);
Detail::LogFatal logMsgFatal(
    std::source_location loc = std::source_location::current()
);

// Logs an error of the form "name(<line no>): msg", followed by two info lines
// with part or all of the line of content containing the error with a caret
// indicating it's exact position.
//
// "content" should be the entire source being parsed and "pos" the offset into
// the content of the error. The line number is calculated by counting '\n'
// characters.
void logParseError(
    std::string_view msg,
    std::string_view name,
    size_t pos,
    std::string_view content
);

// Renders data as 16 byte wide hex dump and debug-logs each of the resulting
// lines.
void logHexDebug(std::string_view data);

// Splits text and logs each line as separate info-logs.
void logMultiInfo(std::string_view text, char sep = '\n');


/****************************************************************************
*
*   Stopwatch
*   Used to info-log elapsed time. Timer automatically started at program
*   start.
*
*   Total time: Time spent not paused.
*   Lap time: Time since the last lap or resume, whichever is more recent.
*
***/

// Logs total time if prefix is not empty, and pauses stopwatch. Ignored if
// already paused.
void logPauseStopwatch(std::string_view prefix = "Elapsed time");

// Resets lap time, and optionally total time, to zero; resumes stopwatch.
// Ignored if not paused.
void logResumeStopwatch(bool resetTotal = false);

// Logs total time unless prefix is empty. Ignored if paused.
void logStopwatch(std::string_view prefix = "Elapsed time");

// Logs lap time unless prefix is empty, and resets lap time. Lap time is the
// time since the last lap or, if first lap, since the stopwatch started.
// Ignored if paused.
void logStopwatchLap(std::string_view prefix = "Elapsed time");


/****************************************************************************
*
*   Query log info
*
***/

// Returns the number of messages of the selected type that have been logged
int logGetMsgCount(LogType type);
void logAddMsgCount(LogType type, int count);


/****************************************************************************
*
*   Monitor message log
*
***/

class ILogNotify {
public:
    virtual ~ILogNotify() = default;
    virtual void onLog(const LogMsg & log) = 0;
};

void logMonitor(ILogNotify * notify);
void logMonitorClose(ILogNotify * notify);

// The default notifier (whether user supplied or the internal one) is only
// called if no other notifiers have been added. Setting the default to
// nullptr sets it to the internal default, which writes to std::cout.
void logDefaultMonitor(ILogNotify * notify);

} // namespace
