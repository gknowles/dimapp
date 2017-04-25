// Copyright Glen Knowles 2015 - 2017.
// Distributed under the Boost Software License, Version 1.0.
//
// log.h - dim core
#pragma once

#include "cppconf/cppconf.h"

#include <sstream>
#include <string_view>

namespace Dim {


/****************************************************************************
*
*   Declarations
*
***/

enum LogType {
    kLogTypeDebug,
    kLogTypeInfo,
    kLogTypeError,
    kLogTypeCrash,
    kLogTypes
};

namespace Detail {

class Log : public std::ostringstream {
public:
    Log(LogType type);
    Log(Log && from);
    ~Log();

private:
    LogType m_type;
};

class LogCrash : public Log {
public:
    using Log::Log;
    [[noreturn]] ~LogCrash();
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
Detail::Log logMsgError();
Detail::LogCrash logMsgCrash();

// Logs an error of the form "name(<line no>): msg", followed by two info 
// lines with part or all of the line of content containing the error 
// with a caret indicating it's exact position.
//
// "content" should be the entire source being parsed and "pos" the failing
// offset into the content. The line number is calculated from that.
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
    virtual ~ILogNotify() {}
    virtual void onLog(LogType type, std::string_view msg) = 0;
};

void logMonitor(ILogNotify * notify);

// The default notifier (whether user supplied or the internal one) is only
// called if no other notifiers have been added. Setting the default to
// nullptr sets it to the internal default, which writes to std::cout.
void logDefaultMonitor(ILogNotify * notify);

} // namespace
