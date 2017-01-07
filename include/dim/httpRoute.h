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
        std::unordered_multimap<std::string, std::string> & params,
        HttpMsg & msg
    ) = 0;
};

void httpRouteAdd(
    IHttpRouteNotify * notify,
    const std::string host,
    const std::string path,
    unsigned methods = fHttpMethodGet
);

void httpRouteReply(unsigned reqId, HttpMsg & msg, bool more = false);
void httpRouteReply(unsigned reqId, const CharBuf & data, bool more);

} // namespace
