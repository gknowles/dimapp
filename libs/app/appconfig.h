// Copyright Glen Knowles 2015 - 2017.
// Distributed under the Boost Software License, Version 1.0.
//
// appconfig.h - dim app
#pragma once

#include "cppconf/cppconf.h"

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

    virtual void onConfigChange(std::string_view path) = 0;
};

void appConfigMonitor(std::string_view file, IAppConfigNotify * notify);

void appConfigChange(
    std::string_view file, 
    IAppConfigNotify * notify = nullptr
);

} // namespace
