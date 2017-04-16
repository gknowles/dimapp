// Copyright Glen Knowles 2015 - 2017.
// Distributed under the Boost Software License, Version 1.0.
//
// app.cpp - dim app
#include "pch.h"
#pragma hdrstop

using namespace std;
using namespace std::chrono;
using namespace Dim;
namespace fs = std::experimental::filesystem;


/****************************************************************************
*
*   Declarations
*
***/

namespace {

class RunTask : public ITaskNotify {
public:
    // ITaskNotify
    void onTask() override;
};

class ConsoleLogger : public ILogNotify {
    void onLog(LogType type, string_view msg) override;
};

} // namespace


/****************************************************************************
*
*   Variables
*
***/

static RunTask s_runTask;
static ConsoleLogger s_consoleLogger;
static int s_exitcode;

static mutex s_runMut;
static condition_variable s_runCv;
static RunMode s_runMode{kRunStopped};

static IAppNotify * s_app;
static AppFlags s_appFlags;

/****************************************************************************
*
*   RunTask
*
***/

//===========================================================================
void RunTask::onTask() {
    s_app->onAppRun();
}


/****************************************************************************
*
*   ConsoleLogger
*
***/

//===========================================================================
void ConsoleLogger::onLog(LogType type, string_view msg) {
    if (type == kLogTypeError) {
        ConsoleScopedAttr attr(kConsoleError);
        cout.write(msg.data(), msg.size());
    } else {
        cout.write(msg.data(), msg.size());
    }
    cout << endl;
}


/****************************************************************************
*
*   Public API
*
***/

//===========================================================================
RunMode Dim::appMode() {
    return s_runMode;
}

//===========================================================================
AppFlags Dim::appRunFlags() {
    return s_appFlags;
}

//===========================================================================
int Dim::appRun(IAppNotify & app, int argc, char * argv[], AppFlags flags) {
    s_appFlags = flags;
    s_runMode = kRunStarting;
    if (flags & fAppWithConsole)
        logDefaultMonitor(&s_consoleLogger);
    iConsoleInitialize();
    if (flags & fAppWithChdir) {
        auto fp = fs::u8path(envGetExecPath());
        fs::current_path(fp.parent_path());
    }
    iTaskInitialize();
    iTimerInitialize();
    iPlatformInitialize();
    iFileInitialize();
    if (flags & fAppWithConfig)
        iAppConfigInitialize("conf");
    iSocketInitialize();
    iAppSocketInitialize();
    iHttpRouteInitialize();

    s_app = &app;
    s_app->m_argc = argc;
    s_app->m_argv = argv;
    s_runMode = kRunRunning;
    taskPushEvent(s_runTask);

    unique_lock<mutex> lk{s_runMut};
    while (s_runMode == kRunRunning)
        s_runCv.wait(lk);

    iShutdownDestroy();
    iTimerDestroy();
    iTaskDestroy();
    s_runMode = kRunStopped;
    return s_exitcode;
}

//===========================================================================
void Dim::appSignalShutdown(int exitcode) {
    if (exitcode > s_exitcode)
        s_exitcode = exitcode;
    s_runMode = kRunStopping;
    s_runCv.notify_one();
}

//===========================================================================
void Dim::appSignalUsageError(string_view err, string_view detail) {
    if (!err.empty())
        return appSignalUsageError(EX_USAGE, err, detail);
    Cli cli;
    appSignalUsageError(cli.exitCode(), err, detail);
}

//===========================================================================
void Dim::appSignalUsageError(int code, string_view err, string_view detail) {
    if (code) {
        Cli cli;
        auto em = err.empty() ? cli.errMsg() : err;
        auto dm = detail.empty() ? cli.errDetail() : detail;
        if (!em.empty())
            logMsgError() << "Error: " << em;
        if (!dm.empty())
            logMsgInfo() << dm;
        auto os = logMsgInfo();
        cli.printUsageEx(os);
    }
    return appSignalShutdown(code);
}
