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
    virtual void onWebAdminRequest(
        IJBuilder * out, 
        unsigned reqId, 
        HttpRequest & msg
    ) = 0;

protected:
    void onHttpRequest(unsigned reqId, HttpRequest & msg) override;

private:
    Param & m_jsVar = param("jsVar");
};


} // namespace
