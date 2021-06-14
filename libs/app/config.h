// Copyright Glen Knowles 2017 - 2019.
// Distributed under the Boost Software License, Version 1.0.
//
// config.h - dim app
#pragma once

#include "cppconf/cppconf.h"

#include "core/time.h"
#include "json/json.h"
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
    SockAddr saddr;
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

const XNode * configElement(
    const ConfigContext & context,
    const XDocument & doc,
    std::string_view name
);
const char * configString(
    const ConfigContext & context,
    const XDocument & doc,
    std::string_view name,
    const char defVal[] = ""
);
bool configBool(
    const ConfigContext & context,
    const XDocument & doc,
    std::string_view name,
    bool defVal = false
);
double configNumber(
    const ConfigContext & context,
    const XDocument & doc,
    std::string_view name,
    double defVal = 0
);
Duration configDuration(
    const ConfigContext & context,
    const XDocument & doc,
    std::string_view name,
    Duration defVal = {}
);

const XNode * configElement(
    const XDocument & doc,
    std::string_view name
);
const char * configString(
    const XDocument & doc,
    std::string_view name,
    const char defVal[] = ""
);
bool configBool(
    const XDocument & doc,
    std::string_view name,
    bool defVal = false
);
double configNumber(
    const XDocument & doc,
    std::string_view name,
    double defVal = 0
);
Duration configDuration(
    const XDocument & doc,
    std::string_view name,
    Duration defVal = {}
);


/****************************************************************************
*
*   Debugging
*
***/

void configWriteRules(IJBuilder * out);

} // namespace
