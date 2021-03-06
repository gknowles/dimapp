// Copyright Glen Knowles 2015 - 2021.
// Distributed under the Boost Software License, Version 1.0.
//
// app.cpp - dim app
#include "pch.h"
#pragma hdrstop

using namespace std;
using namespace std::chrono;
using namespace Dim;


/****************************************************************************
*
*   Variables
*
***/

static int s_exitcode;
static bool s_usageErrorSignaled;

static mutex s_runMut;
static condition_variable s_runCv;
static RunMode s_runMode{kRunStopped};

static IAppNotify * s_app;
static string s_appBaseName;
static unsigned s_appIndex;
static string s_appName;    // appBaseName[<appIndex>], if appIndex > 1
static vector<ITaskNotify *> s_appTasks;
static AppFlags s_appFlags;
static Path s_initialDir;
static Path s_rootDir;
static Path s_binDir;
static Path s_confDir;
static Path s_crashDir; // where to place crash dumps
static Path s_dataDir;
static Path s_logDir;


/****************************************************************************
*
*   Helpers
*
***/

//===========================================================================
static bool makeAppPath(
    Path * out,
    string_view root,
    string_view file,
    bool createDirIfNotExist
) {
    *out = Path(root) / file;
    if (out->str().compare(0, root.size(), root.data(), root.size()) != 0
        || out->str()[root.size()] != '/'
    ) {
        return false;
    }
    if (createDirIfNotExist) {
        auto fp = *out;
        fileCreateDirs(fp.removeFilename());
    }
    return true;
}

//===========================================================================
static Path makeAppDir(string_view path) {
    auto out = appRootDir() / path;
    return out;
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
    shutdownDisableTimeout(configNumber(doc, "DisableShutdownTimeout"));
    s_logDir = makeAppDir(configString(doc, "LogDir", "log"));
    s_dataDir = makeAppDir(configString(doc, "DataDir", "data"));
}


/****************************************************************************
*
*   FnProxyAppNotify
*
***/

namespace {
struct FnProxyAppNotify : public IAppNotify {
    function<void(int, char *[])> fn;

    void onAppRun() override;
};
} // namespace
static FnProxyAppNotify s_fnProxyApp;

//===========================================================================
void FnProxyAppNotify::onAppRun() {
    fn(m_argc, m_argv);
}


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
    iPlatformInitialize();
    iFileInitialize();
    iConfigInitialize();
    configMonitor("app.xml", &s_appXml);
    if (s_appFlags & fAppWithLogs) {
        iLogFileInitialize();
        if (s_appFlags & fAppWithConsole)
            logMonitor(consoleBasicLogger());
    }
    iPlatformConfigInitialize();
    iPipeInitialize();
    iSocketInitialize();
    iAppSocketInitialize();
    iSockMgrInitialize();
    iHttpRouteInitialize();
    iWebAdminInitialize();

    taskPushEvent(s_appTasks.data(), s_appTasks.size());

    {
        unique_lock lk{s_runMut};
        if (s_runMode == kRunStopping) {
            // One of the initialization tasks called appSignalShutdown()
            // moving mode to stopping, so we don't want to move backward
            // to running.
        } else {
            assert(s_runMode == kRunStarting);
            s_runMode = kRunRunning;
        }
    }

    s_app->onAppRun();
}


/****************************************************************************
*
*   Internal API
*
***/

//===========================================================================
void Dim::iAppPushStartupTask(ITaskNotify * task) {
    assert(appStarting());
    s_appTasks.push_back(task);
}

//===========================================================================
void Dim::iAppSetFlags(AppFlags flags) {
    assert(appStarting());
    s_appFlags = flags;
}


/****************************************************************************
*
*   Public API
*
***/

//===========================================================================
const string & Dim::appBaseName() {
    return s_appBaseName;
}

//===========================================================================
unsigned Dim::appIndex() {
    return s_appIndex;
}

//===========================================================================
const string & Dim::appName() {
    return s_appName;
}

//===========================================================================
RunMode Dim::appMode() {
    return s_runMode;
}

//===========================================================================
AppFlags Dim::appFlags() {
    return s_appFlags;
}

//===========================================================================
int Dim::appExitCode() {
    return s_exitcode;
}

//===========================================================================
bool Dim::appUsageErrorSignaled() {
    return s_usageErrorSignaled;
}

//===========================================================================
const Path & Dim::appRootDir() {
    return s_rootDir;
}

//===========================================================================
const Path & Dim::appConfigDir() {
    return s_confDir;
}

//===========================================================================
bool Dim::appConfigPath(Path * out, string_view file, bool cine) {
    return makeAppPath(out, appConfigDir(), file, cine);
}

//===========================================================================
const Path & Dim::appLogDir() {
    return s_logDir;
}

//===========================================================================
bool Dim::appLogPath(Path * out, string_view file, bool cine) {
    return makeAppPath(out, appLogDir(), file, cine);
}

//===========================================================================
const Path & Dim::appDataDir() {
    return s_dataDir;
}

//===========================================================================
bool Dim::appDataPath(Path * out, string_view file, bool cine) {
    return makeAppPath(out, appDataDir(), file, cine);
}

//===========================================================================
const Path & Dim::appCrashDir() {
    return s_crashDir;
}

//===========================================================================
bool Dim::appCrashPath(Path * out, string_view file, bool cine) {
    return makeAppPath(out, appCrashDir(), file, cine);
}

//===========================================================================
int Dim::appRun(
    function<void(int argc, char *argv[])> fn,
    int argc,
    char * argv[],
    AppFlags flags
) {
    s_fnProxyApp.fn = move(fn);
    return appRun(&s_fnProxyApp, argc, argv, flags);
}

//===========================================================================
int Dim::appRun(IAppNotify * app, int argc, char * argv[], AppFlags flags) {
    s_appFlags = flags;
    s_runMode = kRunStarting;
    s_appTasks.clear();
    s_exitcode = 0;

    if (s_appFlags & fAppWithService) {
        Cli cli;
        cli.opt(&s_appIndex, "app-index", 1)
            .desc("Identifies service when multiple are configured.");

        // The command line will be validated later by the application, right
        // now we just need to get the appIndex before processing the
        // configuration.
        (void) cli.parse(argc, argv);
    }

    s_initialDir = fileGetCurrentDir();
    auto exeName = (Path) envExecPath();
    s_appBaseName = exeName.stem();
    s_appName = s_appBaseName;
    if (s_appIndex > 1)
        s_appName += StrFrom<unsigned>(s_appIndex);
    s_binDir = exeName.removeFilename();
    if ((flags & fAppWithFiles) && s_binDir.stem() == "bin") {
        s_rootDir = s_binDir.parentPath();
        s_confDir = makeAppDir("conf");
    } else {
        s_rootDir = Path(envProcessLogDataDir()) / s_appName;
        s_confDir = s_binDir;
    }
    s_crashDir = makeAppDir("crash");
    s_dataDir = makeAppDir("data");
    s_logDir = makeAppDir("log");

    if (flags & fAppWithChdir)
        fileSetCurrentDir(s_rootDir);

    iPerfInitialize();
    iLogInitialize();
    iConsoleInitialize();
    if (flags & fAppWithConsole)
        logDefaultMonitor(consoleBasicLogger());
    iTaskInitialize();
    iTimerInitialize();

    s_app = app;
    s_app->m_argc = argc;
    s_app->m_argv = argv;
    taskPushEvent(&s_runTask);

    unique_lock lk{s_runMut};
    while (s_runMode == kRunStarting || s_runMode == kRunRunning)
        s_runCv.wait(lk);
    lk.unlock();

    iShutdownDestroy();
    iConsoleDestroy();

    //-----------------------------------------------------------------------
    // No external effects should happen after this point. Any sockets, pipes,
    // files, or other shared resources MUST ALREADY be closed. When running
    // as a service the SERVICE_STOPPED status has been reported and the
    // Service Control Manager may have already suspended us indefinitely as
    // it spun up another instance of the service.
    //-----------------------------------------------------------------------

    iTimerDestroy();
    iTaskDestroy();
    iLogDestroy();
    iPerfDestroy();
    lk.lock();
    s_runMode = kRunStopped;
    return s_exitcode;
}

//===========================================================================
void Dim::appSignalShutdown(int exitcode) {
    assert(s_runMode != kRunStopped);
    if (exitcode > s_exitcode)
        s_exitcode = exitcode;

    {
        unique_lock lk{s_runMut};
        s_runMode = kRunStopping;
    }
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
    if (code == EX_PENDING)
        return;

    s_usageErrorSignaled = true;
    if (code) {
        bool hasConsole = appFlags() & (fAppWithConsole | fAppIsService);
        if (!hasConsole)
            logMonitor(consoleBasicLogger());
        Cli cli;
        auto em = err.empty() ? cli.errMsg() : err;
        auto dm = detail.empty() ? cli.errDetail() : detail;
        if (!em.empty())
            logMsgError() << "Error: " << em;
        if (!dm.empty())
            logMultiInfo(dm);
        ostringstream os;
        cli.printUsageEx(os, {}, cli.commandMatched());
        logMultiInfo(os.view());
        if (!hasConsole)
            logMonitorClose(consoleBasicLogger());
    }
    appSignalShutdown(code);
}
