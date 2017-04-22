// Copyright Glen Knowles 2017.
// Distributed under the Boost Software License, Version 1.0.
//
// logfile.cpp - dim core
#include "pch.h"
#pragma hdrstop

using namespace std;
using namespace Dim;
namespace fs = std::experimental::filesystem;


/****************************************************************************
*
*   Private
*
***/

const unsigned kLogBufferSize = 65'536;

namespace {
class LogBuffer : IFileWriteNotify {
public:
    LogBuffer();
    void writeLog(string_view msg);

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
};
} // namespace

static LogBuffer s_buffer;
static string s_hostname = "-";
static string s_logfile;

static auto & s_perfDropped = uperf("log buffer dropped");


/****************************************************************************
*
*   Helpers
*
***/

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
void LogBuffer::writeLog(string_view msg) {
    unique_lock<mutex> lk{m_mut};
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
        fileAppend(this, m_file, m_writing.data(), m_writing.size());
        return;
    }
    if (m_pending.size() + msg.size() > kLogBufferSize) {
        s_perfDropped += 1;
        m_pending = msg;
    } else {
        m_pending.append(msg);
    }
    m_pending.push_back('\n');
}

//===========================================================================
void LogBuffer::onFileWrite(
    int written,
    string_view data,
    int64_t offset,
    FileHandle f
) {
    lock_guard<mutex> lk{m_mut};
    m_writing.swap(m_pending);
    m_pending.clear();
    if (m_writing.empty())
        return;
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
    char tmp[256];
    assert(sizeof(tmp) + 2 <= kLogBufferSize);
    const unsigned kFacility = 3; // system daemons
    int pri = 8 * kFacility;
    switch (type) {
    case kLogTypeCrash: pri += 2; break;
    case kLogTypeError: pri += 3; break;
    case kLogTypeInfo: pri += 6; break;
    case kLogTypeDebug: pri += 7; break;
    }
    TimePoint now = Clock::now();
    int tzMinutes = timeZoneMinutes(now);
    Time8601Str nowStr{now, 3, tzMinutes};
    snprintf(tmp, size(tmp), "<%d>1 %s %s %s %s %s %s %.*s", 
        pri, 
        nowStr.c_str(),
        s_hostname.c_str(),
        "appName",
        "-",    // procid
        "-",    // msgid
        "-",    // structured data
        (int) msg.size(),
        msg.data()
    );
    s_buffer.writeLog(tmp);
}


/****************************************************************************
*
*   Public API
*
***/

//===========================================================================
void Dim::iLogFileInitialize(string_view logDir) {
    //vector<Address> addrs;
    //addressGetLocal(&addrs);
    //ostringstream os;
    //os << addrs.front();
    //s_hostname = os.str();

    error_code ec;
    s_logfile = logDir;
    fs::create_directories(fs::u8path(s_logfile), ec);

    s_logfile += "/server.log";

    logMonitor(&s_logger);
}
