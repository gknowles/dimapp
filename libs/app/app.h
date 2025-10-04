// Copyright Glen Knowles 2015 - 2025. Distributed under the Boost Software
// License, Version 1.0.
//
// app.h - dim app
//
// Application life-cycle
//
// 1. Stopped - application configures command line options via Cli and
//      performs any other initialization, and then calls appRun, usually from
//      main(), to start the framework.
// 2. Starting - framework initialization, state never seen by application.
// 3. Running - framework calls Cli::exec() on the event thread. Application
//      then does stuff, and eventually calls appSignalShutdown(). A command
//      line tool might do all its work and signal shutdown from inside the
//      exec handler. Whereas a server might start listening for socket
//      connections and return, eventually calling appSignalShutdown() in
//      response to some later event. For windows services the state is changed
//      from PENDING_START to RUNNING only after the exec handler returns.
// 4. Stopping - triggered by appSignalShutdown(), process all shutdown
//      monitors and shutdown the framework.
// 5. Stopped - framework is no longer running and appRun returns with the exit
//      code passed to appSignalShutdown().
//
// After the framework has stopped you can call appRun over again.

#pragma once

#include "cppconf/cppconf.h"

#include "config.h"
#include "resource.h"
#include "webadmin.h"

#include "basic/types.h"
#include "core/process.h"
#include "file/path.h"

#include <functional>
#include <string_view>

namespace Dim {


/****************************************************************************
*
*   Run application
*
***/

enum AppFlags : unsigned {
    fAppWithChdir = 0x01,

    // Has root with bin, conf, crash, log, etc subdirs.
    fAppWithFiles = 0x02,

    // Writes crash and memory leak dumps.
    fAppWithDumps = 0x08,

    // Writes logs to log/* files. Does perf dump to logs at exit.
    fAppWithLogs = 0x04,

    fAppWithWebAdmin = 0x10,
    fAppWithConsole = 0x20,
    fAppWithGui = 0x40,
    fAppWithService = 0x80, // can run as service
    fAppIsService = 0x100, // is running as service

    fAppUtility = fAppWithConsole | fAppWithGui,
    fAppClient = fAppWithFiles | fAppWithDumps | fAppWithConsole
        | fAppWithGui,
    fAppServer = fAppWithChdir | fAppWithFiles | fAppWithDumps | fAppWithLogs
        | fAppWithWebAdmin | fAppWithGui | fAppWithService,

    fAppReadOnlyFlags = fAppIsService,
};

// Before calling appRun, the application must configure (via Cli) subcommands
// and/or the default cli.action().
//
// The default action (or subcommand) is called by the framework on the event
// thread, therefore servers should spawn whatever tasks they need then return
// promptly to allow event processing to continue. This is especially important
// when running as a service because the Windows SCM requires regular progress
// reports during startup.
//
// When running as a Windows service, the state is initially set to
// START_PENDING and only switched to RUNNING after the exec action has
// returned.
//
// For a command line utility that doesn't do any asynchronous work it is fine
// to do all of the work in the action handler and then call
// appSignalShutdown() just before returning.
int appRun(
    int argc,
    char * argv[],
    const VersionInfo & ver = {},   // defaults to envExecVersion()
    std::string_view baseName = {}, // defaults to stem of executable file name
    EnumFlags<AppFlags> flags = fAppUtility
);


/****************************************************************************
*
*   Application status
*
***/

const std::string & appName();
const std::string & appBaseName();
const VersionInfo & appVersion();
unsigned appIndex();

// returns flags passed to appRun()
EnumFlags<AppFlags> appFlags();

// Used for logging and preferred address for outbound connections.
SockAddr appAddress();

LogType appLogLevel();
unsigned appGroupIndex();
std::string appGroupType();

RunMode appMode();
inline bool appStarting() { return appMode() == kRunStarting; }
inline bool appStopping() { return appMode() == kRunStopping; }

// returns EX_OK unless changed by appSignalShutdown()
int appExitCode();

// Application directory structure
// If bin dir name is "bin":
//  root
//      /bin
//      /conf
//      /crash
//      /data
//      /log
//      /web
//
// otherwise:
//  <binDir> (is also config dir)
//  LOCALAPPDATA/<AppName>
//      /crash
//      /data
//      /log
//      /web

const Path & appRootDir();      // application root
const Path & appInitialDir();   // current directory when appRun was called
const Path & appBinDir();       // directory containing this binary
const Path & appConfigDir();
const Path & appCrashDir();
const Path & appDataDir();
const Path & appLogDir();

// False if file relative to root is not within the root path. This can happen
// if file breaks out via ".." or is an absolute path.
bool appConfigPath(
    Path * out,
    std::string_view file,
    bool createDirIfNotExist = true
);
bool appCrashPath(
    Path * out,
    std::string_view file,
    bool createDirIfNotExist = true
);
bool appDataPath(
    Path * out,
    std::string_view file,
    bool createDirIfNotExist = true
);
bool appLogPath(
    Path * out,
    std::string_view file,
    bool createDirIfNotExist = true
);


/****************************************************************************
*
*   Shutdown application
*
***/

// Signals shutdown to begin, the exit code will be returned from appRun()
// after shutdown finishes.
//
// When running as a Windows service, a call to set SERVICE_STOP_PENDING
// is immediately queued, with a change to SERVICE_STOPPED coming only after
// all application shutdown handlers (see shutdownMonitor) have completed.
void appSignalShutdown(int exitCode = EX_OK);

// Returns true if appSignalShutdown() has been called because the
// application's action or subcommand returned without calling
// cli.fail(EX_PENDING) or otherwise setting the cli exit code to EX_PENDING.
bool appCliShutdownSignaled();

} // namespace
