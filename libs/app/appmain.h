// Copyright Glen Knowles 2015 - 2017.
// Distributed under the Boost Software License, Version 1.0.
//
// appmain.h - dim app
//
// Application life-cycle
// 
// 1. Stopped - application performs initialization and then calls appRun, 
//      usually from main(), to start the framework.
// 2. Starting - framework initialization, state never seen by application
// 3. Running - framework calls IAppNotify::onAppRun(). Application then does
//      stuff, and eventually calls appSignalShutdown(). A command line 
//      tool might do all its work and signal shutdown from inside onAppRun.
//      Whereas a server might start listening for socket connections and
//      return, and use some later event to signal shutdown.
// 4. Stopping - process all shutdown monitors and shutdown the framework 
// 5. Stopped - framework is no longer running and returns from appRun call
//
// After the framework has stopped you can call appRun over again.

#pragma once

#include "cppconf/cppconf.h"

#include "core/types.h"

#include <string_view>

namespace Dim {


/****************************************************************************
*
*   Run application
*
***/

class IAppNotify {
public:
    virtual ~IAppNotify() {}

    virtual void onAppRun() = 0;

    // argc & argv are set by the framework before the call to onAppRun() 
    int m_argc;
    char ** m_argv;
};

enum AppFlags : unsigned {
    fAppWithConsole = 0x01,

    fAppWithChdir = 0x02,
    fAppWithFiles = 0x04, // conf, log, etc
    fAppWithWebAdmin = 0x10,

    fAppClient = fAppWithConsole,
    fAppServer = fAppWithChdir | fAppWithFiles | fAppWithWebAdmin,
};

// returns exit code
int appRun(
    IAppNotify & app, 
    int argc, 
    char * argv[], 
    AppFlags flags = fAppClient
);


/****************************************************************************
*
*   Application status
*
***/

RunMode appMode();

// returns flags passed to appRun()
AppFlags appFlags(); 

std::string_view appConfigDirectory();
std::string_view appLogDirectory();
std::string_view appDataDirectory();

// false if file relative to root is not within the root path. This can happen 
// if file breaks out via ".." or is an absolute path.
bool appConfigPath(std::string & out, std::string_view file);
bool appLogPath(std::string & out, std::string_view file);
bool appDataPath(std::string & out, std::string_view file);


/****************************************************************************
*
*   Shutdown application
*
***/

// program exit codes
// On *nix the EX_* constants are defined in <sysexits.h>
enum {
    EX_OK = 0,
    EX_USAGE = 64,       // bad command line
    EX_DATAERR = 65,     // bad input file
    EX_UNAVAILABLE = 69, // failed for some other reason
    EX_SOFTWARE = 70,    // internal software error
    EX_OSERR = 71,       // some os operation failed
    EX_IOERR = 74,       // console or file read/write error
    EX_NOPERM = 77,      // insufficient permission
    EX_CONFIG = 78,      // configuration error

    // Microsoft specific
    EX_ABORTED = 3, // used by CRT in assert() failures and abort()

    // 100+ reserved for application
    EX__APPBASE = 100,
};

// Signals shutdown to begin, the exitcode will be return from appRun() after
// shutdown finishes.
void appSignalShutdown(int exitcode = EX_OK);

// Intended for use with command line errors, calls appSignalShutdown() after
// potentially logging an error. Explicit err text implies EX_USAGE, otherwise 
// exitcode, err, and detail default to being pulled from the global Dim::Cli 
// instance. 
void appSignalUsageError(
    std::string_view err = {},
    std::string_view detail = {});
void appSignalUsageError(
    int exitcode,
    std::string_view err = {},
    std::string_view detail = {});

} // namespace
