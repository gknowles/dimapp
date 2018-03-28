// Copyright Glen Knowles 2017.
// Distributed under the Boost Software License, Version 1.0.
//
// path.cpp - dimapp test path
#include "pch.h"
#pragma hdrstop

using namespace std;
using namespace Dim;


/****************************************************************************
*
*   Declarations
*
***/

#define EXPECT(...)                                                         \
    if (!bool(__VA_ARGS__)) {                                               \
        logMsgError() << "Line " << (line ? line : __LINE__) << ": EXPECT(" \
                      << #__VA_ARGS__ << ") failed";                        \
    }


/****************************************************************************
*
*   Application
*
***/

//===========================================================================
static void app(int argc, char * argv[]) {
    int line = 0;
    Path p;
    p = "..\\a\\b\\..\\base.ext";
    EXPECT(p == "../a/base.ext"sv);
    p = "hello";
    p.defaultExt("txt");
    EXPECT(p == "hello.txt"sv);

    struct {
        string_view base;
        string_view rel;
        string_view out;
        int line;
    } resolveTests[] = {
        { "",        "rel",    "rel",           __LINE__ },
        { "base",    "rel",    "base/rel",      __LINE__ },
        { "base",    "/rel",   "/rel",          __LINE__ },
        { "base",    "c:rel",  "c:rel",         __LINE__ },
        { "base",    "c:/rel", "c:/rel",        __LINE__ },
        { "/base",   "rel",    "/base/rel",     __LINE__ },
        { "/base",   "/rel",   "/rel",          __LINE__ },
        { "/base",   "c:rel",  "c:rel",         __LINE__ },
        { "/base",   "c:/rel", "c:/rel",        __LINE__ },
        { "c:base",  "rel",    "c:base/rel",    __LINE__ },
        { "c:base",  "/rel",   "c:/rel",        __LINE__ },
        { "c:base",  "c:rel",  "c:base/rel",    __LINE__ },
        { "c:base",  "c:/rel", "c:/rel",        __LINE__ },
        { "c:base",  "a:rel",  "a:rel",         __LINE__ },
        { "c:base",  "a:/rel", "a:/rel",        __LINE__ },
        { "c:/base", "rel",    "c:/base/rel",   __LINE__ },
        { "c:/base", "/rel",   "c:/rel",        __LINE__ },
        { "c:/base", "c:rel",  "c:/base/rel",   __LINE__ },
        { "c:/base", "c:/rel", "c:/rel",        __LINE__ },
        { "c:/base", "a:rel",  "a:rel",         __LINE__ },
        { "c:/base", "a:/rel", "a:/rel",        __LINE__ },
    };
    for (auto && t : resolveTests) {
        p.assign(t.rel);
        p.resolve(t.base);
        line = t.line;
        EXPECT(p == t.out);
    }
    line = 0;

    if (int errs = logGetMsgCount(kLogTypeError)) {
        ConsoleScopedAttr attr(kConsoleError);
        cerr << "*** TEST FAILURES: " << errs << endl;
        appSignalShutdown(EX_SOFTWARE);
    } else {
        cout << "All tests passed" << endl;
        appSignalShutdown(EX_OK);
    }
}


/****************************************************************************
*
*   main
*
***/

//===========================================================================
int main(int argc, char * argv[]) {
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF
        | _CRTDBG_LEAK_CHECK_DF
        | _CRTDBG_DELAY_FREE_MEM_DF);
    _set_error_mode(_OUT_TO_MSGBOX);

    int code = appRun(app, argc, argv);
    return code;
}
