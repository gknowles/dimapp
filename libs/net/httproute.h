// Copyright Glen Knowles 2017.
// Distributed under the Boost Software License, Version 1.0.
//
// httpRoute.h - dim net
#pragma once

#include "cppconf/cppconf.h"

#include "net/http.h"

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
    virtual ~IHttpRouteNotify() {}

    virtual void onHttpRequest(
        unsigned reqId,
        std::unordered_multimap<std::string_view, std::string_view> & params,
        HttpRequest & msg) = 0;
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

void httpRouteReplyNotFound(unsigned reqId, const HttpRequest & req);
void httpRouteReplyWithFile(unsigned reqId, std::string_view path);

} // namespace
