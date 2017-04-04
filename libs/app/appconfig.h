// Copyright Glen Knowles 2017.
// Distributed under the Boost Software License, Version 1.0.
//
// appconfig.h - dim app
#pragma once

#include "cppconf/cppconf.h"

#include "xml/xml.h"

#include <string_view>

namespace Dim {


/****************************************************************************
*
*   Monitor
*
***/

class IAppConfigNotify {
public:
    virtual ~IAppConfigNotify () {}

    virtual void onConfigChange(
        std::string_view relpath, 
        const XNode * root
    ) = 0;
};

void appConfigMonitor(std::string_view file, IAppConfigNotify * notify);

void appConfigChange(
    std::string_view file, 
    IAppConfigNotify * notify = nullptr
);

} // namespace
