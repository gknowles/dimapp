// Copyright Glen Knowles 2015 - 2017.
// Distributed under the Boost Software License, Version 1.0.
//
// log.h - dim core
#pragma once

#include "cppconf/cppconf.h"

#include <sstream>
#include <string_view>

namespace Dim {

enum LogType {
    kLogTypeDebug,
    kLogTypeInfo,
    kLogTypeError,
    kLogTypeCrash,
    kLogTypes
};

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

Detail::Log logMsgDebug();
Detail::Log logMsgInfo();
Detail::Log logMsgError();
Detail::LogCrash logMsgCrash();

void logParseError(
    std::string_view msg,
    std::string_view objname,
    size_t pos,
    std::string_view source);

// Returns the number of messages of the selected type that have been logged
int logGetMsgCount(LogType type);

} // namespace
