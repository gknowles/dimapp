// http.h - dim services
//
// implements http/2, as defined by:
//  rfc7540 - Hypertext Transfer Protocol Version 2 (HTTP/2)
//  rfc7541 - HPACK: Header Compression for HTTP/2

#pragma once

#include "config/config.h"

#include "core/handle.h"
#include "core/util.h" // ForwardListIterator

#include <memory>
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
    kHttpForwardedFor,
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

std::string to_string(HttpHdr id);


enum HttpMethod {
    fHttpMethodConnect = 0x01,
    fHttpMethodDelete = 0x02,
    fHttpMethodGet = 0x04,
    fHttpMethodHead = 0x08,
    fHttpMethodOptions = 0x10,
    fHttpMethodPost = 0x20,
    fHttpMethodPut = 0x40,
    fHttpMethodTrace = 0x80,
};

std::string to_string(HttpMethod method);


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
    virtual ~HttpMsg() {}

    void addHeader(HttpHdr id, const char value[]);
    void addHeader(const char name[], const char value[]);

    // When adding references the memory referenced by the name and value
    // pointers must be valid for the life of the http msg, such as constants
    // or strings allocated from this messages Heap().
    void addHeaderRef(HttpHdr id, const char value[]);
    void addHeaderRef(const char name[], const char value[]);

    HdrList headers();
    const HdrList headers() const;

    HdrName headers(HttpHdr header);
    const HdrName headers(HttpHdr header) const;
    HdrName headers(const char name[]);
    const HdrName headers(const char name[]) const;

    CharBuf & body();
    const CharBuf & body() const;

    ITempHeap & heap();

    virtual bool isRequest() const = 0;

protected:
    virtual bool checkPseudoHeaders() const = 0;

    enum {
        kFlagHasStatus = 0x01,
        kFlagHasMethod = 0x02,
        kFlagHasScheme = 0x04,
        kFlagHasAuthority = 0x08,
        kFlagHasPath = 0x10,
        kFlagHasHeader = 0x20,
    };
    int m_flags{0}; // kFlag*

private:
    void addHeaderRef(HttpHdr id, const char name[], const char value[]);

    CharBuf m_data;
    TempHeap m_heap;
    HdrName * m_firstHeader{nullptr};
};

struct HttpMsg::HdrValue {
    const char * m_value;
    HdrValue * m_next{nullptr};
    HdrValue * m_prev{nullptr};
};

struct HttpMsg::HdrName {
    HttpHdr m_id{kHttpInvalid};
    const char * m_name{nullptr};
    HdrName * m_next{nullptr};
    HdrValue m_value;

    ForwardListIterator<HdrValue> begin();
    ForwardListIterator<HdrValue> end();
    ForwardListIterator<const HdrValue> begin() const;
    ForwardListIterator<const HdrValue> end() const;
};

struct HttpMsg::HdrList {
    HdrName * m_firstHeader{nullptr};

    ForwardListIterator<HdrName> begin();
    ForwardListIterator<HdrName> end();
    ForwardListIterator<const HdrName> begin() const;
    ForwardListIterator<const HdrName> end() const;
};


class HttpRequest : public HttpMsg {
public:
    const char * method() const;
    const char * scheme() const;
    const char * authority() const;

    // includes path, query, and fragment
    const char * pathAbsolute() const;

    const char * path() const;
    const char * query() const;
    const char * fragment() const;

    bool isRequest() const override { return true; }

private:
    bool checkPseudoHeaders() const override;
};

class HttpResponse : public HttpMsg {
public:
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
HttpConnHandle httpListen();

void httpClose(HttpConnHandle conn);

// Returns false when no more data will be accepted, either by request
// of the input or due to error.
// Even after an error, msgs and out should be processed.
//  - msg: zero or more requests, push promises, and/or replies are appended
//  - out: data to send to the remote endpoint is appended
bool httpRecv(
    HttpConnHandle conn,
    CharBuf * out,
    std::vector<std::unique_ptr<HttpMsg>> * msgs,
    const void * src,
    size_t srcLen);

// Serializes a request and returns the stream id used
int httpRequest(HttpConnHandle conn, CharBuf * out, const HttpMsg & msg);

// Serializes a push promise
void httpPushPromise(HttpConnHandle conn, CharBuf * out, const HttpMsg & msg);

// Serializes a reply on the specified stream
void httpReply(
    HttpConnHandle conn,
    CharBuf * out,
    int stream,
    const HttpMsg & msg);

void httpResetStream(HttpConnHandle conn, CharBuf * out, int stream);

} // namespace