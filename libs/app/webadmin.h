// Copyright Glen Knowles 2022.
// Distributed under the Boost Software License, Version 1.0.
//
// webadmin.h - dim app
#pragma once

#include "cppconf/cppconf.h"

#include "core/core.h"
#include "json/json.h"
#include "net/net.h"

#include <string_view>

namespace Dim {


/****************************************************************************
*
*   Declarations
*
***/

class IWebAdminNotify : public IHttpRouteNotify {
public:
    JBuilder initResponse(
        HttpResponse * out,
        unsigned reqId,
        const HttpRequest & req
    );

    // Inherited via IHttpRouteNotify
    void onHttpRequest(unsigned reqId, HttpRequest & msg) override = 0;

private:
    Param<> m_jsVar = {this, "jsVar"};
};

std::unordered_map<std::string, std::string> & webAdminAppData();

} // namespace
