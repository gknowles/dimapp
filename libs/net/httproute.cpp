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
        function<void(HttpConnHandle h, CharBuf * out, int stream)> fn, 
        bool more
    );
    static void reply(unsigned reqId, HttpResponse & msg, bool more);
    static void reply(unsigned reqId, std::string_view data, bool more);
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


/****************************************************************************
*
*   Helpers
*
***/

//===========================================================================
static IHttpRouteNotify * find(std::string_view path, HttpMethod method) {
    IHttpRouteNotify * best = nullptr;
    size_t bestSegs = 0;
    for (auto && pi : s_paths) {
        if (path == pi.path 
            || pi.recurse && path.compare(0, pi.path.size(), pi.path) == 0
        ) {
            if (pi.segs >= bestSegs) {
                best = pi.notify;
                bestSegs = pi.segs;
            }
        }
    }
    return best;
}

//===========================================================================
static void route(unsigned reqId, HttpRequest & req) {
    auto path = string_view(req.pathAbsolute());
    auto method = httpMethodFromString(req.method());
    auto notify = find(path, method);
    if (!notify)
        return httpRouteReplyNotFound(reqId, req);

    unordered_multimap<string_view, string_view> params;
    notify->onHttpRequest(reqId, params, req);
}

//===========================================================================
static unsigned makeRequestInfo (HttpSocket * conn, int stream) {
    for (;;) {
        auto id = ++s_nextReqId;
        auto & found = s_requests[id];
        if (!found.conn) {
            found = {conn, stream};
            return id;
        }
    }
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
    function<void(HttpConnHandle h, CharBuf * out, int stream)> fn, 
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
    auto fn = [&](HttpConnHandle h, CharBuf * out, int stream) -> void {
        httpReply(h, out, stream, msg, more);
    };
    iReply(reqId, fn, more);
}

//===========================================================================
// static 
void HttpSocket::reply(unsigned reqId, string_view data, bool more) {
    auto fn = [&](HttpConnHandle h, CharBuf * out, int stream) -> void {
        httpData(h, out, stream, data, more);
    };
    iReply(reqId, fn, more);
}

//===========================================================================
// static 
void HttpSocket::reply(unsigned reqId, const CharBuf & data, bool more) {
    auto fn = [&](HttpConnHandle h, CharBuf * out, int stream) -> void {
        httpData(h, out, stream, data, more);
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
    bool result = httpRecv(m_conn, &out, &msgs, data.data, data.bytes);
    if (!out.empty())
        socketWrite(this, out);
    if (!result)
        return socketDisconnect(this);
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
    if (firstTry && !s_paths.empty())
        socketCloseWait<HttpSocket>(s_endpoint, AppSocket::kHttp2);
    if (!s_requests.empty())
        return shutdownIncomplete();
}

//===========================================================================
void ShutdownNotify::onShutdownConsole(bool firstTry) {
}


/****************************************************************************
*
*   Internal API
*
***/

//===========================================================================
void Dim::iHttpRouteInitialize() {
    shutdownMonitor(&s_cleanup);
    socketAddFamily(AppSocket::kHttp2, &s_http2Match);

    s_endpoint = {};
    s_endpoint.port = 41000;
}


/****************************************************************************
*
*   Public API
*
***/

//===========================================================================
void Dim::httpRouteAdd(
    IHttpRouteNotify * notify,
    std::string_view path,
    unsigned methods,
    bool recurse
) {
    assert(!path.empty());
    if (s_paths.empty())
        socketListen<HttpSocket>(s_endpoint, AppSocket::kHttp2);
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
        << start("p") << "Requested URL: " << req.pathAbsolute() << end
        << end
        << end;
    res.addHeader(kHttpContentType, "text/html");
    res.addHeader(kHttp_Status, "404");
    httpRouteReply(reqId, res);
}

//===========================================================================
// ReplyWithFile
//===========================================================================
struct ReplyWithFileNotify : IFileReadNotify {
    unsigned m_reqId{0};
    char m_buffer[8'192];

    bool onFileRead(
        string_view data, 
        int64_t offset, 
        FileHandle f
    ) override {
        HttpSocket::reply(m_reqId, data, true);
        return true;
    }
    void onFileEnd(int64_t offset, FileHandle f) override {
        HttpSocket::reply(m_reqId, string_view(), false);
        fileClose(f);
        delete this;
    }
};

//===========================================================================
void Dim::httpRouteReplyWithFile(unsigned reqId, std::string_view path) {
    HttpResponse msg;
    msg.addHeader(kHttp_Status, "404");
    httpRouteReply(reqId, msg, true);
    auto notify = new ReplyWithFileNotify;
    notify->m_reqId = reqId;
    auto file = fileOpen(path, File::fReadOnly | File::fDenyNone);
    if (!file)
        return notify->onFileEnd(0, file);
    fileRead(
        notify, 
        notify->m_buffer, 
        size(notify->m_buffer), 
        file
    );
}
