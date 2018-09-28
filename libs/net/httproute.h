// Copyright Glen Knowles 2017 - 2018.
// Distributed under the Boost Software License, Version 1.0.
//
// httpRoute.h - dim net
#pragma once

#include "cppconf/cppconf.h"

#include "json/json.h"
#include "net/http.h"
#include "net/url.h"

#include <string>
#include <unordered_map>

namespace Dim {


/****************************************************************************
*
*   Http routes
*
***/

class IHttpRouteNotify {
public:
    virtual ~IHttpRouteNotify() = default;

    virtual void onHttpRequest(unsigned reqId, HttpRequest & msg) = 0;
};
void httpRouteAdd(
    IHttpRouteNotify * notify,
    std::string_view path,
    HttpMethod methods = fHttpMethodGet,
    bool recurse = false
);
void httpRouteAddFile(
    std::string_view path,
    TimePoint mtime,
    std::string_view content,
    std::string_view mimeType = {},
    std::string_view charSet = {}
);
void httpRouteAddFileRef(
    std::string_view path,
    TimePoint mtime,
    std::string_view content,
    std::string_view mimeType = {},
    std::string_view charSet = {}
);

struct MimeType {
    std::string_view fileExt;
    std::string_view type;
    std::string_view charSet;
};
int compareExt(const MimeType & a, const MimeType & b);
MimeType mimeTypeDefault(std::string_view path);

void httpRouteReply(unsigned reqId, HttpResponse && msg, bool more = false);
void httpRouteReply(unsigned reqId, CharBuf && data, bool more);
void httpRouteReply(unsigned reqId, std::string_view data, bool more);

// Aborts incomplete reply with CANCEL or INTERNAL_ERROR
void httpRouteCancel(unsigned reqId);
void httpRouteInternalError(unsigned reqId);

void httpRouteReplyWithFile(unsigned reqId, std::string_view path);
void httpRouteReplyWithFile(
    unsigned reqId,
    TimePoint mtime,
    std::string_view content,
    std::string_view mimeType,
    std::string_view charSet
);

void httpRouteReplyNotFound(unsigned reqId, const HttpRequest & req);
void httpRouteReply(
    unsigned reqId,
    const HttpRequest & req,
    unsigned status,
    std::string_view msg = {}
);
void httpRouteReply(unsigned reqId, unsigned status, std::string_view msg = {});

void httpRouteSetDefaultReplyHeader(
    HttpHdr hdr,
    const char value[] = nullptr
);
void httpRouteSetDefaultReplyHeader(
    const char name[],
    const char value[] = nullptr
);


/****************************************************************************
*
*   Debugging
*
***/

void httpRouteGetRoutes(IJBuilder * out);

} // namespace
