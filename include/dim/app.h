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

enum {
    kExitSuccess = 0,
    kExitBadArgs = 1,
    kExitCtrlBreak = 2,

    // first available for use by application
    kExitFirstAvailable
};
void appSignalShutdown(int exitcode = kExitSuccess);

void appMonitorShutdown(IAppShutdownNotify * cleanup);
bool appQueryDestroyFailed();

RunMode appMode();

} // namespace
