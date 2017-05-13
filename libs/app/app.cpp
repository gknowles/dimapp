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
*   Variables
*
***/

static int s_exitcode;

static mutex s_runMut;
static condition_variable s_runCv;
static RunMode s_runMode{kRunStopped};

static IAppNotify * s_app;
static vector<ITaskNotify *> s_appTasks;
static AppFlags s_appFlags;
static string s_logDir;
static string s_confDir;
static string s_dataDir;


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
*   Internal API
*
***/

//===========================================================================
void Dim::iAppPushStartupTask(ITaskNotify & task) {
    assert(appStarting());
    s_appTasks.push_back(&task);
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
RunMode Dim::appMode() {
    return s_runMode;
}

//===========================================================================
AppFlags Dim::appFlags() {
    return s_appFlags;
}

//===========================================================================
static bool makeAppPath(
    string & out, 
    string_view root, 
    string_view file,
    bool createIfNotExist
) {
    auto fp = fs::u8path(root.begin(), root.end()) 
        / fs::u8path(file.begin(), file.end());
    error_code ec;
    fp = fs::canonical(fp, ec);
    out = fp.u8string();
    replace(out.begin(), out.end(), '\\', '/');
    if (ec 
        || out.compare(0, root.size(), root.data(), root.size()) != 0
        || out[root.size()] != '/'
    ) {
        return false;
    }
    if (createIfNotExist)
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
string_view Dim::appConfigDirectory() {
    return s_confDir;
}

//===========================================================================
string_view Dim::appLogDirectory() {
    return s_logDir;
}

//===========================================================================
string_view Dim::appDataDirectory() {
    return s_dataDir;
}

//===========================================================================
bool Dim::appConfigPath(string & out, string_view file, bool cine) {
    return makeAppPath(out, appConfigDirectory(), file, cine);
}

//===========================================================================
bool Dim::appLogPath(string & out, string_view file, bool cine) {
    return makeAppPath(out, appLogDirectory(), file, cine);
}

//===========================================================================
bool Dim::appDataPath(string & out, string_view file, bool cine) {
    return makeAppPath(out, appDataDirectory(), file, cine);
}

//===========================================================================
int Dim::appRun(IAppNotify & app, int argc, char * argv[], AppFlags flags) {
    s_appFlags = flags;
    s_runMode = kRunStarting;
    s_appTasks.clear();
    s_appTasks.push_back(&s_runTask);

    if (flags & fAppWithChdir) {
        auto fp = fs::u8path(envGetExecPath());
        fs::current_path(fp.parent_path());
    }
    s_confDir = makeAppDir("conf");
    s_logDir = makeAppDir("log");
    s_dataDir = makeAppDir("data");

    iPerfInitialize();
    iLogInitialize();
    iConsoleInitialize();
    if (flags & fAppWithConsole)
        logDefaultMonitor(&s_consoleLogger);
    iTaskInitialize();
    iTimerInitialize();
    iPlatformInitialize();
    iFileInitialize();
    iConfigInitialize();
    configMonitor("app.xml", &s_appXml);
    if (flags & fAppWithFiles) {
        iLogFileInitialize();
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
    taskPushEvent(s_appTasks.data(), s_appTasks.size());

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
