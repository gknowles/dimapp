// Copyright Glen Knowles 2017 - 2022.
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
    void onFileWrite(const FileWriteData & data) override;

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
            auto ec = fileOpen(
                &m_file,
                s_logfile,
                File::fCreat | File::fReadWrite | File::fDenyNone
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
void LogBuffer::onFileWrite(const FileWriteData & data) {
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
    void onLog(const LogMsg & log) override;
};
} // namespace
static Logger s_logger;

//===========================================================================
void Logger::onLog(const LogMsg & log) {
    assert(log.type < kLogTypes);
    if (log.type < appLogLevel())
        return;

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
    snprintf(tmp, size(tmp), "<%d>1 %s %s %s %s %s %s %.*s",
        pri,
        Time8601Str{}.set().c_str(),
        s_hostname.c_str(),
        "appName",
        "-",    // procid
        "-",    // msgid
        "-",    // structured data
        (int) log.msg.size(),
        log.msg.data()
    );
    s_buffer.writeLog(tmp, log.type == kLogTypeFatal);
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
    for (auto&& f : FileIter(dir, "*.log")) {
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
    Param & m_limit = param("limit");
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

    auto limit = clamp(m_limit ? strToUint(*m_limit) : 50u, 10u, 10'000u);

    auto job = new LogJob;
    job->m_reqId = reqId;
    job->m_limit = limit;
    job->m_bld = initResponse(&job->m_res, reqId, msg);
    job->m_bld.member("file").object()
        .member("limit", limit)
        .member("name", qpath);

    FileHandle file;
    auto ec = fileOpen(&file, path, File::fReadOnly | File::fDenyNone);
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
        auto i = lines.begin() + m_found - m_limit;
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
                if (perf.value != "0")
                    logMsgInfo() << "perf: " << perf.value << ' ' << perf.name;
            }
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

static JsonLogFiles s_jsonLogFiles;
static JsonLogTail s_jsonLogTail;

//===========================================================================
void Dim::iLogFileInitialize() {
    shutdownMonitor(&s_cleanup);
    configMonitor("app.xml", &s_appXml);

    if (!appLogPath(&s_logfile, "server.log"))
        logMsgFatal() << "Invalid log path: " << s_logfile;

    logMonitor(&s_logger);
}

//===========================================================================
void Dim::iLogFileWebInitialize() {
    if (appFlags().any(fAppWithWebAdmin)) {
        httpRouteAdd({
            .notify = &s_jsonLogFiles, 
            .path = "/srv/log/files.json",
            .name = "Logs",
            .desc = "Server logs files.",
            .renderPath = "/web/srv/log-files.html",
        });
        httpRouteAdd({
            .notify = &s_jsonLogTail, 
            .path = "/srv/log/tail/", 
            .recurse = true
        });
    }
}

//===========================================================================
LogType Dim::appLogLevel() {
    return s_logLevel;
}
