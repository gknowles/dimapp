// Copyright Glen Knowles 2017 - 2018.
// Distributed under the Boost Software License, Version 1.0.
//
// httpRoute.h - dim net
#pragma once

#include "cppconf/cppconf.h"

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
    unsigned methods = fHttpMethodGet,
    bool recurse = false
);


struct MimeType {
    const char * fileExt;
    const char * type;
    const char * charSet;
};

void httpRouteReply(unsigned reqId, HttpResponse && msg, bool more = false);
void httpRouteReply(unsigned reqId, CharBuf && data, bool more);
void httpRouteReply(unsigned reqId, std::string_view data, bool more);

// Aborts incomplete reply with CANCEL or INTERNAL_ERROR
void httpRouteCancel(unsigned reqId);
void httpRouteInternalError(unsigned reqId);

void httpRouteReplyWithFile(unsigned reqId, std::string_view path);

void httpRouteReplyNotFound(unsigned reqId, const HttpRequest & req);
void httpRouteReply(
    unsigned reqId,
    const HttpRequest & req,
    unsigned status,
    std::string_view msg = {}
);
void httpRouteReply(unsigned reqId, unsigned status, std::string_view msg = {});

} // namespace
