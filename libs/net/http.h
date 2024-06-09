// Copyright Glen Knowles 2015 - 2024.
// Distributed under the Boost Software License, Version 1.0.
//
// http.h - dim http
//
// implements http/2, as defined by:
//  rfc7540 - Hypertext Transfer Protocol Version 2 (HTTP/2)
//  rfc7541 - HPACK: Header Compression for HTTP/2

#pragma once

#include "cppconf/cppconf.h"

#include "core/charbuf.h"
#include "core/handle.h"
#include "core/types.h" // ForwardListIterator
#include "net/url.h"

#include <memory>
#include <string_view>
#include <vector>

namespace Dim {


/****************************************************************************
*
*   Constants
*
***/

enum HttpHdr {
    kHttpInvalid,
    kHttp_Authority,
    kHttp_Method,
    kHttp_Path,
    kHttp_Scheme,
    kHttp_Status,
    kHttpAccept,
    kHttpAcceptCharset,
    kHttpAcceptEncoding,
    kHttpAcceptLanguage,
    kHttpAcceptRanges,
    kHttpAccessControlAllowOrigin,
    kHttpAge,
    kHttpAllow,
    kHttpAuthorization,
    kHttpCacheControl,
    kHttpConnection,
    kHttpContentDisposition,
    kHttpContentEncoding,
    kHttpContentLanguage,
    kHttpContentLength,
    kHttpContentLocation,
    kHttpContentRange,
    kHttpContentType,
    kHttpCookie,
    kHttpDate,
    kHttpETag,
    kHttpExpect,
    kHttpExpires,
    kHttpForwarded,
    kHttpFrom,
    kHttpHost,
    kHttpIfMatch,
    kHttpIfModifiedSince,
    kHttpIfNoneMatch,
    kHttpIfRange,
    kHttpIfUnmodifiedSince,
    kHttpLastModified,
    kHttpLink,
    kHttpLocation,
    kHttpMaxForwards,
    kHttpProxyAuthenticate,
    kHttpProxyAuthorization,
    kHttpRange,
    kHttpReferer,
    kHttpRefresh,
    kHttpRetryAfter,
    kHttpServer,
    kHttpSetCookie,
    kHttpStrictTransportSecurity,
    kHttpTransferEncoding,
    kHttpUserAgent,
    kHttpVary,
    kHttpVia,
    kHttpWwwAuthenticate,
    kHttps
};

const char * toString(HttpHdr id);
HttpHdr httpHdrFromString(std::string_view name, HttpHdr def = kHttpInvalid);

enum HttpMethod : unsigned {
    fHttpMethodInvalid  = 0x00,
    fHttpMethodConnect  = 0x01,
    fHttpMethodDelete   = 0x02,
    fHttpMethodGet      = 0x04,
    fHttpMethodHead     = 0x08,
    fHttpMethodOptions  = 0x10,
    fHttpMethodPost     = 0x20,
    fHttpMethodPut      = 0x40,
    fHttpMethodTrace    = 0x80,
    kHttpMethods        = 8
};

const char * toString(HttpMethod method);
HttpMethod httpMethodFromString(
    std::string_view name,
    HttpMethod def = fHttpMethodInvalid
);

std::vector<std::string_view> toViews(EnumFlags<HttpMethod> methods);

enum HttpStatus {
    // 1xx Informational
    kHttpStatusContinue = 100,
    kHttpStatusSwitchingProtocols = 101,
    // 2xx Success
    kHttpStatusOk = 200,
    kHttpStatusCreated = 201,
    kHttpStatusAccepted = 202,
    kHttpStatusNonAuthoritative = 203,
    kHttpStatusNoContent = 204,
    kHttpStatusResetContent = 205,
    kHttpStatusPartialContent = 206,
    // 3xx Redirection
    kHttpStatusMultipleChoices = 300,
    kHttpStatusMovedPermanently = 301,
    kHttpStatusFound = 302,
    kHttpStatusSeeOther = 303,
    kHttpStatusNotModified = 304,
    kHttpStatusUseProxy = 305,
    kHttpStatusTemporaryRedirect = 307,
    kHttpStatusPermanentRedirect = 308,
    // 4xx Client Error
    kHttpStatusBadRequest = 400,
    kHttpStatusUnauthorized = 401,
    kHttpStatusPaymentRequired = 402,
    kHttpStatusForbidden = 403,
    kHttpStatusNotFound = 404,
    kHttpStatusMethodNotAllowed = 405,
    kHttpStatusNotAcceptable = 406,
    kHttpStatusProxyAuthenticationRequired = 407,
    kHttpStatusRequestTimeout = 408,
    kHttpStatusConflict = 409,
    kHttpStatusGone = 410,
    kHttpStatusLengthRequired = 411,
    kHttpStatusPreconditionFailed = 412,
    kHttpStatusPayloadTooLarge = 413,
    kHttpStatusUriTooLong = 414,
    kHttpStatusUnsupportedMediaType = 415,
    kHttpStatusRangeNotSatisfiable = 416,
    kHttpStatusExpectationFailed = 417,
    kHttpStatusMisdirectedRequest = 421,
    kHttpStatusUpgradeRequired = 426,
    kHttpStatusPreconditionRequired = 428,
    kHttpStatusTooManyRequests = 429,
    kHttpStatusRequestHeaderFieldsTooLarge = 431,
    kHttpStatusUnavailableForLegalReasons = 451,
    // 5xx Server Error
    kHttpStatusInternalServerError = 500,
    kHttpStatusNotImplemented = 501,
    kHttpStatusBadGateway = 502,
    kHttpStatusServiceUnavailable = 503,
    kHttpStatusGatewayTimeout = 504,
    kHttpStatusHttpVersionNotSupported = 505,
    kHttpStatusVariantAlsoNegotiates = 506,
    kHttpStatusNotExtended = 510,
    kHttpStatusNetworkAuthenticationRequired = 511,
};

// Will fallback to more generic reasons for unknown status codes
const char * toReasonPhrase(HttpStatus status);


/****************************************************************************
*
*   Http message
*
***/

class HttpMsg {
public:
    struct HdrList;
    struct HdrName;
    struct HdrValue;

public:
    HttpMsg (int stream = 0) : m_stream{stream} {}
    HttpMsg (HttpMsg && from) = default;
    virtual ~HttpMsg() = default;
    HttpMsg & operator= (HttpMsg && from) = default;
    void clear();
    virtual void swap(HttpMsg & other);

    void addHeader(HttpHdr id, const char value[]);
    void addHeader(HttpHdr id, std::string_view value);
    void addHeader(const char name[], const char value[]);
    void addHeader(const char name[], std::string_view value);
    void addHeader(HttpHdr id, TimePoint time);
    void addHeader(const char name[], TimePoint time);

    // When adding references the memory referenced by the name and value
    // pointers must be valid for the life of the HttpMsg, such as constants
    // or strings allocated from this messages heap().
    void addHeaderRef(HttpHdr id, const char value[]);
    void addHeaderRef(const char name[], const char value[]);

    const char * addRef(TimePoint time);

    // Returns true if header removed, false if header wasn't found.
    bool removeHeader(HttpHdr id);
    bool removeHeader(const char name[]);

    HdrList headers();
    const HdrList headers() const;

    HdrName headers(HttpHdr header);
    const HdrName headers(HttpHdr header) const;
    HdrName headers(const char name[]);
    const HdrName headers(const char name[]) const;

    bool hasHeader(HttpHdr header) const;
    bool hasHeader(const char name[]) const;

    CharBuf & body();
    const CharBuf & body() const;

    IHeap & heap();

    virtual bool isRequest() const = 0;
    int stream() const { return m_stream; }

    virtual bool checkPseudoHeaders() const = 0;

protected:
    virtual void addHeaderRef(
        HttpHdr id,
        const char name[],
        const char value[]
    );
    virtual bool removeHeader(HttpHdr id, const char name[]);

    enum Flags : unsigned {
        fFlagHasStatus = 0x01,
        fFlagHasMethod = 0x02,
        fFlagHasScheme = 0x04,
        fFlagHasAuthority = 0x08,
        fFlagHasPath = 0x10,
        fFlagHasHeader = 0x20,
    };
    EnumFlags<Flags> m_flags = {};
    Flags toHasFlag(HttpHdr id) const;

private:
    CharBuf m_data;
    TempHeap m_heap;
    HdrName * m_firstHeader{};
    int m_stream{};
};

struct HttpMsg::HdrValue {
    const char * m_value{};
    HdrValue * m_next{};
    HdrValue * m_prev{};
};

struct HttpMsg::HdrName {
    HttpHdr m_id{kHttpInvalid};
    const char * m_name{};
    HdrName * m_next{};
    HdrValue m_value;

    ForwardListIterator<HdrValue> begin();
    ForwardListIterator<HdrValue> end();
    ForwardListIterator<const HdrValue> begin() const;
    ForwardListIterator<const HdrValue> end() const;
};

struct HttpMsg::HdrList {
    HdrName * m_firstHeader{};

    ForwardListIterator<HdrName> begin();
    ForwardListIterator<HdrName> end();
    ForwardListIterator<const HdrName> begin() const;
    ForwardListIterator<const HdrName> end() const;

    explicit operator bool() const { return m_firstHeader; }
};


class HttpRequest : public HttpMsg {
public:
    using HttpMsg::HttpMsg;
    using HttpMsg::removeHeader;
    void swap(HttpMsg & other) override;

    const char * method() const;
    const char * scheme() const;
    const char * authority() const;

    // includes path, query, and fragment
    const char * pathRaw() const;
    const HttpQuery & query() const;

    bool isRequest() const override { return true; }

private:
    bool checkPseudoHeaders() const override;
    bool removeHeader(HttpHdr id, const char name[]) override;

    HttpQuery * m_query{};
};

class HttpResponse : public HttpMsg {
public:
    HttpResponse(HttpStatus status, std::string_view contentType = {});
    using HttpMsg::HttpMsg;

    int status() const;

    bool isRequest() const override { return false; }

private:
    bool checkPseudoHeaders() const override;
};


/****************************************************************************
*
*   Http connection
*
***/

struct HttpConnHandle : HandleBase {};

HttpConnHandle httpConnect(CharBuf * out);
HttpConnHandle httpAccept();

void httpClose(HttpConnHandle conn);
std::string_view httpGetError(HttpConnHandle conn);

// Returns false when no more data will be accepted, either by request
// of the input or due to error.
// Even after an error, out and msgs should be processed.
//  - out: data to send to the remote address is appended
//  - msgs: zero or more requests, push promises, and/or replies are appended
bool httpRecv(
    CharBuf * out,
    std::vector<std::unique_ptr<HttpMsg>> * msgs,
    HttpConnHandle conn,
    const void * src,
    size_t srcLen
);

// Serializes a request and returns the stream id used
int httpRequest(
    CharBuf * out,
    HttpConnHandle conn,
    const HttpMsg & msg,
    bool more = false
);

// Serializes a push promise and returns the stream id used
int httpPushPromise(
    CharBuf * out,
    HttpConnHandle conn,
    const HttpMsg & msg,
    bool more = false
);

// Serializes a reply on the specified stream, returns false if the stream has
// been closed.
bool httpReply(
    CharBuf * out,
    HttpConnHandle conn,
    int stream,
    const HttpMsg & msg,
    bool more = false
);

// Sends more data on a stream, a stream ends after request, push promise,
// reply, or data is called with more false. Returns false if the stream has
// been closed.
bool httpData(
    CharBuf * out,
    HttpConnHandle conn,
    int stream,
    const CharBuf & data,
    bool more = false
);
bool httpData(
    CharBuf * out,
    HttpConnHandle conn,
    int stream,
    std::string_view data,
    bool more = false
);

// Resets stream with either INTERNAL_ERROR or CANCEL
void httpResetStream(
    CharBuf * out,
    HttpConnHandle conn,
    int stream,
    bool internal
);


/****************************************************************************
*
*   Headers
*
***/

bool httpParse(TimePoint * time, std::string_view val);

} // namespace
