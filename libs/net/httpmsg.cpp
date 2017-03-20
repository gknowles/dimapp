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

struct HttpHdrInfo {
    HttpHdr header;
    const char * name;
};

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
    {kHttpAccessControlAllowOrigin, "accept-control-allow-origin"},
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
static_assert(size(s_hdrNames) == kHttps, "");

const TokenTable s_hdrNameTbl(s_hdrNames, size(s_hdrNames));

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
auto HttpMsg::HdrList::begin() const -> ForwardListIterator<const HdrName> {
    return ForwardListIterator<const HdrName>{m_firstHeader};
}

//===========================================================================
auto HttpMsg::HdrList::end() const -> ForwardListIterator<const HdrName> {
    return ForwardListIterator<const HdrName>{nullptr};
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
auto HttpMsg::HdrName::begin() const -> ForwardListIterator<const HdrValue> {
    return ForwardListIterator<const HdrValue>(&m_value);
}

//===========================================================================
auto HttpMsg::HdrName::end() const -> ForwardListIterator<const HdrValue> {
    return ForwardListIterator<const HdrValue>(nullptr);
}


/****************************************************************************
*
*   HttpMsg
*
***/

//===========================================================================
void HttpMsg::addHeader(HttpHdr id, const char value[]) {
    addHeaderRef(id, m_heap.strDup(value));
}

//===========================================================================
void HttpMsg::addHeader(const char name[], const char value[]) {
    HttpHdr id;
    if (s_hdrNameTbl.find((int *)&id, name))
        return addHeader(id, value);

    addHeaderRef(kHttpInvalid, m_heap.strDup(name), m_heap.strDup(value));
}

//===========================================================================
void HttpMsg::addHeaderRef(HttpHdr id, const char name[], const char value[]) {
    auto ni = m_firstHeader;
    auto prev = ni;
    for (;;) {
        if (!ni) {
            ni = m_heap.emplace<HdrName>();
            if (prev) {
                prev->m_next = ni;
            } else {
                m_firstHeader = ni;
            }
            ni->m_id = id;
            ni->m_name = name;
            break;
        }

        if (ni->m_id == id && (id || !strcmp(ni->m_name, name))) {
            break;
        }
        prev = ni;
        ni = ni->m_next;
    }

    // not found
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
    const char * name = tokenTableGetName(s_hdrNameTbl, id);
    addHeaderRef(id, name, value);
}

//===========================================================================
void HttpMsg::addHeaderRef(const char name[], const char value[]) {
    HttpHdr id = tokenTableGetEnum(s_hdrNameTbl, name, kHttpInvalid);
    addHeaderRef(id, name, value);
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
    HttpHdr id = tokenTableGetEnum(s_hdrNameTbl, name, kHttpInvalid);
    return headers(id);
}

//===========================================================================
const HttpMsg::HdrName HttpMsg::headers(const char name[]) const {
    return const_cast<HttpMsg *>(this)->headers(name);
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
const char * HttpRequest::pathAbsolute() const { 
    return headers(kHttp_Path).begin()->m_value;
}

//===========================================================================
bool HttpRequest::checkPseudoHeaders() const {
    const int must = kFlagHasMethod | kFlagHasScheme | kFlagHasPath;
    const int mustNot = kFlagHasStatus;
    return (m_flags & must) == must && (~m_flags & mustNot);
}


/****************************************************************************
*
*   HttpResponse
*
***/

//===========================================================================
bool HttpResponse::checkPseudoHeaders() const {
    const int must = kFlagHasStatus;
    const int mustNot = kFlagHasMethod | kFlagHasScheme | kFlagHasAuthority
        | kFlagHasPath;
    return (m_flags & must) == must && (~m_flags & mustNot);
}


/****************************************************************************
*
*   Public API
*
***/

//===========================================================================
std::string Dim::to_string(HttpHdr id) {
    return tokenTableGetName(s_hdrNameTbl, id);
}