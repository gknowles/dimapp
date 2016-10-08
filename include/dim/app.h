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

// additional program exit codes
enum {
    // 100+ reserved for application
    EX__APPBASE = 100,
};

void appSignalShutdown(int exitcode = EX_OK);

void appMonitorShutdown(IAppShutdownNotify * cleanup);
bool appQueryDestroyFailed();

RunMode appMode();

} // namespace
