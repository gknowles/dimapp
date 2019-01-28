// Copyright Glen Knowles 2019.
// Distributed under the Boost Software License, Version 1.0.
//
// strtrietest.cpp - dim test strtrie
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
static void app(int argc, char *argv[]) {
    int line = 0;

    StrTrie vals;
    string out;
    EXPECT(vals.empty());
    auto b = vals.insert("abc", "123");
    EXPECT(b);
    EXPECT(vals.find(&out, "abc") && out == "123");
    EXPECT(!vals.find(&out, "b"));
    EXPECT(!vals.find(&out, "ab"));
    EXPECT(!vals.find(&out, "abcd"));
    vals.dump(cout);
    b = vals.insert("a", "1");
    vals.dump(cout);
    EXPECT(b);
    EXPECT(vals.find(&out, "a") && out == "1");
    EXPECT(vals.find(&out, "abc") && out == "123");

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
*   External
*
***/

//===========================================================================
int main(int argc, char * argv[]) {
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF
        | _CRTDBG_LEAK_CHECK_DF
        | _CRTDBG_DELAY_FREE_MEM_DF);
    _set_error_mode(_OUT_TO_MSGBOX);
    return appRun(app, argc, argv);
}
