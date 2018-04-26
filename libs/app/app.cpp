// Copyright Glen Knowles 2015 - 2018.
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
*   Variables
*
***/

static int s_exitcode;

static mutex s_runMut;
static condition_variable s_runCv;
static RunMode s_runMode{kRunStopped};

static IAppNotify * s_app;
static string s_appName;
static vector<ITaskNotify *> s_appTasks;
static AppFlags s_appFlags;
static string s_logDir;
static string s_confDir;
static string s_dataDir;
static string s_crashDir; // where to place crash dumps


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
    char stkbuf[256];
    unique_ptr<char[]> heapbuf;
    auto buf = stkbuf;
    if (msg.size() >= size(stkbuf)) {
        heapbuf.reset(new char[msg.size() + 1]);
        buf = heapbuf.get();
    }
    memcpy(buf, msg.data(), msg.size());
    buf[msg.size()] = '\n';
    if (type == kLogTypeError) {
        ConsoleScopedAttr attr(kConsoleError);
        cout.write(buf, msg.size() + 1);
    } else if (type == kLogTypeWarn) {
        ConsoleScopedAttr attr(kConsoleWarn);
        cout.write(buf, msg.size() + 1);
    } else {
        cout.write(buf, msg.size() + 1);
    }
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
}


/****************************************************************************
*
*   FnProxyAppNotify
*
***/

namespace {
struct FnProxyAppNotify : public IAppNotify {
    function<void(int, char*[])> fn;

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
    if (s_appFlags & fAppWithFiles) {
        iLogFileInitialize();
        if (s_appFlags & fAppWithConsole)
            logMonitor(&s_consoleLogger);
    }
    iPlatformConfigInitialize();
    iSocketInitialize();
    iAppSocketInitialize();
    iSockMgrInitialize();
    iHttpRouteInitialize();
    if (s_appFlags & fAppWithWebAdmin)
        iWebAdminInitialize();

    taskPushEvent(s_appTasks.data(), s_appTasks.size());

    s_runMode = kRunRunning;
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
static bool makeAppPath(
    string * out,
    string_view root,
    string_view file,
    bool createDirIfNotExist
) {
    auto fp = fs::u8path(root.begin(), root.end())
        / fs::u8path(file.begin(), file.end());
    error_code ec;
    fp = fs::canonical(fp, ec);
    *out = fp.u8string();
    replace(out->begin(), out->end(), '\\', '/');
    if (ec
        || out->compare(0, root.size(), root.data(), root.size()) != 0
        || (*out)[root.size()] != '/'
    ) {
        return false;
    }
    if (createDirIfNotExist)
        fs::create_directories(fp.remove_filename());
    return true;
}

//===========================================================================
static string makeAppDir(string_view path) {
    auto fp = fs::current_path() / fs::u8path(path.begin(), path.end());
    error_code ec;
    fp = fs::canonical(fp, ec);
    assert(!ec);
    string out = fp.u8string();
    replace(out.begin(), out.end(), '\\', '/');
    return out;
}

//===========================================================================
const string & Dim::appConfigDir() {
    return s_confDir;
}

//===========================================================================
bool Dim::appConfigPath(string * out, string_view file, bool cine) {
    return makeAppPath(out, appConfigDir(), file, cine);
}

//===========================================================================
const string & Dim::appLogDir() {
    return s_logDir;
}

//===========================================================================
bool Dim::appLogPath(string * out, string_view file, bool cine) {
    return makeAppPath(out, appLogDir(), file, cine);
}

//===========================================================================
const string & Dim::appDataDir() {
    return s_dataDir;
}

//===========================================================================
bool Dim::appDataPath(string * out, string_view file, bool cine) {
    return makeAppPath(out, appDataDir(), file, cine);
}

//===========================================================================
const string & Dim::appCrashDir() {
    return s_crashDir;
}

//===========================================================================
bool Dim::appCrashPath(string * out, string_view file, bool cine) {
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

    auto exeName = (Path) envExecPath();
    s_appName = exeName.stem();

    if (flags & fAppWithChdir) {
        auto fp = fs::u8path(exeName.c_str());
        fs::current_path(fp.parent_path());
    }
    s_confDir = makeAppDir("conf");
    s_logDir = makeAppDir("log");
    s_dataDir = makeAppDir("data");
    s_crashDir = makeAppDir("crash");

    iPerfInitialize();
    iLogInitialize();
    iConsoleInitialize();
    if (flags & fAppWithConsole)
        logDefaultMonitor(&s_consoleLogger);
    iTaskInitialize();
    iTimerInitialize();

    s_app = app;
    s_app->m_argc = argc;
    s_app->m_argv = argv;
    taskPushEvent(&s_runTask);

    unique_lock<mutex> lk{s_runMut};
    while (s_runMode == kRunStarting || s_runMode == kRunRunning)
        s_runCv.wait(lk);

    iShutdownDestroy();

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
    s_runMode = kRunStopped;
    return s_exitcode;
}

//===========================================================================
void Dim::appSignalShutdown(int exitcode) {
    assert(s_runMode != kRunStopped);
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
    if (code == EX_PENDING)
        return;

    if (code) {
        bool console = !(appFlags() & (fAppWithConsole | fAppIsService));
        if (console)
            logMonitor(&s_consoleLogger);
        Cli cli;
        auto em = err.empty() ? cli.errMsg() : err;
        auto dm = detail.empty() ? cli.errDetail() : detail;
        if (!em.empty())
            logMsgError() << "Error: " << em;
        if (!dm.empty())
            logMsgInfo() << dm;
        {
            auto os = logMsgInfo();
            cli.printUsageEx(os, {}, cli.runCommand());
        }
        if (console)
            logMonitorClose(&s_consoleLogger);
    }
    appSignalShutdown(code);
}
