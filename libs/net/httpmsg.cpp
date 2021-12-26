// Copyright Glen Knowles 2016 - 2021.
// Distributed under the Boost Software License, Version 1.0.
//
// httpmsg.cpp - dim http
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
*   Declarations
*
***/


/****************************************************************************
*
*   Private
*
***/

namespace {

const TokenTable::Token s_hdrNames[] = {
    {kHttpInvalid, "INVALID"},
    {kHttp_Authority, ":authority"},
    {kHttp_Method, ":method"},
    {kHttp_Path, ":path"},
    {kHttp_Scheme, ":scheme"},
    {kHttp_Status, ":status"},
    {kHttpAccept, "accept"},
    {kHttpAcceptCharset, "accept-charset"},
    {kHttpAcceptEncoding, "accept-encoding"},
    {kHttpAcceptLanguage, "accept-language"},
    {kHttpAcceptRanges, "accept-ranges"},
    {kHttpAccessControlAllowOrigin, "access-control-allow-origin"},
    {kHttpAge, "age"},
    {kHttpAllow, "allow"},
    {kHttpAuthorization, "authorization"},
    {kHttpCacheControl, "cache-control"},
    {kHttpConnection, "connection"},
    {kHttpContentDisposition, "content-disposition"},
    {kHttpContentEncoding, "content-encoding"},
    {kHttpContentLanguage, "content-language"},
    {kHttpContentLength, "content-length"},
    {kHttpContentLocation, "content-location"},
    {kHttpContentRange, "content-range"},
    {kHttpContentType, "content-type"},
    {kHttpCookie, "cookie"},
    {kHttpDate, "date"},
    {kHttpETag, "etag"},
    {kHttpExpect, "expect"},
    {kHttpExpires, "expires"},
    {kHttpForwardedFor, "forwarded-for"},
    {kHttpFrom, "from"},
    {kHttpHost, "host"},
    {kHttpIfMatch, "if-match"},
    {kHttpIfModifiedSince, "if-modified-since"},
    {kHttpIfNoneMatch, "if-none-match"},
    {kHttpIfRange, "if-range"},
    {kHttpIfUnmodifiedSince, "if-unmodified-since"},
    {kHttpLastModified, "last-modified"},
    {kHttpLink, "link"},
    {kHttpLocation, "location"},
    {kHttpMaxForwards, "max-forwards"},
    {kHttpProxyAuthenticate, "proxy-authenticate"},
    {kHttpProxyAuthorization, "proxy-authorization"},
    {kHttpRange, "range"},
    {kHttpReferer, "referer"},
    {kHttpRefresh, "refresh"},
    {kHttpRetryAfter, "retry-after"},
    {kHttpServer, "server"},
    {kHttpSetCookie, "set-cookie"},
    {kHttpStrictTransportSecurity, "strict-transport-security"},
    {kHttpTransferEncoding, "transfer-encoding"},
    {kHttpUserAgent, "user-agent"},
    {kHttpVary, "vary"},
    {kHttpVia, "via"},
    {kHttpWwwAuthenticate, "www-authenticate"},
};
static_assert(size(s_hdrNames) == kHttps);
const TokenTable s_hdrNameTbl(s_hdrNames);

const TokenTable::Token s_methodNames[] = {
    {fHttpMethodConnect, "CONNECT"},
    {fHttpMethodDelete, "DELETE"},
    {fHttpMethodGet, "GET"},
    {fHttpMethodHead, "HEAD"},
    {fHttpMethodOptions, "OPTIONS"},
    {fHttpMethodPost, "POST"},
    {fHttpMethodPut, "PUT"},
    {fHttpMethodTrace, "TRACE"},
};
static_assert(size(s_methodNames) == kHttpMethods);
const TokenTable s_methodNameTbl(s_methodNames);

} // namespace


/****************************************************************************
*
*   HttpMsg::HdrList
*
***/

//===========================================================================
auto HttpMsg::HdrList::begin() -> ForwardListIterator<HdrName> {
    return ForwardListIterator<HdrName>{m_firstHeader};
}

//===========================================================================
auto HttpMsg::HdrList::end() -> ForwardListIterator<HdrName> {
    return ForwardListIterator<HdrName>{nullptr};
}

//===========================================================================
auto HttpMsg::HdrList::begin() const -> ForwardListIterator<HdrName const> {
    return ForwardListIterator<HdrName const>{m_firstHeader};
}

//===========================================================================
auto HttpMsg::HdrList::end() const -> ForwardListIterator<HdrName const> {
    return ForwardListIterator<HdrName const>{nullptr};
}


/****************************************************************************
*
*   HttpMsg::HdrName
*
***/

//===========================================================================
auto HttpMsg::HdrName::begin() -> ForwardListIterator<HdrValue> {
    return ForwardListIterator<HdrValue>(&m_value);
}

//===========================================================================
auto HttpMsg::HdrName::end() -> ForwardListIterator<HdrValue> {
    return ForwardListIterator<HdrValue>(nullptr);
}

//===========================================================================
auto HttpMsg::HdrName::begin() const -> ForwardListIterator<HdrValue const> {
    return ForwardListIterator<HdrValue const>(&m_value);
}

//===========================================================================
auto HttpMsg::HdrName::end() const -> ForwardListIterator<HdrValue const> {
    return ForwardListIterator<HdrValue const>(nullptr);
}


/****************************************************************************
*
*   HttpMsg
*
***/

//===========================================================================
void HttpMsg::clear() {
    m_flags = {};
    m_data.clear();
    m_heap.clear();
    m_firstHeader = nullptr;
    m_stream = 0;
}

//===========================================================================
void HttpMsg::swap(HttpMsg & other) {
    // You can't swap a request with a response. They different data and,
    // consequently, different memory layouts.
    assert(isRequest() == other.isRequest());

    ::swap(m_flags, other.m_flags);
    m_data.swap(other.m_data);
    m_heap.swap(other.m_heap);
    ::swap(m_firstHeader, other.m_firstHeader);
    ::swap(m_stream, other.m_stream);
}

//===========================================================================
void HttpMsg::addHeader(HttpHdr id, const char value[]) {
    addHeaderRef(id, m_heap.strDup(value));
}

//===========================================================================
void HttpMsg::addHeader(HttpHdr id, string_view value) {
    addHeaderRef(id, m_heap.strDup(value));
}

//===========================================================================
void HttpMsg::addHeader(const char name[], const char value[]) {
    addHeader(name, string_view(value));
}

//===========================================================================
void HttpMsg::addHeader(const char name[], string_view value) {
    if (auto id = httpHdrFromString(name))
        return addHeader(id, value);

    addHeaderRef(kHttpInvalid, m_heap.strDup(name), m_heap.strDup(value));
}

//===========================================================================
void HttpMsg::addHeader(HttpHdr id, TimePoint time) {
    auto hv = addRef(time);
    if (!hv)
        logMsgFatal() << "addHeader(" << id << "): invalid time";

    if (auto hv = addRef(time))
        addHeaderRef(id, hv);
}

//===========================================================================
void HttpMsg::addHeader(const char name[], TimePoint time) {
    auto hv = addRef(time);
    if (!hv)
        logMsgFatal() << "addHeader(" << name << "): invalid time";

    if (auto id = httpHdrFromString(name)) {
        addHeaderRef(id, hv);
    } else {
        addHeaderRef(id, m_heap.strDup(name), hv);
    }
}

//===========================================================================
HttpMsg::Flags HttpMsg::toHasFlag(HttpHdr id) const {
    switch (id) {
    case kHttp_Status: return fFlagHasStatus;
    case kHttp_Method: return fFlagHasMethod;
    case kHttp_Scheme: return fFlagHasScheme;
    case kHttp_Authority: return fFlagHasAuthority;
    case kHttp_Path: return fFlagHasPath;
    default:
        return {};
    }
}

//===========================================================================
void HttpMsg::addHeaderRef(HttpHdr id, const char name[], const char value[]) {
    auto ni = m_firstHeader;
    auto prev = (HdrName *) nullptr;
    auto pseudo = name[0] == ':';
    for (;;) {
        if (!ni) {
            ni = m_heap.emplace<HdrName>();
            if (pseudo)
                m_flags |= toHasFlag(id);
            break;
        }
        if (pseudo && ni->m_name[0] != ':') {
            auto next = ni;
            ni = m_heap.emplace<HdrName>();
            ni->m_next = next;
            m_flags |= toHasFlag(id);
            break;
        }
        if (ni->m_id == id && (id || strcmp(ni->m_name, name) == 0))
            goto ADD_VALUE;

        prev = ni;
        ni = ni->m_next;
    }

    // update new header name info
    if (!prev) {
        // only header goes to front of list
        m_firstHeader = ni;
    } else {
        // add regular headers to the back
        prev->m_next = ni;
    }
    ni->m_id = id;
    ni->m_name = name;

ADD_VALUE:
    auto vi = ni->m_value.m_prev;
    if (!vi) {
        vi = &ni->m_value;
    } else {
        vi = m_heap.emplace<HdrValue>();
        vi->m_next = &ni->m_value;
        if (auto pv = ni->m_value.m_prev) {
            vi->m_prev = pv;
            pv->m_next = vi;
        }
        vi->m_next->m_prev = vi;
    }
    vi->m_value = value;
}

//===========================================================================
void HttpMsg::addHeaderRef(HttpHdr id, const char value[]) {
    const char * name = s_hdrNameTbl.findName(id);
    addHeaderRef(id, name, value);
}

//===========================================================================
void HttpMsg::addHeaderRef(const char name[], const char value[]) {
    HttpHdr id = s_hdrNameTbl.find(name, kHttpInvalid);
    addHeaderRef(id, name, value);
}

//===========================================================================
const char * HttpMsg::addRef(TimePoint time) {
    const unsigned kHttpDateLen = 30;
    tm tm;
    if (!timeToDesc(&tm, time))
        return nullptr;
    char * tmp = heap().alloc<char>(kHttpDateLen);
    strftime(tmp, kHttpDateLen, "%a, %d %b %Y %T GMT", &tm);
    return tmp;
}

//===========================================================================
HttpMsg::HdrList HttpMsg::headers() {
    HdrList hl;
    hl.m_firstHeader = m_firstHeader;
    return hl;
}

//===========================================================================
const HttpMsg::HdrList HttpMsg::headers() const {
    HdrList hl;
    hl.m_firstHeader = m_firstHeader;
    return hl;
}

//===========================================================================
HttpMsg::HdrName HttpMsg::headers(HttpHdr header) {
    for (auto && hdr : headers()) {
        if (hdr.m_id == header)
            return hdr;
    }
    return {};
}

//===========================================================================
const HttpMsg::HdrName HttpMsg::headers(HttpHdr header) const {
    return const_cast<HttpMsg *>(this)->headers(header);
}

//===========================================================================
HttpMsg::HdrName HttpMsg::headers(const char name[]) {
    HttpHdr id = s_hdrNameTbl.find(name, kHttpInvalid);
    return headers(id);
}

//===========================================================================
const HttpMsg::HdrName HttpMsg::headers(const char name[]) const {
    return const_cast<HttpMsg *>(this)->headers(name);
}

//===========================================================================
bool HttpMsg::hasHeader(HttpHdr header) const {
    return headers(header).m_id;
}

//===========================================================================
bool HttpMsg::hasHeader(const char name[]) const {
    return headers(name).m_id;
}

//===========================================================================
CharBuf & HttpMsg::body() {
    return m_data;
}

//===========================================================================
const CharBuf & HttpMsg::body() const {
    return m_data;
}

//===========================================================================
ITempHeap & HttpMsg::heap() {
    return m_heap;
}


/****************************************************************************
*
*   HttpRequest
*
***/

//===========================================================================
void HttpRequest::swap(HttpMsg & other) {
    HttpMsg::swap(other);
    ::swap(m_query, static_cast<HttpRequest &>(other).m_query);
}

//===========================================================================
const char * HttpRequest::method() const {
    return headers(kHttp_Method).begin()->m_value;
}

//===========================================================================
const char * HttpRequest::scheme() const {
    return headers(kHttp_Scheme).begin()->m_value;
}

//===========================================================================
const char * HttpRequest::authority() const {
    return headers(kHttp_Authority).begin()->m_value;
}

//===========================================================================
const char * HttpRequest::pathRaw() const {
    return headers(kHttp_Path).begin()->m_value;
}

//===========================================================================
const HttpQuery & HttpRequest::query() const {
    if (!m_query) {
        auto self = const_cast<HttpRequest *>(this);
        self->m_query = urlParseHttpPath(pathRaw(), self->heap());
        if (!body().empty()) {
            auto hdr = headers(kHttpContentType);
            if (auto val = hdr.m_value.m_value;
                strcmp(val, "application/x-www-form-urlencoded") == 0
            ) {
                urlAddQueryString(self->m_query, body().c_str(), self->heap());
            }
        }
    }
    return *m_query;
}

//===========================================================================
bool HttpRequest::checkPseudoHeaders() const {
    const Flags must = fFlagHasMethod | fFlagHasScheme | fFlagHasPath;
    const Flags mustNot = fFlagHasStatus;
    return (m_flags & must) == must && (~m_flags & mustNot);
}


/****************************************************************************
*
*   HttpResponse
*
***/

//===========================================================================
HttpResponse::HttpResponse(
    HttpStatus status,
    string_view contentType
) {
    auto str = StrFrom{status};
    addHeader(kHttp_Status, str.view());
    if (!contentType.empty())
        addHeader(kHttpContentType, contentType);
}

//===========================================================================
int HttpResponse::status() const {
    auto val = headers(kHttp_Status).begin()->m_value;
    return strToInt(val);
}

//===========================================================================
bool HttpResponse::checkPseudoHeaders() const {
    const Flags must = fFlagHasStatus;
    const Flags mustNot = fFlagHasMethod | fFlagHasScheme | fFlagHasAuthority
        | fFlagHasPath;
    return (m_flags & must) == must && (~m_flags & mustNot);
}


/****************************************************************************
*
*   Public API
*
***/

//===========================================================================
const char * Dim::toString(HttpHdr id) {
    return s_hdrNameTbl.findName(id);
}

//===========================================================================
HttpHdr Dim::httpHdrFromString(string_view name, HttpHdr def) {
    return s_hdrNameTbl.find(name, def);
}

//===========================================================================
const char * Dim::toString(HttpMethod id) {
    return s_methodNameTbl.findName(id);
}

//===========================================================================
vector<string_view> Dim::to_views(HttpMethod methods) {
    return s_methodNameTbl.findNames(methods);
}

//===========================================================================
HttpMethod Dim::httpMethodFromString(string_view name, HttpMethod def) {
    return s_methodNameTbl.find(name, def);
}

//===========================================================================
bool httpParse(TimePoint * time, std::string_view val) {
    assert(!"httpTimeFromChar not implemented");
    *time = {};
    return false;
}
