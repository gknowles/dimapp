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
*   Variables
*
***/

static bool s_verbose;


/****************************************************************************
*
*   Helpers
*
***/

//===========================================================================
static void dump(const StrTrie & vals) {
    if (s_verbose)
        vals.dump(cout);
}

//===========================================================================
static void internalTests() {
    int line = 0;
    StrTrie vals;
    string out;
    EXPECT(vals.empty());
    auto b = vals.insert("abc");
    EXPECT(b);
    dump(vals);
    EXPECT(vals.contains("abc"));
    EXPECT(!vals.contains("b"));
    EXPECT(!vals.contains("ab"));
    EXPECT(!vals.contains("abcd"));

    b = vals.insert("a");
    dump(vals);
    EXPECT(b);
    EXPECT(vals.contains("a"));
    EXPECT(vals.contains("abc"));
    EXPECT(!vals.contains("ab"));
    EXPECT(!vals.contains("abcd"));

    b = vals.insert("ac");
    dump(vals);
    EXPECT(vals.contains("ac"));

    b = vals.insert("ade");
    dump(vals);
    EXPECT(vals.contains("ade"));

    b = vals.insert("abcdefghijklmnopqrstuvwxyz");
    dump(vals);
    EXPECT(vals.contains("abcdefghijklmnopqrstuvwxyz"));
    EXPECT(vals.contains("abc"));
}


/****************************************************************************
*
*   Application
*
***/

//===========================================================================
static void app(int argc, char *argv[]) {
    Cli cli;
    cli.helpNoArgs();
    cli.versionOpt("1.0 (" __DATE__ ")");
    cli.opt<bool>(&s_verbose, "v verbose.")
        .desc("Dump container state between tests.");
    auto & test = cli.opt<bool>("test", true).desc("Run internal unit tests");
    if (!cli.parse(argc, argv))
        return appSignalUsageError();
    if (!*test) {
        cout << "No tests run" << endl;
        return appSignalShutdown(EX_OK);
    }

    internalTests();

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
