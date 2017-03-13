// log.h - dim core
#pragma once

#include "config.h"

#include <sstream>
#include <string_view>

namespace Dim {

enum LogType {
    kLogDebug,
    kLogInfo,
    kLogError,
    kLogCrash,
};

class ILogNotify {
public:
    virtual void onLog(LogType type, std::string_view msg) = 0;
};

void logAddNotify(ILogNotify * notify);

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
    /* [[noreturn]] */ ~LogCrash();
};

} // namespace

Detail::Log logMsgDebug();
Detail::Log logMsgInfo();
Detail::Log logMsgError();
Detail::LogCrash logMsgCrash();

void logParseError(
    std::string_view msg,
    std::string_view path,
    size_t pos,
    std::string_view source);

} // namespace
