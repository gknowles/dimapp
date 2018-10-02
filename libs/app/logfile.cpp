// Copyright Glen Knowles 2017 - 2018.
// Distributed under the Boost Software License, Version 1.0.
//
// logfile.cpp - dim app
#include "pch.h"
#pragma hdrstop

using namespace std;
using namespace Dim;


/****************************************************************************
*
*   Private
*
***/

constexpr unsigned kLogBufferSize = 65'536;

namespace {

class LogBuffer : IFileWriteNotify {
public:
    LogBuffer();
    void writeLog(string_view msg, bool wait);

    // returns true if close completed, otherwise try again until no more
    // writes are pending.
    bool closeLog();

private:
    void onFileWrite(
        int written,
        string_view data,
        int64_t offset,
        FileHandle f
    ) override;

    mutex m_mut;
    FileHandle m_file;
    string m_pending;
    string m_writing;
    bool m_closing{false};
};

} // namespace

static LogType s_logLevel{kLogTypeInfo};
static LogBuffer s_buffer;
static string s_hostname = "-";
static Path s_logfile;

static auto & s_perfDropped = uperf("logfile.buffer dropped");


/****************************************************************************
*
*   LogBuffer
*
***/

//===========================================================================
LogBuffer::LogBuffer() {
    m_writing.reserve(kLogBufferSize);
    m_pending.reserve(kLogBufferSize);
}

//===========================================================================
void LogBuffer::writeLog(string_view msg, bool wait) {
    unique_lock lk{m_mut};
    if (m_writing.empty()) {
        m_writing.append(msg);
        m_writing.push_back('\n');
        if (!m_file) {
            m_file = fileOpen(
                s_logfile,
                File::fCreat | File::fReadWrite | File::fDenyNone
            );
            if (!m_file) {
                lk.unlock();
                logMsgError() << "fileOpen(log file): " << errno;
                return;
            }
        }
        if (wait) {
            fileAppendWait(m_file, m_writing.data(), m_writing.size());
            m_writing.clear();
        } else {
            fileAppend(this, m_file, m_writing.data(), m_writing.size());
        }
        return;
    }
    if (m_pending.size() + msg.size() > kLogBufferSize) {
        s_perfDropped += 1;
        m_pending = msg;
    } else {
        m_pending.append(msg);
    }
    m_pending.push_back('\n');
    if (wait) {
        fileAppendWait(m_file, m_pending.data(), m_pending.size());
        m_pending.clear();
    }
}

//===========================================================================
bool LogBuffer::closeLog() {
    scoped_lock lk{m_mut};
    if (m_writing.empty()) {
        if (m_file) {
            fileClose(m_file);
            m_file = {};
        }
        return true;
    }
    m_closing = true;
    return false;
}

//===========================================================================
void LogBuffer::onFileWrite(
    int written,
    string_view data,
    int64_t offset,
    FileHandle f
) {
    scoped_lock lk{m_mut};
    m_writing.swap(m_pending);
    m_pending.clear();
    if (m_writing.empty()) {
        if (m_closing) {
            fileClose(m_file);
            m_file = {};
        }
        return;
    }
    fileAppend(this, m_file, m_writing.data(), m_writing.size());
}


/****************************************************************************
*
*   Logger
*
***/

namespace {
class Logger : public ILogNotify {
    void onLog(LogType type, string_view msg) override;
};
} // namespace
static Logger s_logger;

//===========================================================================
void Logger::onLog(LogType type, string_view msg) {
    assert(type < kLogTypes);
    if (type < appLogLevel())
        return;

    char tmp[256];
    assert(sizeof(tmp) + 2 <= kLogBufferSize);
    const unsigned kFacility = 3; // system daemons
    int pri = 8 * kFacility;
    switch (type) {
    case kLogTypeFatal: pri += 2; break;
    case kLogTypeError: pri += 3; break;
    case kLogTypeWarn: pri += 4; break;
    case kLogTypeInfo: pri += 6; break;
    case kLogTypeDebug: pri += 7; break;
    default: break;
    }
    snprintf(tmp, size(tmp), "<%d>1 %s %s %s %s %s %s %.*s",
        pri,
        Time8601Str{}.set().c_str(),
        s_hostname.c_str(),
        "appName",
        "-",    // procid
        "-",    // msgid
        "-",    // structured data
        (int) msg.size(),
        msg.data()
    );
    s_buffer.writeLog(tmp, type == kLogTypeFatal);
}


/****************************************************************************
*
*   app.xml monitor
*
***/

namespace {
class ConfigAppXml : public IConfigNotify {
    void onConfigChange(const XDocument & doc) override;
};
} // namespace
static ConfigAppXml s_appXml;

//===========================================================================
void ConfigAppXml::onConfigChange(const XDocument & doc) {
    auto raw = configString(doc, "LogLevel", "warn");
    if (auto level = fromString(raw, kLogTypeInvalid)) {
        s_logLevel = level;
    } else {
        logMsgError() << "Invalid log level '" << raw << "'";
    }
}


/****************************************************************************
*
*   ShutdownNotify
*
***/

namespace {
class ShutdownNotify : public IShutdownNotify {
    void onShutdownConsole(bool firstTry) override;
};
} // namespace
static ShutdownNotify s_cleanup;

//===========================================================================
void ShutdownNotify::onShutdownConsole(bool firstTry) {
    if (firstTry) {
        vector<PerfValue> perfs;
        perfGetValues(&perfs, true);
        for (auto && perf : perfs) {
            if (perf.value != "0")
                logMsgInfo() << "perf: " << perf.value << ' ' << perf.name;
        }
        return shutdownIncomplete();
    }

    if (!s_buffer.closeLog())
        return shutdownIncomplete();
}


/****************************************************************************
*
*   Public API
*
***/

//===========================================================================
void Dim::iLogFileInitialize() {
    shutdownMonitor(&s_cleanup);
    configMonitor("app.xml", &s_appXml);

    if (!appLogPath(&s_logfile, "server.log"))
        logMsgFatal() << "Invalid log path: " << s_logfile;

    logMonitor(&s_logger);
}

//===========================================================================
LogType Dim::appLogLevel() {
    return s_logLevel;
}
