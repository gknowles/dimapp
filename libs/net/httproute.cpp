// Copyright Glen Knowles 2017 - 2018.
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
    string_view path;
    bool recurse{};
    HttpMethod methods{};
    size_t segs{};
    unsigned matched{};
    IHttpRouteNotify * notify{};

    TimePoint mtime;
    string_view content;
    string_view mimeType;
    string_view charSet;

    // Internal data referenced by other members
    unique_ptr<char[]> data;
    unique_ptr<IHttpRouteNotify> notifyOwned;
};

class HttpSocket : public IAppSocketNotify {
public:
    static void iReply(
        unsigned reqId,
        const function<void(HttpConnHandle h, CharBuf * out, int stream)> & fn,
        bool more
    );
    static void reply(unsigned reqId, HttpResponse && msg, bool more);
    static void reply(unsigned reqId, string_view data, bool more);
    static void reply(unsigned reqId, CharBuf && data, bool more);
    static void resetReply(unsigned reqId, bool internal);

public:
    ~HttpSocket ();
    bool onSocketAccept(const AppSocketInfo & info) override;
    void onSocketDisconnect() override;
    bool onSocketRead(AppSocketData & data) override;

private:
    HttpConnHandle m_conn;
    vector<unsigned> m_reqIds;
};

struct RequestInfo {
    HttpSocket * conn {nullptr};
    int stream {0};
    PathInfo * pi {nullptr};

    RequestInfo();
    ~RequestInfo();
};

} // namespace


/****************************************************************************
*
*   Variables
*
***/

static vector<PathInfo> s_paths;
static unordered_map<unsigned, RequestInfo> s_requests;
static unsigned s_nextReqId;

static auto & s_perfRequests = uperf("http.requests");
static auto & s_perfCurrent = uperf("http.requests (current)");
static auto & s_perfInvalid = uperf("http.protocol error");
static auto & s_perfSuccess = uperf("http.reply success");
static auto & s_perfError = uperf("http.reply error");
static auto & s_perfReset = uperf("http.reply canceled");
static auto & s_perfRejects = uperf("http.http1 requests rejected");


/****************************************************************************
*
*   Helpers
*
***/

//===========================================================================
static PathInfo * find(string_view path, HttpMethod method) {
    PathInfo * best = nullptr;
    for (auto && pi : s_paths) {
        if (~pi.methods & method)
            continue;
        if (path == pi.path
            || pi.recurse && path.compare(0, pi.path.size(), pi.path) == 0
        ) {
            if (!best
                || pi.segs > best->segs
                || pi.segs == best->segs && pi.path.size() > best->path.size()
                || pi.segs == best->segs
                    && pi.path == best->path
                    && pi.recurse < best->recurse
            ) {
                best = &pi;
            }
        }
    }
    return best;
}

//===========================================================================
static void route(RequestInfo * ri, unsigned reqId, HttpRequest & req) {
    auto & params = req.query();
    auto method = httpMethodFromString(req.method());
    for (auto && param : params.parameters) {
        if (param.name == "_method") {
            if (auto val = param.values.front())
                method = httpMethodFromString(val->value);
            break;
        }
    }
    auto pi = find(params.path, method);
    if (!pi)
        return httpRouteReplyNotFound(reqId, req);
    ri->pi = pi;
    pi->matched += 1;
    if (pi->notify)
        return pi->notify->onHttpRequest(reqId, req);

    httpRouteReplyWithFile(
        reqId,
        pi->mtime,
        pi->content,
        pi->mimeType,
        pi->charSet
    );
}

//===========================================================================
static struct { unsigned id; RequestInfo * ri; } makeRequestInfo(
    HttpSocket * conn,
    int stream
) {
    for (;;) {
        auto id = ++s_nextReqId;
        auto & found = s_requests[id];
        if (!found.conn) {
            found.conn = conn;
            found.stream = stream;
            return {id, &found};
        }
    }
}


/****************************************************************************
*
*   Default reply headers
*
***/

namespace {

struct DefaultHeader {
    HttpHdr id;
    string name;
    string value;
};

} // namespace

static vector<DefaultHeader> s_defaultHeaders;

//===========================================================================
static void setDefaultReplyHeader(const DefaultHeader & val) {
    auto ii = equal_range(
        s_defaultHeaders.begin(),
        s_defaultHeaders.end(),
        val,
        [](auto & a, auto & b) {
            return a.id < b.id || a.id == b.id && a.name < b.name;
        }
    );
    if (!val.value.empty()) {
        if (ii.first == ii.second) {
            s_defaultHeaders.insert(ii.first, val);
        } else {
            ii.first->value = val.value;
        }
    } if (ii.first != ii.second) {
        s_defaultHeaders.erase(ii.first);
    }
}

//===========================================================================
static void addDefaultHeaders(HttpResponse & msg) {
    for (auto && h : s_defaultHeaders) {
        if (h.id) {
            if (!msg.hasHeader(h.id))
                msg.addHeader(h.id, h.value.c_str());
        } else {
            if (!msg.hasHeader(h.name.c_str()))
                msg.addHeader(h.name.c_str(), h.value.c_str());
        }
    }
}


/****************************************************************************
*
*   RequestInfo
*
***/

//===========================================================================
RequestInfo::RequestInfo() {
    s_perfRequests += 1;
    s_perfCurrent += 1;
}

//===========================================================================
RequestInfo::~RequestInfo() {
    s_perfCurrent -= 1;
}


/****************************************************************************
*
*   ReplyTask
*
***/

namespace {

template<typename T>
struct ReplyTask : ITaskNotify {
    ReplyTask(unsigned reqId, bool more);
    void onTask() override;

    function<void(HttpConnHandle h, CharBuf * out, int stream)> fn;
    unsigned reqId;
    T data;
    bool more;
};

} // namespace

//===========================================================================
template<typename T>
ReplyTask<T>::ReplyTask(unsigned reqId, bool more)
    : reqId{reqId}
    , more{more}
{}

//===========================================================================
template<typename T>
void ReplyTask<T>::onTask() {
    HttpSocket::iReply(reqId, fn, more);
    delete this;
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
void HttpSocket::reply(unsigned reqId, HttpResponse && msg, bool more) {
    if (msg.status() == 200) {
        s_perfSuccess += 1;
    } else {
        s_perfError += 1;
    }
    addDefaultHeaders(msg);
    if (!msg.hasHeader(kHttpDate)) {
        auto now = timeNow();
        msg.addHeader(kHttpDate, now);
    }

    if (taskInEventThread()) {
        auto fn = [&](HttpConnHandle h, CharBuf * out, int stream) {
            httpReply(out, h, stream, msg, more);
        };
        return iReply(reqId, fn, more);
    }

    auto task = new ReplyTask<HttpResponse>(reqId, more);
    task->data = move(msg);
    task->fn = [task](HttpConnHandle h, CharBuf * out, int stream) {
        httpReply(out, h, stream, task->data, task->more);
    };
    taskPushEvent(task);
}

//===========================================================================
// static
void HttpSocket::reply(unsigned reqId, string_view data, bool more) {
    if (taskInEventThread()) {
        auto fn = [&](HttpConnHandle h, CharBuf * out, int stream) {
            httpData(out, h, stream, data, more);
        };
        return iReply(reqId, fn, more);
    }

    auto task = new ReplyTask<string>(reqId, more);
    task->data = data;
    task->fn = [task](HttpConnHandle h, CharBuf * out, int stream) {
        httpData(out, h, stream, task->data, task->more);
    };
    taskPushEvent(task);
}

//===========================================================================
// static
void HttpSocket::reply(unsigned reqId, CharBuf && data, bool more) {
    if (taskInEventThread()) {
        auto fn = [&](HttpConnHandle h, CharBuf * out, int stream) -> void {
            httpData(out, h, stream, data, more);
        };
        return iReply(reqId, fn, more);
    }

    auto task = new ReplyTask<CharBuf>{reqId, more};
    task->data = move(data);
    task->fn = [task](HttpConnHandle h, CharBuf * out, int stream) -> void {
        httpData(out, h, stream, task->data, task->more);
    };
    taskPushEvent(task);
}

//===========================================================================
// static
void HttpSocket::resetReply(unsigned reqId, bool internal) {
    s_perfReset += 1;
    auto fn = [&](HttpConnHandle h, CharBuf * out, int stream) -> void {
        httpResetStream(out, h, stream, internal);
    };
    iReply(reqId, fn, false);
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
bool HttpSocket::onSocketRead(AppSocketData & data) {
    CharBuf out;
    vector<unique_ptr<HttpMsg>> msgs;
    bool result = httpRecv(&out, &msgs, m_conn, data.data, data.bytes);
    if (!out.empty())
        socketWrite(this, out);
    if (!result) {
        s_perfInvalid += 1;
        auto em = httpGetError(m_conn);
        if (em.empty())
            em = "no error";
        logMsgDebug() << "HttpSocket: " << em;
        logHexDebug(string_view{data.data, (size_t) data.bytes}.substr(0, 128));
        socketDisconnect(this);
        return true;
    }
    for (auto && msg : msgs) {
        if (msg->isRequest()) {
            auto [id, ri] = makeRequestInfo(this, msg->stream());
            if (!m_reqIds.empty() && id < m_reqIds.back()) {
                auto ub = upper_bound(m_reqIds.begin(), m_reqIds.end(), id);
                m_reqIds.insert(ub, id);
            } else {
                m_reqIds.push_back(id);
            }
            auto req = static_cast<HttpRequest *>(msg.get());
            route(ri, id, *req);
        } else {
        }
    }
    return true;
}


/****************************************************************************
*
*   Http2Match
*
***/

namespace {
class Http2Match : public IAppSocketMatchNotify {
    AppSocket::MatchType onMatch(
        AppSocket::Family fam,
        string_view view
    ) override;
};
static Http2Match s_http2Match;
} // namespace

//===========================================================================
AppSocket::MatchType Http2Match::onMatch(
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
*   Http1 Rejection
*
***/

namespace {

class Http1Reject : public IAppSocketMatchNotify, public IAppSocketNotify {
    AppSocket::MatchType onMatch(
        AppSocket::Family fam,
        string_view view
    ) override;

    bool onSocketAccept(const AppSocketInfo & info) override;
    bool onSocketRead(AppSocketData & data) override;

private:
    AppSocketInfo m_info;
};

} // namespace

static Http1Reject s_http1Match;

//===========================================================================
AppSocket::MatchType Http1Reject::onMatch(
    AppSocket::Family fam,
    string_view view
) {
    assert(fam == AppSocket::kHttp1);
    auto ptr = view.data();
    auto eptr = ptr + view.size();
    while (isupper(*ptr)) {
        if (++ptr == eptr)
            return AppSocket::kUnknown;
    }
    if (*ptr != ' ')
        return AppSocket::kUnsupported;
    if (++ptr == eptr)
        return AppSocket::kUnknown;
    while (*ptr != ' ') {
        if (!isprint(*ptr))
            return AppSocket::kUnsupported;
        if (++ptr == eptr)
            return AppSocket::kUnknown;
    }
    ptr += 1;
    const char kVersion[] = "HTTP/1.";
    const size_t kVersionLen = size(kVersion) - 1;
    size_t num = min(kVersionLen, size_t(eptr - ptr));
    if (memcmp(ptr, kVersion, num) != 0)
        return AppSocket::kUnsupported;
    if (num == kVersionLen)
        return AppSocket::kPreferred;

    return AppSocket::kUnknown;
}

//===========================================================================
bool Http1Reject::onSocketAccept(const AppSocketInfo & info) {
    m_info = info;
    return true;
}

//===========================================================================
bool Http1Reject::onSocketRead(AppSocketData & data) {
    s_perfRejects += 1;

    const char kBody[] = R"(
<html><body>
<h1>505 Version Not Supported</h1>
<p>This service requires use of the HTTP/2 protocol</p>
</body></html>
)";
    const auto kBodyLen = size(kBody) - 1;
    ostringstream os;
    os << "HTTP/1.1 505 Version Not Supported\r\n"
        "Content-Length: " << kBodyLen << "\r\n"
        "Content-Type: text/html\r\n"
        "\r\n"
        << kBody;
    socketWrite(this, os.str());
    return true;
}


/****************************************************************************
*
*   MimeType
*
***/

static MimeType s_mimeTypes[] = {
    { ".css",  "text/css"                         },
    { ".html", "text/html"                        },
    { ".js",   "application/javascript"           },
    { ".svg",  "image/svg+xml"                    },
    { ".tsd",  "application/octet-stream"         },
    { ".tsl",  "application/octet-stream"         },
    { ".tsw",  "application/octet-stream"         },
    { ".xml",  "application/xml",         "utf-8" },
    { ".xsl",  "application/xslt+xml",    "utf-8" },
};

namespace {
struct AutoInit {
    AutoInit() {
        sort(begin(s_mimeTypes), end(s_mimeTypes), compareExt);
    }
};
static AutoInit s_init;
} // namespace

//===========================================================================
int Dim::compareExt(const MimeType & a, const MimeType & b) {
    if (!a.fileExt.empty() && !b.fileExt.empty())
        return a.fileExt.compare(b.fileExt);
    return !a.fileExt.empty() ? -1 : !b.fileExt.empty() ? 1 : 0;
}

//===========================================================================
MimeType Dim::mimeTypeDefault(string_view path) {
    Path tmp{path};
    auto ext = tmp.extension();
    auto mt = MimeType{ext.data()};
    auto ii = equal_range(
        ::begin(s_mimeTypes),
        ::end(s_mimeTypes),
        mt,
        compareExt
    );
    if (ii.first == ii.second)
        return {};
    return *ii.first;
}


/****************************************************************************
*
*   Shutdown monitor
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
    if (!s_requests.empty())
        return shutdownIncomplete();
    s_paths.clear();
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
    sockMgrListen<Http1Reject>("http1Reject", AppSocket::kHttp1);
}

//===========================================================================
void Dim::iHttpRouteInitialize() {
    shutdownMonitor(&s_cleanup);
    socketAddFamily(AppSocket::kHttp2, &s_http2Match);
    socketAddFamily(AppSocket::kHttp1, &s_http1Match);
    if (!s_paths.empty())
        startListen();
}


/****************************************************************************
*
*   IHttpRouteNotify
*
***/

//===========================================================================
void IHttpRouteNotify::mapParams(const HttpRequest & msg) {
    if (m_params.empty())
        return;
    if (!m_paramTbl) {
        vector<TokenTable::Token> tokens;
        for (auto i = 0; i < m_params.size(); ++i) {
            auto & tok = tokens.emplace_back();
            tok.id = i;
            tok.name = m_params[i]->m_name.c_str();
        }
        m_paramTbl = TokenTable(tokens);
    }
    for (auto && rp : m_params)
        rp->reset();
    int id;
    for (auto && mp : msg.query().parameters) {
        if (m_paramTbl.find(&id, mp.name)) {
            auto & rp = *m_params[id];
            for (auto && v : mp.values)
                rp.append(v.value);
        }
    }
}

//===========================================================================
IHttpRouteNotify::Param & IHttpRouteNotify::param(string name) {
    m_params.push_back(make_unique<Param>(name));
    return static_cast<Param &>(*m_params.back());
}

//===========================================================================
IHttpRouteNotify::ParamVec & IHttpRouteNotify::paramVec(string name) {
    m_params.push_back(make_unique<ParamVec>(name));
    return static_cast<ParamVec &>(*m_params.back());
}


/****************************************************************************
*
*   HttpRouteRedirectNotify
*
***/

//===========================================================================
HttpRouteRedirectNotify::HttpRouteRedirectNotify(
    string_view location,
    unsigned status,
    string_view msg
)
    : m_location(location)
    , m_status(status)
    , m_msg(msg)
{}

//===========================================================================
void HttpRouteRedirectNotify::onHttpRequest(
    unsigned reqId,
    HttpRequest & msg
) {
    httpRouteReplyRedirect(reqId, m_location, m_status, m_msg);
}


/****************************************************************************
*
*   HttpRouteDirListNotify
*
***/

//===========================================================================
HttpRouteDirListNotify::HttpRouteDirListNotify(string_view path)
    : m_path(path)
{}

//===========================================================================
HttpRouteDirListNotify & HttpRouteDirListNotify::set(string_view path) {
    m_path = path;
    return *this;
}

//===========================================================================
void HttpRouteDirListNotify::onHttpRequest(
    unsigned reqId,
    HttpRequest & msg
) {
    httpRouteReplyDirList(reqId, msg, m_path);
}


/****************************************************************************
*
*   Add routes
*
***/

//===========================================================================
static void routeAdd(PathInfo && pi) {
    assert(!pi.path.empty());

    if (!taskInEventThread()) {
        struct RouteAddTask : ITaskNotify {
            PathInfo m_pi;

            void onTask() override {
                routeAdd(move(m_pi));
                delete this;
            }
        };
        auto fn = new RouteAddTask;
        fn->m_pi = move(pi);
        taskPushEvent(fn);
        return;
    }

    if (s_paths.empty() && !appStarting())
        startListen();
    s_paths.emplace_back(move(pi));
}

//===========================================================================
void Dim::httpRouteAdd(
    IHttpRouteNotify * notify,
    string_view path,
    HttpMethod methods,
    bool recurse
) {
    auto pi = PathInfo();
    pi.data = strDup(path);

    pi.notify = notify;
    pi.recurse = recurse;
    pi.path = pi.data.get();
    pi.segs = count(path.begin(), path.end(), '/');
    pi.methods = methods;
    routeAdd(move(pi));
}

//===========================================================================
static void addFileRouteRefs(
    PathInfo && pi,
    string_view path,
    TimePoint mtime,
    string_view content,
    string_view mimeType,
    string_view charSet
) {
    if (pi.mimeType.empty()) {
        auto mt = mimeTypeDefault(path);
        mimeType = mt.type;
        charSet = mt.charSet;
    }

    pi.path = path;
    pi.segs = count(path.begin(), path.end(), '/');
    pi.methods = fHttpMethodGet;
    pi.mtime = mtime;
    pi.content = content;
    pi.mimeType = mimeType;
    pi.charSet = charSet;
    routeAdd(move(pi));
}

//===========================================================================
void Dim::httpRouteAddFile(
    string_view path,
    TimePoint mtime,
    string_view content,
    string_view mimeType,
    string_view charSet
) {
    auto pi = PathInfo();
    string_view * views[] = { &path, &content, &mimeType, &charSet };
    size_t len = accumulate(
        begin(views),
        end(views),
        (size_t) 0,
        [](auto & a, auto & b) { return a + b->size() + 1; }
    );
    pi.data = make_unique<char[]>(len);
    auto out = pi.data.get();
    for (auto && v : views) {
        auto vlen = v->size();
        memcpy(out, v->data(), vlen);
        *v = {out, vlen};
        out += vlen;
        *out++ = 0;
    }

    addFileRouteRefs(move(pi), path, mtime, content, mimeType, charSet);
}

//===========================================================================
void Dim::httpRouteAddFileRef(
    string_view path,
    TimePoint mtime,
    string_view content,
    string_view mimeType,
    string_view charSet
) {
    addFileRouteRefs({}, path, mtime, content, mimeType, charSet);
}


/****************************************************************************
*
*   Query information about requests
*
***/

//===========================================================================
HttpRouteInfo Dim::httpRouteGetInfo(unsigned reqId) {
    assert(taskInEventThread());
    HttpRouteInfo ri = {};
    auto it = s_requests.find(reqId);
    if (it != s_requests.end()) {
        auto pi = it->second.pi;
        ri.path = pi->path;
        ri.methods = pi->methods;
        ri.recurse = pi->recurse;
    }
    return ri;
}


/****************************************************************************
*
*   Reply
*
***/

//===========================================================================
void Dim::httpRouteReply(unsigned reqId, HttpResponse && msg, bool more) {
    HttpSocket::reply(reqId, move(msg), more);
}

//===========================================================================
void Dim::httpRouteReply(unsigned reqId, CharBuf && data, bool more) {
    HttpSocket::reply(reqId, move(data), more);
}

//===========================================================================
void Dim::httpRouteReply(unsigned reqId, string_view view, bool more) {
    HttpSocket::reply(reqId, view, more);
}

//===========================================================================
static void routeReset(unsigned reqId, bool internal) {
    if (taskInEventThread())
        return HttpSocket::resetReply(reqId, internal);

    struct Task : ITaskNotify {
        unsigned m_reqId;
        bool m_internal;
        void onTask() override {
            HttpSocket::resetReply(m_reqId, m_internal);
            delete this;
        }
    };
    auto task = new Task;
    task->m_reqId = reqId;
    task->m_internal = internal;
    taskPushEvent(task);
}

//===========================================================================
void Dim::httpRouteCancel(unsigned reqId) {
    routeReset(reqId, false);
}

//===========================================================================
void Dim::httpRouteInternalError(unsigned reqId) {
    routeReset(reqId, true);
}

//===========================================================================
void Dim::httpRouteReplyNotFound(unsigned reqId, const HttpRequest & req) {
    HttpResponse res;
    XBuilder bld(&res.body());
    bld << start("html")
        << start("head") << elem("title", "404 Not Found") << end
        << start("body")
        << elem("h1", "404 Not Found")
        << start("p") << "Requested URL: " << req.pathRaw() << end
        << end
        << end;
    res.addHeader(kHttpContentType, "text/html");
    res.addHeader(kHttp_Status, "404");
    httpRouteReply(reqId, move(res));
}

//===========================================================================
static void makeReply(
    HttpResponse * out,
    const char url[],
    unsigned status,
    std::string_view msg
) {
    StrFrom<unsigned> st{status};
    XBuilder bld(&out->body());
    bld.start("html")
        .start("head").elem("title", st.c_str()).end()
        .start("body");
    bld.elem("h1", st.c_str());
    bld.start("dl")
        .elem("dt", "Description")
        .elem("dd", string(msg).c_str());
    if (url) {
        bld.elem("dt", "Requested URL: ")
            .elem("dd", url);
    }
    bld.end().end().end();

    out->addHeader(kHttpContentType, "text/html");
    out->addHeader(kHttp_Status, st.c_str());
}

//===========================================================================
static void routeReply(
    unsigned reqId,
    const char url[],
    unsigned status,
    std::string_view msg
) {
    HttpResponse res;
    makeReply(&res, url, status, msg);
    httpRouteReply(reqId, move(res));
}

//===========================================================================
void Dim::httpRouteReply(
    unsigned reqId,
    const HttpRequest & req,
    unsigned status,
    std::string_view msg
) {
    routeReply(reqId, req.pathRaw(), status, msg);
}

//===========================================================================
void Dim::httpRouteReply(
    unsigned reqId,
    unsigned status,
    std::string_view msg
) {
    routeReply(reqId, nullptr, status, msg);
}

//===========================================================================
void Dim::httpRouteReplyRedirect(
    unsigned reqId,
    string_view location,
    unsigned status,
    string_view msg
) {
    HttpResponse out;
    makeReply(&out, nullptr, status, msg);
    out.addHeader(kHttpLocation, location);
    httpRouteReply(reqId, move(out));
}

//===========================================================================
void Dim::httpRouteReplyDirList(
    unsigned reqId,
    const HttpRequest & msg,
    string_view path
) {
    auto now = timeNow();
    HttpResponse res;
    JBuilder bld(&res.body());
    bld.object();
    bld.member("now", now);
    bld.member("files").array();
    for (auto && f : FileIter(path)) {
        auto rname = f.path.view();
        rname.remove_prefix(path.size() + 1);
        bld.object();
        bld.member("name", rname);
        bld.member("mtime", f.mtime);
        bld.member("size", fileSize(f.path));
        bld.end();
    }
    bld.end();
    bld.end();
    res.addHeader(kHttpContentType, "application/json");
    res.addHeader(kHttp_Status, "200");
    httpRouteReply(reqId, move(res));
}

//===========================================================================
// AddDefaultReplyHeader
//===========================================================================
void Dim::httpRouteSetDefaultReplyHeader(HttpHdr hdr, const char value[]) {
    if (hdr == kHttpInvalid)
        return;
    auto key = DefaultHeader{hdr, {}, value ? value : ""};
    setDefaultReplyHeader(key);
}

//===========================================================================
void Dim::httpRouteSetDefaultReplyHeader(
    const char name[],
    const char value[]
) {
    if (auto hdr = httpHdrFromString(name))
        return httpRouteSetDefaultReplyHeader(hdr, value);
    auto key = DefaultHeader{ {}, name, value ? value : "" };
    setDefaultReplyHeader(key);
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
        bool more,
        int64_t offset,
        FileHandle f
    ) override {
        *bytesUsed = data.size();
        HttpSocket::reply(m_reqId, data, more);
        if (!more) {
            fileClose(f);
            delete this;
        }
        return true;
    }
};

} // namespace

//===========================================================================
static void addFileHeaders(
    HttpResponse * msg,
    TimePoint mtime,
    string_view mimeType,
    string_view charSet
) {
    msg->addHeader(kHttp_Status, "200");
    msg->addHeader(kHttpLastModified, mtime);
    if (!mimeType.empty()) {
        auto val = string{mimeType};
        if (!charSet.empty()) {
            val += ";charset=";
            val += charSet;
        }
        msg->addHeader(kHttpContentType, val.c_str());
    }
}

//===========================================================================
void Dim::httpRouteReplyWithFile(unsigned reqId, string_view path) {
    auto file = fileOpen(path, File::fReadOnly | File::fDenyNone);
    if (!file) {
        HttpResponse msg;
        msg.addHeader(kHttp_Status, "404");
        return httpRouteReply(reqId, move(msg), false);
    }
    return httpRouteReplyWithFile(reqId, file);
}

//===========================================================================
void Dim::httpRouteReplyWithFile(unsigned reqId, FileHandle file) {
    HttpResponse msg;
    MimeType mt = mimeTypeDefault(filePath(file));

    auto mtime = fileLastWriteTime(file);
    addFileHeaders(&msg, mtime, mt.type, mt.charSet);
    httpRouteReply(reqId, move(msg), true);
    auto notify = new ReplyWithFileNotify;
    notify->m_reqId = reqId;
    fileRead(
        notify,
        notify->m_buffer,
        size(notify->m_buffer),
        file
    );
}

//===========================================================================
void Dim::httpRouteReplyWithFile(
    unsigned reqId,
    TimePoint mtime,
    string_view content,
    string_view mimeType,
    string_view charSet
) {
    HttpResponse msg;
    addFileHeaders(&msg, mtime, mimeType, charSet);
    msg.body() = content;
    httpRouteReply(reqId, move(msg));
}


/****************************************************************************
*
*   Debugging
*
***/

//===========================================================================
void Dim::httpRouteGetRoutes(IJBuilder * out) {
    assert(taskInEventThread());
    out->array();
    for (auto && p : s_paths) {
        out->object();
        out->member("path");
        if (p.recurse) {
            auto path = string(p.path) + "...";
            out->value(path);
        } else {
            out->value(p.path);
        }
        out->member("methods");
        out->array();
        for (auto mname : to_views(p.methods))
            out->value(mname);
        out->end();
        out->member("matched", p.matched);
        out->end();
    }
    out->end();
}
