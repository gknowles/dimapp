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

void httpRouteReply(unsigned reqId, HttpResponse & msg, bool more = false);
void httpRouteReply(unsigned reqId, const CharBuf & data, bool more);
void httpRouteReply(unsigned reqId, std::string_view data, bool more);
void httpRouteReplyWithFile(unsigned reqId, std::string_view path);

void httpRouteReplyNotFound(unsigned reqId, const HttpRequest & req);
void httpRouteReply(
    unsigned reqId,
    const HttpRequest & req,
    unsigned status,
    const std::string & msg = {}
);

} // namespace
