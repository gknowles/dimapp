// Copyright Glen Knowles 2015 - 2022.
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

static atomic_flag s_usageErrorSignaled;

static mutex s_runMut;
static condition_variable s_runCv;
static RunMode s_runMode{kRunStopped};
static int s_exitcode;

static IAppNotify * s_app;
static string s_appBaseName;
static VersionInfo s_appVer;
static unsigned s_appIndex;
static string s_appName;    // appBaseName[<appIndex>], if appIndex > 1
static unsigned s_groupIndex;
static string s_groupType;
static vector<ITaskNotify *> s_appTasks;
static EnumFlags<AppFlags> s_appFlags;
static SockAddr s_appAddr;  // preferred for outbound connections and logging
static Path s_initialDir;
static Path s_rootDir;
static Path s_binDir;   // location of running binary
static Path s_confDir;
static Path s_crashDir; // where to place crash dumps
static Path s_dataDir;
static Path s_logDir;   // where to place log files
static Path s_webDir;


/****************************************************************************
*
*   Helpers
*
***/

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
    s_appAddr.port = (unsigned) configNumber(doc, "Port", 41000);
    s_groupType = configString(doc, "GroupType", "local");
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
*   Initialize Application
*
***/

//===========================================================================
static void initVars() {
    // Application name and version
    fileGetCurrentDir(&s_initialDir);
    auto exeName = (Path) envExecPath();
    if (s_appBaseName.empty())
        s_appBaseName = exeName.stem();
    if (!s_appVer)
        s_appVer = envExecVersion();
    if (s_appVer) {
        Cli cli;
        ostringstream hdr;
        Time8601Str ds(envExecBuildTime());
        auto verStr = toString(s_appVer);
        hdr << s_appBaseName
            << " v" << verStr
            << " (" << ds.view().substr(0, 10) << ")";
        cli.header(hdr.str())
            .versionOpt(verStr, s_appBaseName);
    }
    s_appName = s_appBaseName;
    if (s_appIndex > 1)
        s_appName += to_string(s_appIndex);

    // Directories
    s_binDir = exeName.parentPath();
    if (s_appFlags.any(fAppWithFiles) && s_binDir.stem() == "bin") {
        s_rootDir = s_binDir.parentPath();
        s_confDir = makeAppDir("conf");
    } else {
        s_rootDir = Path(envProcessLogDataDir()) / s_appName;
        s_confDir = s_binDir;
    }
    s_crashDir = makeAppDir("crash");
    s_dataDir = makeAppDir("data");
    s_logDir = makeAppDir("log");
    s_webDir = makeAppDir("web");

    if (s_appFlags.any(fAppWithChdir))
        fileSetCurrentDir(s_rootDir);

    // Address
    vector<HostAddr> addrs;
    addressGetLocal(&addrs);
    s_appAddr.addr = addrs[0];
}

//===========================================================================
static void initApp() {
    iPlatformInitialize(PlatformInit::kBeforeAppVars);
    if (s_appFlags.all(fAppWithLogs | fAppWithConsole))
        logMonitor(consoleBasicLogger());
    iFileInitialize();
    initVars();
    iConfigInitialize();
    configMonitor("app.xml", &s_appXml);
    if (s_appFlags.any(fAppWithLogs))
        iLogFileInitialize();
    iPlatformInitialize(PlatformInit::kAfterAppVars);
    iAppPerfInitialize();
    iPipeInitialize();
    iSocketInitialize();
    iAppSocketInitialize();
    iSockMgrInitialize();
    iHttpRouteInitialize();
    iWebAdminInitialize();

    taskPushEvent(s_appTasks.data(), s_appTasks.size());

    {
        lock_guard lk{s_runMut};
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
void Dim::iAppQueueStartupTask(ITaskNotify * task) {
    assert(appStarting());
    s_appTasks.push_back(task);
}

//===========================================================================
void Dim::iAppSetFlags(EnumFlags<AppFlags> flags) {
    assert(appStarting());
    s_appFlags = flags;
}


/****************************************************************************
*
*   Public API
*
***/

//===========================================================================
int Dim::appRun(
    function<void(int argc, char *argv[])> fn,
    int argc,
    char * argv[],
    const VersionInfo & ver,
    string_view baseName,
    EnumFlags<AppFlags> flags
) {
    s_fnProxyApp.fn = move(fn);
    return appRun(&s_fnProxyApp, argc, argv, ver, baseName, flags);
}

//===========================================================================
int Dim::appRun(
    IAppNotify * app,
    int argc,
    char * argv[],
    const VersionInfo & ver,
    string_view baseName,
    EnumFlags<AppFlags> flags
) {
    // Save options.
    {
        lock_guard lk(s_runMut);
        assert(s_runMode == kRunStopped);
        s_runMode = kRunStarting;
        s_exitcode = 0;
    }
    s_usageErrorSignaled.clear();
    s_appFlags = flags;
    s_appTasks.clear();

    if (!s_appFlags.any(fAppWithService)) {
        s_appIndex = 1;
        s_groupIndex = 1;
    } else {
        Cli cli;
        cli.opt(&s_appIndex, "app-index", 1)
            .desc("Identifies service when multiple are configured.");
        cli.opt(&s_groupIndex, "group-index", 1)
            .desc("Identifies service group when there are multiple.");

        // The command line will be validated later by the application, right
        // now we just need the appIndex so the configuration can be processed.
        stringstream tin, tout;
        auto conin = &cli.conin();
        auto conout = &cli.conout();
        cli.iostreams(&tin, &tout);
        (void) cli.parse(argc, argv);
        cli.iostreams(conin, conout);
    }

    s_appVer = ver;
    s_appBaseName = baseName;
    s_appName.clear();
    s_webDir.clear();

    // Finish initialization and start.
    iPlatformInitialize(PlatformInit::kBeforeAll);
    iPerfInitialize();
    iLogInitialize();
    if (s_appFlags.any(fAppWithConsole))
        logDefaultMonitor(consoleBasicLogger());
    iTaskInitialize();
    iTimerInitialize();

    s_app = app;
    s_app->m_argc = argc;
    s_app->m_argv = argv;
    taskPushEvent(initApp);

    // Wait for application to finish.
    unique_lock lk{s_runMut};
    while (s_runMode != kRunStopping)
        s_runCv.wait(lk);
    lk.unlock();

    iShutdownDestroy();

    //-----------------------------------------------------------------------
    // No external effects should happen after this point. Any sockets, pipes,
    // files, or other shared resources MUST already be closed. When running as
    // a service the SERVICE_STOPPED status has been reported and the Service
    // Control Manager may have already stopped handling requests from us in
    // favor of a new instance of the service.
    //-----------------------------------------------------------------------

    iTimerDestroy();
    iTaskDestroy();
    iLogDestroy();
    iPerfDestroy();
    iPlatformDestroy();
    lk.lock();
    assert(s_runMode == kRunStopping);
    s_runMode = kRunStopped;
    return s_exitcode;
}

//===========================================================================
unsigned Dim::appIndex() {
    return s_appIndex;
}

//===========================================================================
const VersionInfo & Dim::appVersion() {
    return s_appVer;
}

//===========================================================================
EnumFlags<AppFlags> Dim::appFlags() {
    return s_appFlags;
}

//===========================================================================
const string & Dim::appBaseName() {
    assert(!s_appName.empty());
    return s_appBaseName;
}

//===========================================================================
const string & Dim::appName() {
    assert(!s_appName.empty());
    return s_appName;
}

//===========================================================================
unsigned Dim::appGroupIndex() {
    return s_groupIndex;
}

//===========================================================================
string Dim::appGroupType() {
    return s_groupType;
}

//===========================================================================
const Path & Dim::appInitialDir() {
    assert(!s_appName.empty());
    return s_initialDir;
}

//===========================================================================
const Path & Dim::appRootDir() {
    assert(!s_appName.empty());
    return s_rootDir;
}

//===========================================================================
const Path & Dim::appBinDir() {
    assert(!s_appName.empty());
    return s_binDir;
}

//===========================================================================
const Path & Dim::appConfigDir() {
    assert(!s_appName.empty());
    return s_confDir;
}

//===========================================================================
const Path & Dim::appLogDir() {
    assert(!s_appName.empty());
    return s_logDir;
}

//===========================================================================
const Path & Dim::appDataDir() {
    assert(!s_appName.empty());
    return s_dataDir;
}

//===========================================================================
const Path & Dim::appCrashDir() {
    assert(!s_appName.empty());
    return s_crashDir;
}

//===========================================================================
SockAddr Dim::appAddress() {
    assert(!s_appName.empty());
    return s_appAddr;
}

//===========================================================================
RunMode Dim::appMode() {
    lock_guard lk(s_runMut);
    return s_runMode;
}

//===========================================================================
int Dim::appExitCode() {
    lock_guard lk(s_runMut);
    return s_exitcode;
}

//===========================================================================
bool Dim::appUsageErrorSignaled() {
    return s_usageErrorSignaled.test();
}

//===========================================================================
bool Dim::appConfigPath(Path * out, string_view file, bool cine) {
    return !fileChildPath(out, appConfigDir(), file, cine);
}

//===========================================================================
bool Dim::appLogPath(Path * out, string_view file, bool cine) {
    return !fileChildPath(out, appLogDir(), file, cine);
}

//===========================================================================
bool Dim::appDataPath(Path * out, string_view file, bool cine) {
    return !fileChildPath(out, appDataDir(), file, cine);
}

//===========================================================================
bool Dim::appCrashPath(Path * out, string_view file, bool cine) {
    return !fileChildPath(out, appCrashDir(), file, cine);
}

//===========================================================================
void Dim::appSignalShutdown(int code) {
    if (code == EX_PENDING)
        return;

    unique_lock lk{s_runMut};
    if (code > s_exitcode)
        s_exitcode = code;
    if (s_runMode != kRunStopping) {
        assert(s_runMode != kRunStopped);
        s_runMode = kRunStopping;
        lk.unlock();
        s_runCv.notify_one();
    }
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

    s_usageErrorSignaled.test_and_set();
    if (code) {
        bool hasConsole = appFlags().any(fAppWithConsole | fAppIsService);
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
