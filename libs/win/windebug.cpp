// Copyright Glen Knowles 2020 - 2022.
// Distributed under the Boost Software License, Version 1.0.
//
// windebug.cpp - dim windows platform
#include "pch.h"
#pragma hdrstop

using namespace std;
using namespace Dim;


/****************************************************************************
*
*   Internal API
*
***/

static wchar_t s_fname[MAX_PATH] = L"memleak.log";

//===========================================================================
extern "C" static int attachMemLeakHandle() {
    auto leakFlags = 0;
#ifndef __SANITIZE_ADDRESS__
    leakFlags = _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG);
#endif
    if (~leakFlags & _CRTDBG_LEAK_CHECK_DF) {
        // Leak check disabled.
        return 0;
    }

    _CrtMemState state;
    _CrtMemCheckpoint(&state);
    auto leaks = state.lCounts[_CLIENT_BLOCK] + state.lCounts[_NORMAL_BLOCK];
    if (leaks) {
        if (GetConsoleWindow() != NULL) {
            // Has console attached.
            wchar_t buf[256];
            auto len = swprintf(
                buf,
                size(buf),
                L"\nMemory leaks: %zu, see %s for details.\n",
                leaks,
                s_fname
            );
            ConsoleScopedAttr attr(kConsoleError);
            HANDLE hOutput = GetStdHandle(STD_OUTPUT_HANDLE);
            WriteConsoleW(hOutput, buf, len, NULL, NULL);
        }
        auto f = CreateFileW(
            s_fname,
            GENERIC_READ | GENERIC_WRITE,
            0,      // sharing
            NULL,   // security attributes
            CREATE_ALWAYS,
            0,      // flags and attrs
            NULL    // template file
        );
        if (f == INVALID_HANDLE_VALUE) {
            WinError err;
        } else {
            _CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_FILE);
            _CrtSetReportFile(_CRT_WARN, f);
            _CrtDumpMemoryLeaks();
            CloseHandle(f);
        }

        _CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_DEBUG);
        if (IsDebuggerPresent())
            __debugbreak();
    }
    return 0;
}

// pragma sections
// .CRT$Xpq
//  p (category) values:
//      I   C init
//      C   C++ init, aka namespace scope constructors
//      D   DLL TLS init (per thread)
//      L   Loader TLS Callback (DLL_PROCESS_ATTACH, DLL_THREAD_ATTACH,
//              DLL_THREAD_DETACH, and DLL_PROCESS_DETACH)
//      P   pre-terminators
//      T   terminators
//  q (segment) values:
//      A   reserved for start of list sentinel (A[A-Z] can be used)
//      C   compiler
//      L   library
//      U   user
//      X   terminator ?
//      Z   reserved for end of segments sentinel
//
// Segments within a terminators category are executed in alphabetical order,
// XTU is the user termination segment which runs after the compiler and
// library segments have been terminated.

#pragma section(".CRT$XTU", long, read)
#pragma data_seg(push)
#pragma data_seg(".CRT$XTU")
static auto s_attachMemLeakHandle = attachMemLeakHandle;
#pragma data_seg(pop)

//===========================================================================
void Dim::winDebugInitialize() {
    auto fname = Path("memleak-" + appName()).defaultExt("log");
    Path out;
    if (appLogPath(&out, fname))
        strCopy(s_fname, size(s_fname), out.c_str());

    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
}
