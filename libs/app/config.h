// Copyright Glen Knowles 2017.
// Distributed under the Boost Software License, Version 1.0.
//
// config.h - dim app
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

class IConfigNotify {
public:
    virtual ~IConfigNotify () {}

    virtual void onConfigChange(
        std::string_view relpath, 
        const XNode * root
    ) = 0;
};

void configMonitor(std::string_view file, IConfigNotify * notify);

// If notify is null, all notifiers monitoring the file are called. Otherwise,
// the specified monitor is called if it is monitoring the file. An error is
// logged if a notify is specified and it's not monitoring the file.
void configChange(
    std::string_view file, 
    IConfigNotify * notify = nullptr
);

} // namespace
