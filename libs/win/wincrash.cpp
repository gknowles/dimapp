// Copyright Glen Knowles 2018 - 2022.
// Distributed under the Boost Software License, Version 1.0.
//
// wincrash.cpp - dim windows platform
#include "pch.h"
#pragma hdrstop

using namespace std;
using namespace Dim;


/****************************************************************************
*
*   Declarations
*
***/

const unsigned kAbortBehaviorMask = _WRITE_ABORT_MSG | _CALL_REPORTFAULT;

using SetProcessUserModeExceptionPolicyFn = BOOL(WINAPI *)(DWORD dwFlags);
using GetProcessUserModeExceptionPolicyFn = BOOL(WINAPI *)(LPDWORD lpFlags);
using MiniDumpWriteDumpFn = BOOL(WINAPI *)(
    _In_ HANDLE hProcess,
    _In_ DWORD ProcessId,
    _In_ HANDLE hFile,
    _In_ MINIDUMP_TYPE DumpType,
    _In_opt_ PMINIDUMP_EXCEPTION_INFORMATION ExceptionParam,
    _In_opt_ PMINIDUMP_USER_STREAM_INFORMATION UserStreamParam,
    _In_opt_ PMINIDUMP_CALLBACK_INFORMATION CallbackParam
);


/****************************************************************************
*
*   Variables
*
***/

static MiniDumpWriteDumpFn s_writeDump;

static struct { int sig; void * handler; } s_oldHandlers[] = {
    { SIGABRT },
    //{ SIGSEGV },
};
static PTOP_LEVEL_EXCEPTION_FILTER s_oldFilter;
static int s_oldErrorModeWin32;
static int s_oldErrorModeCrt;
static unsigned s_oldAbortBehavior;
static void * s_oldInval;
static void * s_oldPure;
static new_handler s_oldNew;
static int s_oldNewMode;

static mutex s_mut;
static Path s_crashFile;
static wstring s_crashFileW;


/****************************************************************************
*
*   Helpers
*
***/

//===========================================================================
static void writeDump() {
    WinError err;

    auto f = CreateFileW(
        s_crashFileW.c_str(),
        GENERIC_READ | GENERIC_WRITE,
        0,
        NULL, // security attributes
        CREATE_ALWAYS,
        0,
        NULL // template file
    );
    if (f == INVALID_HANDLE_VALUE) {
        err.set();
    } else {
        MINIDUMP_EXCEPTION_INFORMATION mei = {};
        EXCEPTION_RECORD record = {};
        CONTEXT context = {};
        EXCEPTION_POINTERS newptrs = { &record, &context };

        mei.ThreadId = GetCurrentThreadId();
        mei.ClientPointers = true;
        if (auto ptrs = (EXCEPTION_POINTERS *) _pxcptinfoptrs) {
            mei.ExceptionPointers = ptrs;
        } else {
            mei.ExceptionPointers = &newptrs;
            RtlCaptureContext(&context);
            record.ExceptionCode = EXCEPTION_BREAKPOINT;
        }
        auto typeFlags = MINIDUMP_TYPE(MiniDumpNormal
            | MiniDumpWithDataSegs
            | MiniDumpWithIndirectlyReferencedMemory
            | MiniDumpWithProcessThreadData
            | MiniDumpIgnoreInaccessibleMemory
            | MiniDumpWithThreadInfo
        );
        if (!MiniDumpWriteDump(
            GetCurrentProcess(),
            GetCurrentProcessId(),
            f,
            typeFlags,
            &mei,
            NULL,
            NULL
        )) {
            err.set();
            cout << "Writing crash dump failed: " << err;
            cout.flush();
        }
        CloseHandle(f);
    }
}


/****************************************************************************
*
*   Handlers
*
***/

//===========================================================================
extern "C" void abortHandler(int sig) {
    printf("\nAbnormal abort, sig %d\n", sig);
    scoped_lock lk{s_mut};

    if (IsDebuggerPresent())
        DebugBreak();

    if (appFlags().any(fAppWithDumps) && s_crashFile)
        writeDump();

    _CrtSetDbgFlag(0);
    TerminateProcess(GetCurrentProcess(), 3);
    Sleep(INFINITE);
}

//===========================================================================
void invalidParameterHandler(
    const wchar_t * expression,
    const wchar_t * function,
    const wchar_t * file,
    unsigned line,
    uintptr_t reserved
) {
    logMsgFatal() << "Invalid parameter: "
        << utf8(function) << '(' << utf8(expression) << ")"
        << ", " << utf8(file) << ':' << line;
}

//===========================================================================
void pureCallHandler() {
    logMsgFatal() << "Pure virtual function called";
}

//===========================================================================
void newHandler() {
    logMsgFatal() << "Out of heap memory";
}

//===========================================================================
DWORD WINAPI appRecoveryCallback(void * param) {
    logMsgFatal() << "Application recovery callback";
    return 0;
}

//===========================================================================
LONG WINAPI unhandledExceptionFilter(EXCEPTION_POINTERS * ei) {
    _pxcptinfoptrs = ei;
    logMsgFatal() << "Unhandled exception";
    return EXCEPTION_CONTINUE_SEARCH;
}


/****************************************************************************
*
*   Shutdown monitor
*
***/

namespace {

class ShutdownNotify : public IShutdownNotify {
    void onShutdownConsole(bool firstTry) override;
};

} // namespace

static ShutdownNotify s_cleanup;

//===========================================================================
void ShutdownNotify::onShutdownConsole(bool firstTry) {
    auto sef = SetUnhandledExceptionFilter(s_oldFilter);
    if (sef != unhandledExceptionFilter)
        SetUnhandledExceptionFilter(sef);

    for (auto && [sig, handler] : s_oldHandlers) {
        signal(sig, (decltype(SIG_DFL)) handler);
        handler = nullptr;
    }
    //SetErrorMode(s_oldErrorModeWin32);
    _set_error_mode(s_oldErrorModeCrt);
    _set_abort_behavior(s_oldAbortBehavior, kAbortBehaviorMask);
    _set_invalid_parameter_handler((_invalid_parameter_handler) s_oldInval);
    _set_purecall_handler((_purecall_handler) s_oldPure);
    set_new_handler(s_oldNew);
    _set_new_mode(s_oldNewMode);
}


/****************************************************************************
*
*   Internal API
*
***/

//===========================================================================
static void initBeforeVars() {
    // Make sure DbgHelp.dll gets loaded early, otherwise it could deadlock
    // in the loader lock if we're processing a crash in DllMain.
    winLoadProc(
        &s_writeDump,
        "Dbghelp",
        "MiniDumpWriteDump"
    );

    s_crashFile.clear();
    shutdownMonitor(&s_cleanup);

    RegisterApplicationRecoveryCallback(appRecoveryCallback, NULL, 30'000, 0);
    s_oldFilter = SetUnhandledExceptionFilter(unhandledExceptionFilter);

    for (auto && [sig, handler] : s_oldHandlers)
        handler = signal(sig, abortHandler);

    //s_oldErrorModeWin32 = SetErrorMode(SEM_FAILCRITICALERRORS);
    s_oldErrorModeCrt = _set_error_mode(_OUT_TO_STDERR);
    s_oldAbortBehavior = _set_abort_behavior(0, kAbortBehaviorMask);
    s_oldInval = _set_invalid_parameter_handler(invalidParameterHandler);
    s_oldPure = _set_purecall_handler(pureCallHandler);

    s_oldNew = set_new_handler(newHandler);
    // make malloc failures also call the new_handler
    s_oldNewMode = _set_new_mode(1);

    //printf(nullptr);
    //*(char *) nullptr = 0;
    //assert(0);
}

//===========================================================================
static void initAfterVars() {
    if (appFlags().any(fAppWithFiles)) {
        auto crashDir = appCrashDir();
        vector<Glob::Entry> found;
        for (auto && e : fileGlob(crashDir, "*.dmp"))
            found.push_back(e);
        static const int kMaxKeepFiles = 10;
        if (found.size() > kMaxKeepFiles) {
            auto nth = found.end() - kMaxKeepFiles;
            nth_element(
                found.begin(),
                nth,
                found.end(),
                [](auto & a, auto & b) { return a.mtime < b.mtime; }
            );
            for (auto p = found.begin(); p != nth; ++p)
                fileRemove(p->path);
        }
        s_crashFile = crashDir;
        s_crashFile /= "crash";
        s_crashFile += to_string(timeToUnix(timeNow()));
        s_crashFile += ".dmp";
        s_crashFileW = toWstring(s_crashFile);
    }
}

//===========================================================================
void Dim::winCrashInitialize(PlatformInit phase) {
    if (phase == PlatformInit::kBeforeAppVars) {
        initBeforeVars();
    } else {
        assert(phase == PlatformInit::kAfterAppVars);
        initAfterVars();
    }
}

//===========================================================================
void Dim::winCrashInitializeThread() {
    //for (auto && [sig, handler] : s_oldHandlers)
    //    handler = signal(sig, abortHandler);
}
