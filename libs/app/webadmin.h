// Copyright Glen Knowles 2018.
// Distributed under the Boost Software License, Version 1.0.
//
// webadmin.h - dim app
#pragma once

#include "cppconf/cppconf.h"

#include "net/http.h"

#include <string_view>

namespace Dim {


/****************************************************************************
*
*   Web UI helpers
*
***/

class WebDirList : public IHttpRouteNotify {
};

void httpReplyDirList(
    unsigned reqId,
    Dim::HttpRequest & msg,
    std::string_view path
);

} // namespace
