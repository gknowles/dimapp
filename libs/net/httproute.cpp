// Copyright Glen Knowles 2017.
// Distributed under the Boost Software License, Version 1.0.
//
// httproute.cpp - dim net
#include "pch.h"
#pragma hdrstop

using namespace std;
using namespace Dim;


/****************************************************************************
*
*   Tuning parameters
*
***/


/****************************************************************************
*
*   Private declarations
*
***/

namespace {

struct PathInfo {
    IHttpRouteNotify * notify;
    bool recurse;
    string path;
    size_t segs;
    unsigned methods;
};

class HttpSocket : public IAppSocketNotify {
public:
    static void iReply(
        unsigned reqId,
        const function<void(HttpConnHandle h, CharBuf * out, int stream)> & fn,
        bool more
    );
    static void reply(unsigned reqId, HttpResponse & msg, bool more);
    static void reply(unsigned reqId, string_view data, bool more);
    static void reply(unsigned reqId, const CharBuf & data, bool more);

public:
    ~HttpSocket ();
    bool onSocketAccept(const AppSocketInfo & info) override;
    void onSocketDisconnect() override;
    void onSocketRead(AppSocketData & data) override;

private:
    HttpConnHandle m_conn;
    vector<unsigned> m_reqIds;
};

struct RequestInfo {
    HttpSocket * conn;
    int stream;

    RequestInfo(HttpSocket * conn = nullptr, int stream = 0);
    ~RequestInfo();
};

} // namespace


/****************************************************************************
*
*   Variables
*
***/

static vector<PathInfo> s_paths;
static Endpoint s_endpoint;
static unordered_map<unsigned, RequestInfo> s_requests;
static unsigned s_nextReqId;

static auto & s_perfRequests = uperf("http requests");
static auto & s_perfCurrent = uperf("http requests (current)");
static auto & s_perfInvalid = uperf("http protocol error");
static auto & s_perfSuccess = uperf("http reply success");
static auto & s_perfError = uperf("http reply error");


/****************************************************************************
*
*   Helpers
*
***/

//===========================================================================
static IHttpRouteNotify * find(string_view path, HttpMethod method) {
    IHttpRouteNotify * best = nullptr;
    size_t bestSegs = 0;
    size_t bestLen = 0;
    for (auto && pi : s_paths) {
        if (~pi.methods & method)
            continue;
        if (path == pi.path
            || pi.recurse && path.compare(0, pi.path.size(), pi.path) == 0
        ) {
            if (pi.segs > bestSegs
                || pi.segs == bestSegs && pi.path.size() > bestLen
            ) {
                best = pi.notify;
                bestSegs = pi.segs;
                bestLen = pi.path.size();
            }
        }
    }
    return best;
}

//===========================================================================
static void route(unsigned reqId, HttpRequest & req) {
    auto & params = req.query();
    auto method = httpMethodFromString(req.method());
    for (auto && param : params.parameters) {
        if (param.name == "_method") {
            if (auto val = param.values.front())
                method = httpMethodFromString(val->value);
            break;
        }
    }
    auto notify = find(params.path, method);
    if (!notify)
        return httpRouteReplyNotFound(reqId, req);

    notify->onHttpRequest(reqId, req);
}

//===========================================================================
static unsigned makeRequestInfo (HttpSocket * conn, int stream) {
    for (;;) {
        auto id = ++s_nextReqId;
        auto & found = s_requests[id];
        if (!found.conn) {
            found.conn = conn;
            found.stream = stream;
            return id;
        }
    }
}


/****************************************************************************
*
*   RequestInfo
*
***/

//===========================================================================
RequestInfo::RequestInfo (HttpSocket * conn, int stream)
    : conn(conn)
    , stream(stream)
{
    s_perfRequests += 1;
    s_perfCurrent += 1;
}

//===========================================================================
RequestInfo::~RequestInfo () {
    s_perfCurrent -= 1;
}


/****************************************************************************
*
*   HttpSocket
*
***/

//===========================================================================
// static
void HttpSocket::iReply(
    unsigned reqId,
    const function<void(HttpConnHandle h, CharBuf * out, int stream)> & fn,
    bool more
) {
    auto it = s_requests.find(reqId);
    if (it == s_requests.end())
        return;
    auto conn = it->second.conn;
    CharBuf out;
    fn(conn->m_conn, &out, it->second.stream);
    socketWrite(conn, out);
    if (!more) {
        s_requests.erase(it);
        auto & ids = conn->m_reqIds;
        for (auto & id : ids) {
            if (reqId < id)
                continue;
            if (reqId == id)
                ids.erase(ids.begin() + (&id - ids.data()));
            break;
        }
    }
}

//===========================================================================
// static
void HttpSocket::reply(unsigned reqId, HttpResponse & msg, bool more) {
    if (msg.status() == 200) {
        s_perfSuccess += 1;
    } else {
        s_perfError += 1;
    }
    auto fn = [&](HttpConnHandle h, CharBuf * out, int stream) -> void {
        httpReply(out, h, stream, msg, more);
    };
    iReply(reqId, fn, more);
}

//===========================================================================
// static
void HttpSocket::reply(unsigned reqId, string_view data, bool more) {
    auto fn = [&](HttpConnHandle h, CharBuf * out, int stream) -> void {
        httpData(out, h, stream, data, more);
    };
    iReply(reqId, fn, more);
}

//===========================================================================
// static
void HttpSocket::reply(unsigned reqId, const CharBuf & data, bool more) {
    auto fn = [&](HttpConnHandle h, CharBuf * out, int stream) -> void {
        httpData(out, h, stream, data, more);
    };
    iReply(reqId, fn, more);
}

//===========================================================================
HttpSocket::~HttpSocket () {
    for (auto && id : m_reqIds)
        s_requests.erase(id);
}

//===========================================================================
bool HttpSocket::onSocketAccept(const AppSocketInfo & info) {
    m_conn = httpAccept();
    return true;
}

//===========================================================================
void HttpSocket::onSocketDisconnect() {
    httpClose(m_conn);
    m_conn = {};
}

//===========================================================================
void HttpSocket::onSocketRead(AppSocketData & data) {
    CharBuf out;
    vector<unique_ptr<HttpMsg>> msgs;
    bool result = httpRecv(&out, &msgs, m_conn, data.data, data.bytes);
    if (!out.empty())
        socketWrite(this, out);
    if (!result) {
        s_perfInvalid += 1;
        return socketDisconnect(this);
    }
    for (auto && msg : msgs) {
        if (msg->isRequest()) {
            auto id = makeRequestInfo(this, msg->stream());
            if (!m_reqIds.empty() && id < m_reqIds.back()) {
                auto ub = upper_bound(m_reqIds.begin(), m_reqIds.end(), id);
                m_reqIds.insert(ub, id);
            } else {
                m_reqIds.push_back(id);
            }
            auto req = static_cast<HttpRequest *>(msg.get());
            route(id, *req);
        } else {
        }
    }
}


/****************************************************************************
*
*   Http2Match
*
***/

namespace {
class Http2Match : public IAppSocketMatchNotify {
    AppSocket::MatchType OnMatch(
        AppSocket::Family fam,
        string_view view) override;
};
static Http2Match s_http2Match;
} // namespace

//===========================================================================
AppSocket::MatchType Http2Match::OnMatch(
    AppSocket::Family fam,
    string_view view
) {
    assert(fam == AppSocket::kHttp2);
    const char kPrefaceData[] = "PRI * HTTP/2.0\r\n\r\nSM\r\n\r\n";
    const size_t kPrefaceDataLen = size(kPrefaceData) - 1;
    size_t num = min(kPrefaceDataLen, view.size());
    if (view.compare(0, num, kPrefaceData, num) != 0)
        return AppSocket::kUnsupported;
    if (num == kPrefaceDataLen)
        return AppSocket::kPreferred;

    return AppSocket::kUnknown;
}


/****************************************************************************
*
*   Shutdown monitor
*
***/

namespace {
class ShutdownNotify : public IShutdownNotify {
    void onShutdownClient(bool firstTry) override;
    void onShutdownConsole(bool firstTry) override;
};
} // namespace
static ShutdownNotify s_cleanup;

//===========================================================================
void ShutdownNotify::onShutdownClient(bool firstTry) {
    //if (firstTry && !s_paths.empty())
    //    socketCloseWait<HttpSocket>(s_endpoint, AppSocket::kHttp2);
}

//===========================================================================
void ShutdownNotify::onShutdownConsole(bool firstTry) {
    if (!s_requests.empty())
        return shutdownIncomplete();
}


/****************************************************************************
*
*   Internal API
*
***/

//===========================================================================
static void startListen() {
    sockMgrListen<HttpSocket>(
        "httpRoute",
        AppSocket::kHttp2,
        AppSocket::fMgrConsole
    );
}

//===========================================================================
void Dim::iHttpRouteInitialize() {
    shutdownMonitor(&s_cleanup);
    socketAddFamily(AppSocket::kHttp2, &s_http2Match);
    if (!s_paths.empty())
        startListen();
}


/****************************************************************************
*
*   Public API
*
***/

//===========================================================================
void Dim::httpRouteAdd(
    IHttpRouteNotify * notify,
    string_view path,
    unsigned methods,
    bool recurse
) {
    assert(!path.empty());
    if (s_paths.empty() && !appStarting())
        startListen();
    PathInfo pi;
    pi.notify = notify;
    pi.recurse = recurse;
    pi.path = path;
    pi.segs = count(path.begin(), path.end(), '/');
    pi.methods = methods;
    s_paths.push_back(pi);
}

//===========================================================================
void Dim::httpRouteReply(unsigned reqId, HttpResponse & msg, bool more) {
    HttpSocket::reply(reqId, msg, more);
}

//===========================================================================
void Dim::httpRouteReply(unsigned reqId, const CharBuf & data, bool more) {
    HttpSocket::reply(reqId, data, more);
}

//===========================================================================
void Dim::httpRouteReply(unsigned reqId, string_view data, bool more) {
    HttpSocket::reply(reqId, data, more);
}

//===========================================================================
void Dim::httpRouteReplyNotFound(unsigned reqId, const HttpRequest & req) {
    HttpResponse res;
    XBuilder bld(res.body());
    bld << start("html")
        << start("head") << elem("title", "404 Not Found") << end
        << start("body")
        << elem("h1", "404 Not Found")
        << start("p") << "Requested URL: " << req.pathRaw() << end
        << end
        << end;
    res.addHeader(kHttpContentType, "text/html");
    res.addHeader(kHttp_Status, "404");
    httpRouteReply(reqId, res);
}

//===========================================================================
void Dim::httpRouteReply(
    unsigned reqId,
    const HttpRequest & req,
    unsigned status,
    const string & msg
) {
    StrFrom<unsigned> st{status};
    HttpResponse res;
    XBuilder bld(res.body());
    bld.start("html")
        .start("head").elem("title", st.c_str()).end()
        .start("body")
            .elem("h1", st.c_str())
            .start("dl")
                .elem("dt", "Description")
                .elem("dd", msg.c_str())
                .elem("dt", "Requested URL: ")
                .elem("dd", req.pathRaw())
            .end()
        .end()
        .end();

    res.addHeader(kHttpContentType, "text/html");
    res.addHeader(kHttp_Status, st.c_str());
    httpRouteReply(reqId, res);
}


//===========================================================================
// ReplyWithFile
//===========================================================================
namespace {

struct ReplyWithFileNotify : IFileReadNotify {
    unsigned m_reqId{0};
    char m_buffer[8'192];

    bool onFileRead(
        size_t * bytesUsed,
        string_view data,
        int64_t offset,
        FileHandle f
    ) override {
        *bytesUsed = data.size();
        HttpSocket::reply(m_reqId, data, true);
        return true;
    }
    void onFileEnd(int64_t offset, FileHandle f) override {
        HttpSocket::reply(m_reqId, string_view(), false);
        fileClose(f);
        delete this;
    }
};

} // namespace

//===========================================================================
void Dim::httpRouteReplyWithFile(unsigned reqId, string_view path) {
    HttpResponse msg;
    auto file = fileOpen(path, File::fReadOnly | File::fDenyNone);
    if (!file) {
        msg.addHeader(kHttp_Status, "404");
        return HttpSocket::reply(reqId, msg, false);
    }
    msg.addHeader(kHttp_Status, "200");
    httpRouteReply(reqId, msg, true);
    auto notify = new ReplyWithFileNotify;
    notify->m_reqId = reqId;
    fileRead(
        notify,
        notify->m_buffer,
        size(notify->m_buffer),
        file
    );
}
