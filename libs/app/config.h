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
    SockAddr endpoint;
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

    virtual void onConfigChange(XDocument const & doc) = 0;
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

XNode const * configElement(
    ConfigContext const & context,
    XDocument const & doc,
    std::string_view name
);
char const * configString(
    ConfigContext const & context,
    XDocument const & doc,
    std::string_view name,
    char const defVal[] = ""
);
double configNumber(
    ConfigContext const & context,
    XDocument const & doc,
    std::string_view name,
    double defVal = 0
);
Duration configDuration(
    ConfigContext const & context,
    XDocument const & doc,
    std::string_view name,
    Duration defVal = {}
);

XNode const * configElement(
    XDocument const & doc,
    std::string_view name
);
char const * configString(
    XDocument const & doc,
    std::string_view name,
    char const defVal[] = ""
);
double configNumber(
    XDocument const & doc,
    std::string_view name,
    double defVal = 0
);
Duration configDuration(
    XDocument const & doc,
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
