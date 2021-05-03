// Copyright Glen Knowles 2015 - 2021.
// Distributed under the Boost Software License, Version 1.0.
//
// log.h - dim core
#pragma once

#include "cppconf/cppconf.h"

#include <string_view>
#include <strstream>

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

namespace Detail {

class Log : public std::ostrstream {
public:
    Log(LogType type);
    Log(Log && from) noexcept;
    ~Log();

private:
    LogType m_type;
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

Detail::Log logMsgDebug();
Detail::Log logMsgInfo();
Detail::Log logMsgWarn();
Detail::Log logMsgError();
Detail::LogFatal logMsgFatal();

// Logs an error of the form "name(<line no>): msg", followed by two info
// lines with part or all of the line of content containing the error
// with a caret indicating it's exact position.
//
// "content" should be the entire source being parsed and "pos" the offset
// into the content of the error. The line number is calculated from that.
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
*   Used to info-log elapsed time. Automatically started at program start.
*
***/

// Starts if not already running.
void logStartStopwatch();

// Resets total and lap times to zero. Ignored unless stopwatch is paused.
void logResetStopwatch();

// Logs total time unless prefix is empty, and pauses stopwatch. Ignored if
// paused.
void logPauseStopwatch(std::string_view prefix = "Elapsed time");

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


/****************************************************************************
*
*   Monitor message log
*
***/

class ILogNotify {
public:
    virtual ~ILogNotify() = default;
    virtual void onLog(LogType type, std::string_view msg) = 0;
};

void logMonitor(ILogNotify * notify);
void logMonitorClose(ILogNotify * notify);

// The default notifier (whether user supplied or the internal one) is only
// called if no other notifiers have been added. Setting the default to
// nullptr sets it to the internal default, which writes to std::cout.
void logDefaultMonitor(ILogNotify * notify);

} // namespace
