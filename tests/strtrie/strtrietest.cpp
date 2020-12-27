// Copyright Glen Knowles 2019 - 2020.
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
    auto b = vals.insert("abc");
    EXPECT(b);
    EXPECT(vals.lowerBound(&out, "abc") && out == "abc");
    EXPECT(!vals.lowerBound(&out, "b"));
    EXPECT(!vals.lowerBound(&out, "ab"));
    EXPECT(!vals.lowerBound(&out, "abcd"));
    vals.dump(cout);
    b = vals.insert("a");
    vals.dump(cout);
    EXPECT(b);
    EXPECT(vals.lowerBound(&out, "a") && out == "a");
    EXPECT(vals.lowerBound(&out, "abc") && out == "abc");

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
    return appRun(app, argc, argv);
}
