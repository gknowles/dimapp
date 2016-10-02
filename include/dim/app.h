// app.h - dim core
#pragma once

#include "dim/config.h"

namespace Dim {

// forward declarations
enum RunMode;
class ITaskNotify;

class IAppShutdownNotify {
public:
    virtual ~IAppShutdownNotify() {}

    virtual void onAppStartClientCleanup() {}
    virtual bool onAppQueryClientDestroy() { return true; }
    virtual void onAppStartServerCleanup() {}
    virtual bool onAppQueryServerDestroy() { return true; }
    virtual void onAppStartConsoleCleanup() {}
    virtual bool onAppQueryConsoleDestroy() { return true; }
};

// returns exit code
int appRun(ITaskNotify & app);

// program exit codes
#ifndef EX_OK
enum {
    EX_OK = 0, // success

    // 1-2 available for application

    EX_ABORTED = 3, // assert() failure or direct call to abort()

    // 4-63 available for application

    // 64-78 are borrowed from unix's sysexits.h
    EX_USAGE = 64,       // bad command line
    EX_DATAERR = 65,     // bad input file
    EX_UNAVAILABLE = 69, // failed for some other reason
    EX_SOFTWARE = 70,    // internal software error
    EX_OSERR = 71,       // some os operation failed
    EX_IOERR = 74,       // console or file read/write error
    EX_NOPERM = 77,      // insufficient permission
    EX_CONFIG = 78,      // configuration error

    // 100+ reserved for application
    EX__APPBASE = 100,
};
#endif

void appSignalShutdown(int exitcode = EX_OK);

void appMonitorShutdown(IAppShutdownNotify * cleanup);
bool appQueryDestroyFailed();

RunMode appMode();

} // namespace
