// app.h - dim core
#pragma once

#include "config.h"

#include <string_view>

namespace Dim {

// forward declarations
enum RunMode : int;

class IAppNotify {
public:
    virtual ~IAppNotify() {}

    virtual void onAppRun() = 0;

    // argc & argv are set by the framework before the call to onAppRun 
    int m_argc;
    char ** m_argv;
};

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
int appRun(IAppNotify & app, int argc, char * argv[]);

// additional program exit codes
enum {
    // 100+ reserved for application
    EX__APPBASE = 100,
};

void appSignalShutdown(int exitcode = EX_OK);
void appSignalUsageError(
    std::string_view err = {},
    std::string_view detail = {});
void appSignalUsageError(
    int exitcode,
    std::string_view err = {},
    std::string_view detail = {});

void appMonitorShutdown(IAppShutdownNotify * cleanup);
bool appQueryDestroyFailed();

RunMode appMode();

} // namespace
