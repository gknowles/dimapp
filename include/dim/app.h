// app.h - dim core
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

#include "config.h"

#include <string_view>

namespace Dim {

// forward declarations
enum RunMode : int;


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

// returns exit code
int appRun(IAppNotify & app, int argc, char * argv[]);


/****************************************************************************
*
*   Application status
*
***/

RunMode appMode();


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
    EX_ABORTED = 3, // assert() failure or direct call to abort()

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


//===========================================================================
// Monitor shutdown
// 
// Application shutdown happens in three phases, client, server, and finally 
// console. The intent is that client connections are shutdown, then 
// connections to servers used to process client requests, and finally 
// consoles (web, text, etc) used to monitor the server.
// 
// For each phase one call is made to every onApp*Shutdown() handler with 
// retry set to false. Each handler begins shutting down and returns false if 
// it doesn't finish immediately. After the first call each handler that 
// returned false, will be called periodically, with retry true, until it 
// returns true. This is done in reverse order of registration, only moving on 
// to the next handler after the current one succeeds. The phase ends after 
// all handlers have returned true.
//
// After a handler returns true it will not be called again. When returning 
// false handlers should return appShutdownFailed(), which is always false, 
// and helps track shutdown problems.
//
// Do not block in the handler, as it prevents timers from running and things 
// from shutting down in parallel. This is especially important when it 
// involves file, socket, or other systems that may block in the OS. 
//
// NOTE: If the shutdown process takes too long (>2 minutes) the process
//       is terminated. This can be delayed with appDelayShutdown().
//===========================================================================
class IAppShutdownNotify {
public:
    virtual ~IAppShutdownNotify() {}

    virtual bool onAppClientShutdown(bool retry) { return true; }
    virtual bool onAppServerShutdown(bool retry) { return true; }
    virtual bool onAppConsoleShutdown(bool retry) { return true; }
};

// Used to register shutdown handlers
void appMonitorShutdown(IAppShutdownNotify * cleanup);

// Helps to track shutdown problems, always returns false. Called from inside 
// shutdown handlers when they are returning false. 
bool appShutdownFailed();

// Reset shutdown timeout back to 2 minutes from now. Use with caution, called
// repeatedly shutdown can be delayed indefinitely.
void appDelayShutdown();

} // namespace
