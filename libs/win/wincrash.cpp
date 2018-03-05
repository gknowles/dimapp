// Copyright Glen Knowles 2018.
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
    //{ SIGILL },
    //{ SIGSEGV },
};
static PTOP_LEVEL_EXCEPTION_FILTER s_oldFilter;
static int s_oldErrorMode;
static unsigned s_oldAbortBehavior;
static void * s_oldInval;
static void * s_oldPure;
static new_handler s_oldNew;
static int s_oldNewMode;

static mutex s_mut;
static Path s_crashFile;


/****************************************************************************
*
*   Handlers
*
***/

//===========================================================================
extern "C" void abortHandler(int sig) {
    WinError err;
    printf("abnormal abort, sig %d\n", sig);
    scoped_lock<mutex> lk{s_mut};
    auto f = CreateFile(
        s_crashFile.c_str(),
        GENERIC_READ | GENERIC_WRITE,
        0,
        NULL, // security attributes
        CREATE_ALWAYS,
        0,
        NULL // template file
    );
    if (f == INVALID_HANDLE_VALUE) {
        err.set();
        _CrtSetDbgFlag(0);
        _Exit(3);
    }
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
    auto type = MINIDUMP_TYPE(MiniDumpWithDataSegs
        | MiniDumpWithIndirectlyReferencedMemory
        | MiniDumpWithProcessThreadData
        | MiniDumpIgnoreInaccessibleMemory
        | MiniDumpWithFullMemoryInfo
        | MiniDumpWithThreadInfo
    );
    if (!MiniDumpWriteDump(
        GetCurrentProcess(),
        GetCurrentProcessId(),
        f,
        type,
        &mei,
        NULL,
        NULL
    )) {
        err.set();
        cout << "Writing crash dump failed: " << err;
        cout.flush();
    }
    CloseHandle(f);
    _CrtSetDbgFlag(0);
    _Exit(3);
}

//===========================================================================
void invalidParameterHandler(
    const wchar_t * expression,
    const wchar_t * function,
    const wchar_t * file,
    unsigned line,
    uintptr_t reserved
) {
    logMsgCrash() << "Invalid parameter: "
        << utf8(function) << '(' << utf8(expression) << ")"
        << ", " << utf8(file) << ':' << line;
}

//===========================================================================
void pureCallHandler() {
    logMsgCrash() << "Pure virtual function called";
}

//===========================================================================
void newHandler() {
    logMsgCrash() << "Out of heap memory";
}

//===========================================================================
DWORD WINAPI appRecoveryCallback(void * param) {
    logMsgCrash() << "Application recovery callback";
    return 0;
}

//===========================================================================
LONG WINAPI unhandledExceptionFilter(EXCEPTION_POINTERS * ei) {
    _pxcptinfoptrs = ei;
    logMsgCrash() << "Unhandled exception";
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
    for (auto && [sig, handler] : s_oldHandlers) {
        signal(sig, (decltype(SIG_DFL)) handler);
        handler = nullptr;
    }
    _set_error_mode(s_oldErrorMode);
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
void Dim::winCrashInitialize() {
    // Make sure DbgHelp.dll gets loaded early, otherwise it could deadlock
    // in the loader lock if we're processing a crash in DllMain.
    winLoadProc(
        s_writeDump,
        "Dbghelp",
        "MiniDumpWriteDump"
    );

    shutdownMonitor(&s_cleanup);

    s_crashFile = "crash/crash.dmp";

    RegisterApplicationRecoveryCallback(appRecoveryCallback, NULL, 30'000, 0);
    SetUnhandledExceptionFilter(unhandledExceptionFilter);

    for (auto && [sig, handler] : s_oldHandlers)
        handler = signal(sig, abortHandler);

    //SetErrorMode(
    s_oldErrorMode = _set_error_mode(_OUT_TO_STDERR);
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
void Dim::winCrashThreadInitialize() {
    //for (auto && [sig, handler] : s_oldHandlers)
    //    handler = signal(sig, abortHandler);
}
