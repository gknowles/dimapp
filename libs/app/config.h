// Copyright Glen Knowles 2017.
// Distributed under the Boost Software License, Version 1.0.
//
// config.h - dim app
#pragma once

#include "cppconf/cppconf.h"

#include "net/address.h"
#include "xml/xml.h"

#include <string_view>

namespace Dim {


/****************************************************************************
*
*   Declarations
*
***/

struct ConfigContext {
    Endpoint endpoint;
    std::string appName;
    unsigned appIndex{0};
    std::string config;
    std::string module; // socket manager name
};


/****************************************************************************
*
*   Monitor
*
***/

class IConfigNotify {
public:
    virtual ~IConfigNotify () = default;

    virtual void onConfigChange(const XDocument & doc) = 0;
};

void configMonitor(std::string_view file, IConfigNotify * notify);
void configCloseWait(std::string_view file, IConfigNotify * notify);

// If notify is null, all notifiers monitoring the file are called. Otherwise,
// the specified monitor is called if it is monitoring the file. An error is
// logged if a notify is specified and it's not monitoring the file.
void configChange(
    std::string_view file,
    IConfigNotify * notify = nullptr
);


/****************************************************************************
*
*   Config file queries
*
***/

unsigned configUnsigned(
    const ConfigContext & context,
    const XDocument & doc,
    std::string_view name,
    unsigned defVal = 0
);
const char * configString(
    const ConfigContext & context,
    const XDocument & doc,
    std::string_view name,
    const char defVal[] = ""
);

unsigned configUnsigned(
    const XDocument & doc,
    std::string_view name,
    unsigned defVal = 0
);
const char * configString(
    const XDocument & doc,
    std::string_view name,
    const char defVal[] = ""
);

} // namespace
