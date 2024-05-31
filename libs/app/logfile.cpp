// Copyright Glen Knowles 2017 - 2023.
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

class ILogFormat {
public:
    virtual ~ILogFormat() = default;
    virtual void onFormatLog(string * out, const LogMsg & log) const = 0;
    virtual void onFormatHeader(string * out) const {}
};

class LogFile : IFileWriteNotify, public ILogNotify {
public:
    LogFile();
    void setFileName(const Path & name);
    void setFormatter(const ILogFormat * fmt);
    void writeLog(string_view msg, bool wait);

    // Returns true if close completed, otherwise try again until no more
    // writes are pending.
    bool closeLog();

private:
    void onFileWrite(const FileWriteData & data) override;
    void onLog(const LogMsg & log) override;

    mutex m_mut;
    Path m_fileName;
    FileHandle m_file;
    string m_pending;
    string m_writing;
    bool m_closing = false;
    const ILogFormat * m_fmt;
};

} // namespace

static LogType s_logLevel = kLogTypeInfo;
static LogFile s_log;

static auto & s_perfDropped = uperf("logfile.buffer dropped");
static auto & s_perfLineTrunc = uperf("logfile.log line truncated");


/****************************************************************************
*
*   LogFile
*
***/

//===========================================================================
LogFile::LogFile() {
    m_writing.reserve(kLogBufferSize);
    m_pending.reserve(kLogBufferSize);
}

//===========================================================================
void LogFile::setFileName(const Path & name) {
    m_fileName = name;
}

//===========================================================================
void LogFile::setFormatter(const ILogFormat * fmt) {
    m_fmt = fmt;
}

//===========================================================================
void LogFile::writeLog(string_view msg, bool wait) {
    using enum File::OpenMode;

    unique_lock lk{m_mut};
    if (m_writing.empty()) {
        m_writing.append(msg);
        m_writing.push_back('\n');
        if (!m_file) {
            m_closing = false;
            auto ec = fileOpen(
                &m_file,
                m_fileName,
                fCreat | fReadWrite | fDenyNone
            );
            if (ec) {
                lk.unlock();
                logMsgError() << "fileOpen(log file): " << errno;
                return;
            }
        }
        if (wait) {
            fileAppendWait(
                nullptr,
                m_file,
                m_writing.data(),
                m_writing.size()
            );
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
        fileAppendWait(nullptr, m_file, m_pending.data(), m_pending.size());
        m_pending.clear();
    }
}

//===========================================================================
bool LogFile::closeLog() {
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
void LogFile::onFileWrite(const FileWriteData & data) {
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

//===========================================================================
void LogFile::onLog(const LogMsg & log) {
    assert(log.type < kLogTypes);
    if (log.type < appLogLevel())
        return;
    string out;
    m_fmt->onFormatLog(&out, log);
    if (out.size() > kLogBufferSize - 2) {
        s_perfLineTrunc += 1;
        out.resize(kLogBufferSize - 2);
    }
    writeLog(out, log.type == kLogTypeFatal);
}


/****************************************************************************
*
*   Default Log Format
*
***/

namespace {
class DefaultFormat : public ILogFormat {
    void onFormatLog(string * out, const LogMsg & log) const override;
};
} // namespace
static DefaultFormat s_defaultFmt;

//===========================================================================
void DefaultFormat::onFormatLog(string * out, const LogMsg & log) const {
    *out = format("{} {} [{}@{}] {}",
        Time8601Str{}.set().view(),
        toUpper(toString(log.type, "UNKNOWN")),
        appName(),
        toString(appAddress().addr),
        log.msg
    );
}


/****************************************************************************
*
*   SysLog Format
*
***/

namespace {
class SysLogFormat : public ILogFormat {
    void onFormatLog(string * out, const LogMsg & log) const override;
};
} // namespace
static SysLogFormat s_syslogFmt;

//===========================================================================
void SysLogFormat::onFormatLog(string * out, const LogMsg & log) const {
    char tmp[256];
    assert(sizeof tmp + 2 <= kLogBufferSize);
    const unsigned kFacility = 3; // system daemons
    int pri = 8 * kFacility;
    switch (log.type) {
    case kLogTypeFatal: pri += 2; break;
    case kLogTypeError: pri += 3; break;
    case kLogTypeWarn: pri += 4; break;
    case kLogTypeInfo: pri += 6; break;
    case kLogTypeDebug: pri += 7; break;
    default: break;
    }
    snprintf(tmp, size(tmp), "<%d>1 %s %s %s %u %s %s %.*s",
        pri,
        Time8601Str{}.set().c_str(),
        envComputerName().c_str(),
        appName().c_str(),
        envProcessId(),
        "-",    // msgid
        "-",    // structured data
        (int) log.msg.size(),
        log.msg.data()
    );
    *out = tmp;
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
*   JsonLogFiles
*
***/

namespace {
class JsonLogFiles : public IWebAdminNotify {
    void onHttpRequest(unsigned reqId, HttpRequest & msg) override;
};
} // namespace

//===========================================================================
void JsonLogFiles::onHttpRequest(unsigned reqId, HttpRequest & msg) {
    auto res = HttpResponse(kHttpStatusOk);
    auto bld = initResponse(&res, reqId, msg);
    auto dir = appLogDir();
    bld.member("files").array();
    uint64_t bytes = 0;
    for (auto&& f : fileGlob(dir, "*.log")) {
        auto rname = f.path.view();
        rname.remove_prefix(dir.size() + 1);
        fileSize(&bytes, f.path);
        bld.object()
            .member("name", rname)
            .member("mtime", f.mtime)
            .member("size", bytes)
            .end();
    }
    bld.end();
    bld.end();
    httpRouteReply(reqId, move(res));
}


/****************************************************************************
*
*   JsonLogTail
*
***/

namespace {
class JsonLogTail : public IWebAdminNotify {
public:
    struct LogJob : IFileReadNotify {
        unsigned m_reqId {0};
        unsigned m_limit {0};
        unsigned m_found {0};
        size_t m_endPos {0};
        char m_buffer[8'192];
        HttpResponse m_res{kHttpStatusOk};
        JBuilder m_bld {&m_res.body()};

        bool onFileRead(
            size_t * bytesUsed,
            const FileReadData & data
        ) override;
    };
public:
    void onHttpRequest(unsigned reqId, HttpRequest & msg) override;
private:
    Param<unsigned> m_limit = {this, "limit", 50};
};
} // namespace

//===========================================================================
void JsonLogTail::onHttpRequest(unsigned reqId, HttpRequest & msg) {
    auto qpath = msg.query().path;
    auto prefix = httpRouteGetInfo(reqId).path.size();
    qpath.remove_prefix(prefix);
    Path path;
    if (!appLogPath(&path, qpath, false))
        return httpRouteReplyNotFound(reqId, msg);

    auto limit = clamp(*m_limit, 10u, 10'000u);

    auto job = new LogJob;
    job->m_reqId = reqId;
    job->m_limit = limit;
    job->m_bld = initResponse(&job->m_res, reqId, msg);
    job->m_bld.member("file").object()
        .member("limit", limit)
        .member("name", qpath);

    using om = File::OpenMode;
    FileHandle file;
    auto ec = fileOpen(&file, path, om::fReadOnly | om::fDenyNone);
    if (ec) {
        FileReadData data = {};
        data.f = file;
        data.ec = ec;
        size_t bytesUsed = 0;
        job->onFileRead(&bytesUsed, data);
        return;
    }
    if (ec = fileSize(&job->m_endPos, file); !ec)
        job->m_bld.member("size", job->m_endPos);
    TimePoint mtime;
    if (ec = fileLastWriteTime(&mtime, file); !ec)
        job->m_bld.member("mtime", mtime);
    auto pos = job->m_endPos > size(job->m_buffer)
        ? job->m_endPos - size(job->m_buffer)
        : 0;
    fileRead(job, job->m_buffer, size(job->m_buffer), file, pos);
}

//===========================================================================
bool JsonLogTail::LogJob::onFileRead(
    size_t * bytesUsed,
    const FileReadData & data
) {
    constexpr size_t kMaxLineLen = 256;

    *bytesUsed = data.data.size();
    vector<string_view> lines;
    split(&lines, data.data, "\r\n", kMaxLineLen);
    if (m_found < m_limit) {
        // Searching backwards for start of last "limit" lines.
        m_found += (unsigned) lines.size() - 1;
        if (m_found < m_limit) {
            if (data.offset > 0) {
                // Haven't gone back far enough into the file to find the first
                // line and we're not already at the beginning of the file.
                auto pos = data.offset > (int64_t) size(m_buffer)
                    ? data.offset - size(m_buffer)
                    : 0;
                fileRead(this, m_buffer, size(m_buffer), data.f, pos);
                return false;
            }
            m_limit = m_found;
        }

        // Found the first line we will return (either at the limit or, if
        // there weren't enough lines, the start of the file).
        m_bld.member("lines").array();
        auto i = lines.begin() + (m_found - m_limit);
        auto e = lines.end() - 1;
        assert(i <= e);
        for (; i < e; ++i)
            m_bld.value(*i);
        if (!e->empty())
            m_bld.startValue().value(*e);
        m_limit = m_found - (unsigned) lines.size() + 1;
        if (m_limit) {
            // More to return
            httpRouteReply(m_reqId, move(m_res), true);
            m_res.clear();
            m_found = m_limit;

            // We weren't already in the last block of the file, so re-read
            // the blocks from here to EOF and add them to the reply.
            auto pos = data.offset + data.data.size();
            auto len = m_endPos - pos;
            fileRead(this, m_buffer, size(m_buffer), data.f, pos, len);
            return false;
        }
    } else {
        // Reading forward, replying with lines as we go.
        auto i = lines.begin();
        auto e = lines.end() - 1;
        if (i < e) {
            m_bld.value(*i);
            if (m_bld.state().next == JBuilder::Type::kText)
                m_bld.end();
            while (++i < e)
                m_bld.value(*i);
        }
        if (!e->empty())
            m_bld.startValue().value(*e);
        if (data.more) {
            // More to return
            httpRouteReply(m_reqId, move(m_res.body()), true);
            m_res.clear();
            return true;
        }
    }

    // All lines have been returned
    m_bld.end(); // end array of lines
    m_bld.end(); // end file object
    m_bld.end(); // end web admin object
    if (m_res.headers()) {
        httpRouteReply(m_reqId, move(m_res));
    } else {
        httpRouteReply(m_reqId, move(m_res.body()), false);
    }
    fileClose(data.f);
    delete this;
    return false;
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
        if (!appUsageErrorSignaled()) {
            vector<PerfValue> perfs;
            perfGetValues(&perfs, true);
            for (auto && perf : perfs) {
                if (perf.value != "0") {
                    logMsgDebug() << "PERF " << perf.name << ": "
                        << perf.value;
                }
            }
        }
        return shutdownIncomplete();
    }

    if (!s_log.closeLog())
        return shutdownIncomplete();
}


/****************************************************************************
*
*   Public API
*
***/

static JsonLogFiles s_jsonLogFiles;
static JsonLogTail s_jsonLogTail;

//===========================================================================
void Dim::iLogFileInitialize() {
    shutdownMonitor(&s_cleanup);
    configMonitor("app.xml", &s_appXml);

    Path file;
    if (!appLogPath(&file, "server.log"))
        logMsgFatal() << "Invalid log path: " << file;
    s_log.setFileName(file);
    s_log.setFormatter(&s_defaultFmt);

    logMonitor(&s_log);
}

//===========================================================================
void Dim::iLogFileWebInitialize() {
    if (appFlags().any(fAppWithWebAdmin)) {
        httpRouteAdd({
            .notify = &s_jsonLogFiles,
            .path = "/srv/file/logs.json",
        });
        httpRouteAdd({
            .notify = &s_jsonLogTail,
            .path = "/srv/file/logs/tail/",
            .recurse = true
        });
    }
}

//===========================================================================
LogType Dim::appLogLevel() {
    return s_logLevel;
}
