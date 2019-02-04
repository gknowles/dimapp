// Copyright Glen Knowles 2015 - 2018.
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
char const * toString(LogType type, char const def[]);
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

// Renders data as 16 byte wide hex dump and debug logs each of the resulting
// lines.
void logHexDebug(std::string_view data);


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
