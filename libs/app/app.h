// Copyright Glen Knowles 2015 - 2021.
// Distributed under the Boost Software License, Version 1.0.
//
// app.h - dim app
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
//      return, eventually calling appSignalShutdown() in response to some 
//      later event.
// 4. Stopping - process all shutdown monitors and shutdown the framework
// 5. Stopped - framework is no longer running and returns from appRun call
//
// After the framework has stopped you can call appRun over again.

#pragma once

#include "cppconf/cppconf.h"

#include "config.h"
#include "resource.h"

#include "core/process.h"
#include "core/types.h"
#include "file/path.h"

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

    // Since this is called on the event thread, servers should return promptly
    // to allow event processing to continue. This is especially important when
    // running as a service because the Windows SCM requires regular progress
    // reports during startup.
    virtual void onAppRun() = 0;

    // argc & argv are set by the framework before the call to onAppRun()
    int m_argc{};
    char ** m_argv{};
};

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

// Returns exit code.
//
// When running as a Windows service, the state is initially set to
// START_PENDING and only switched to RUNNING after calling either the
// onAppRun() method (for notifier version) or the function (for the function
// version).
int appRun(
    IAppNotify * app,
    int argc,
    char * argv[],
    const VersionInfo & ver = {},
    std::string_view baseName = {}, // defaults to stem of execuable file name
    AppFlags flags = fAppClient
);
int appRun(
    std::function<void(int argc, char *argv[])> fn,
    int argc,
    char * argv[],
    const VersionInfo & ver = {},
    std::string_view baseName = {}, // defaults to stem of execuable file name
    AppFlags flags = fAppClient
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
RunMode appMode();
inline bool appStarting() { return appMode() == kRunStarting; }
inline bool appStopping() { return appMode() == kRunStopping; }

// returns flags passed to appRun()
AppFlags appFlags();

// returns EX_OK unless changed by appSignalShutdown()
int appExitCode();

const Path & appRootDir();
const Path & appInitialDir();
const Path & appBinDir();
const Path & appConfigDir();
const Path & appCrashDir();
const Path & appDataDir();
const Path & appLogDir();

LogType appLogLevel();

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
void appSignalShutdown(int exitcode = EX_OK);

// Intended for use with command line errors, calls appSignalShutdown() after
// reporting the error. Explicit err text implies EX_USAGE, otherwise defaults
// for exit code, err, and detail are pulled from the global Dim::Cli instance.
//
// Unless the application is running as a service, the error is logged to the
// console in addition to any other log targets.
//
// If exit code is EX_PENDING appSignalUsageError() returns immediately without
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

// Returns true if appSignalUsageError() has been called with any code other
// than EX_PENDING.
bool appUsageErrorSignaled();

} // namespace
