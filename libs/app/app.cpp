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



/****************************************************************************
*
*   Variables
*
***/

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

namespace {
class RunTask : public ITaskNotify {
    void onTask() override;
};
} // namespace
static RunTask s_runTask;

//===========================================================================
void RunTask::onTask() {
    s_app->onAppRun();
}


/****************************************************************************
*
*   ConsoleLogger
*
***/

namespace {
class ConsoleLogger : public ILogNotify {
    void onLog(LogType type, string_view msg) override;
};
} // namespace
static ConsoleLogger s_consoleLogger;

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
*   app.xml monitor
*
***/

namespace {
class ConfigAppXml : public IConfigNotify {
    void onConfigChange(const XDocument & doc) override;
};
} // namespace
static ConfigAppXml s_appXml;

//===========================================================================
void ConfigAppXml::onConfigChange(const XDocument & doc) {
    shutdownDisableTimeout(configUnsigned(doc, "DisableShutdownTimeout"));
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
    iPerfInitialize();
    iLogInitialize();
    iConsoleInitialize();
    if (flags & fAppWithConsole)
        logDefaultMonitor(&s_consoleLogger);
    if (flags & fAppWithChdir) {
        auto fp = fs::u8path(envGetExecPath());
        fs::current_path(fp.parent_path());
    }
    iTaskInitialize();
    iTimerInitialize();
    iPlatformInitialize();
    iFileInitialize();
    iConfigInitialize("conf");
    configMonitor("app.xml", &s_appXml);
    if (flags & fAppWithLogFiles) {
        iLogFileInitialize("log");
        if (flags & fAppWithConsole)
            logMonitor(&s_consoleLogger);
    }
    iPlatformConfigInitialize();
    iSocketInitialize();
    iAppSocketInitialize();
    iSockMgrInitialize();
    iHttpRouteInitialize();
    if (flags & fAppWithWebAdmin)
        iWebAdminInitialize();

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
    iLogDestroy();
    iPerfDestroy();
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
