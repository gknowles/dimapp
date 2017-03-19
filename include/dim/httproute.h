// httpRoute.h - dim services
#pragma once

#include "config.h"

#include "http.h"

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
    unsigned methods = fHttpMethodGet);

void httpRouteReply(unsigned reqId, HttpResponse & msg, bool more = false);
void httpRouteReply(unsigned reqId, const CharBuf & data, bool more);

} // namespace
