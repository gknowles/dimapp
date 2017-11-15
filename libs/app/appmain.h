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
//      return, calling appSignalShutdown() only in response to some later
//      event.
// 4. Stopping - process all shutdown monitors and shutdown the framework
// 5. Stopped - framework is no longer running and returns from appRun call
//
// After the framework has stopped you can call appRun over again.

#pragma once

#include "cppconf/cppconf.h"

#include "core/types.h"

#include <functional>
#include <string_view>

namespace Dim {


/****************************************************************************
*
*   Run application
*
***/

class IAppNotify {
public:
    virtual ~IAppNotify() = default;

    // Since this is called on the event thread, servers (especially when
    // running as a service) should return promptly to allow event processing
    // to continue.
    virtual void onAppRun() = 0;

    // argc & argv are set by the framework before the call to onAppRun()
    int m_argc;
    char ** m_argv;
};

enum AppFlags : unsigned {
    fAppWithChdir = 0x01,
    fAppWithFiles = 0x02, // conf, log, etc
    fAppWithWebAdmin = 0x04,
    fAppWithConsole = 0x10,
    fAppWithGui = 0x20,
    fAppWithService = 0x40, // can run as service
    fAppIsService = 0x100, // is running as service

    fAppClient = fAppWithConsole | fAppWithGui,
    fAppServer = fAppWithChdir | fAppWithFiles | fAppWithWebAdmin
        | fAppWithGui | fAppWithService,

    fAppReadOnlyFlags = fAppIsService,
};

// Returns exit code.
//
// When running as a Windows service, the state is initially set to
// START_PENDING and only switched to RUNNING after onAppRun returns.
int appRun(
    IAppNotify & app,
    int argc,
    char * argv[],
    AppFlags flags = fAppClient
);
int appRun(
    std::function<void(int argc, char *argv[])> fn,
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
inline bool appStarting() { return appMode() == kRunStarting; }
inline bool appStopping() { return appMode() == kRunStopping; }

// returns flags passed to appRun()
AppFlags appFlags();

std::string_view appConfigDirectory();
std::string_view appLogDirectory();
std::string_view appDataDirectory();

// false if file relative to root is not within the root path. This can happen
// if file breaks out via ".." or is an absolute path.
bool appConfigPath(
    std::string & out,
    std::string_view file,
    bool createIfNotExist = true
);
bool appLogPath(std::string & out, std::string_view file, bool cine = true);
bool appDataPath(std::string & out, std::string_view file, bool cine = true);


/****************************************************************************
*
*   Shutdown application
*
***/

// program exit codes
enum {
    EX_OK = 0,

    // Microsoft specific
    EX_ABORTED = 3, // used by CRT in assert() failures and abort()

    // Start of exit codes defined by this application
    EX__APPBASE = 10,

    // Exit code is being set by an asynchronously event that hasn't completed,
    // prevents appSignalUsageError() from taking action.
    EX_PENDING = 63,

    // On *nix the following EX_* constants are defined in <sysexits.h>
    // These are centered around why a command line utility might fail.
    EX__BASE = 64,       // base value for standard error codes
    EX_USAGE = 64,       // bad command line
    EX_DATAERR = 65,     // bad input file
    EX_NOINPUT = 66,     // missing or inaccessible input file
    EX_NOUSER = 67,      // user doesn't exist
    EX_NOHOST = 68,      // host doesn't exist (not merely connect failed)
    EX_UNAVAILABLE = 69, // failed for some other reason
    EX_SOFTWARE = 70,    // internal software error
    EX_OSERR = 71,       // some OS operation failed
    EX_OSFILE = 72,      // some operation on a system file failed
    EX_CANTCREAT = 73,   // can't create user supplied output file
    EX_IOERR = 74,       // console or file read/write error
    EX_TEMPFAIL = 75,    // temporary failure, trying again later may work
    EX_PROTOCOL = 76,    // illegal response from remote system
    EX_NOPERM = 77,      // insufficient permission
    EX_CONFIG = 78,      // configuration error
};

// Signals shutdown to begin, the exitcode will be returned from appRun()
// after shutdown finishes.
//
// When running as a Windows service, a call to set SERVICE_STOP_PENDING
// is immediately queued, with a change to SERVICE_STOPPED coming only after
// all application shutdown handlers (see shutdownMonitor) have completed.
void appSignalShutdown(int exitcode = EX_OK);

// Intended for use with command line errors, calls appSignalShutdown() after
// reporting the error. Explicit err text implies EX_USAGE, otherwise defaults
// for exitcode, err, and detail are pulled from the global Dim::Cli instance.
//
// Unless the application is running as a service, the error is logged to the
// console in addition to any other log targets.
//
// If exitcode is EX_PENDING appSignalUsageError() returns immediately without
// doing anything.
void appSignalUsageError(
    std::string_view err = {},
    std::string_view detail = {}
);
void appSignalUsageError(
    int exitcode,
    std::string_view err = {},
    std::string_view detail = {}
);

} // namespace
