// Copyright Glen Knowles 2019 - 2021.
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
static bool insert(StrTrie * vals, string_view val) {
    if (s_verbose)
        cout << "---\ninsert: " << urlEncodeQueryComponent(val)
            << ", len=" << val.size() << '\n';
    auto rc = vals->insert(val);
    if (s_verbose)
        vals->dump(cout);
    return rc;
}

//===========================================================================
inline static void internalTests() {
    if (s_verbose)
        cout << "\n> INTERNAL TESTS" << endl;
    int line = 0;
    StrTrie vals;
    string out;
    EXPECT(vals.empty());
    auto b = insert(&vals, "abc");
    EXPECT(b);
    EXPECT(vals.contains("abc"));
    EXPECT(!vals.contains("b"));
    EXPECT(!vals.contains("ab"));
    EXPECT(!vals.contains("abcd"));

    b = insert(&vals, "a");
    EXPECT(b);
    EXPECT(vals.contains("a"));
    EXPECT(vals.contains("abc"));
    EXPECT(!vals.contains("ab"));
    EXPECT(!vals.contains("abcd"));

    b = insert(&vals, "ac");
    EXPECT(vals.contains("ac"));

    b = insert(&vals, "ade");
    EXPECT(vals.contains("ade"));

    b = insert(&vals, "abcdefghijklmnopqrstuvwxyz");
    EXPECT(vals.contains("abcdefghijklmnopqrstuvwxyz"));
    EXPECT(vals.contains("abc"));
}

//===========================================================================
inline static void randomFill() {
    if (s_verbose)
        cout << "\n> RANDOM FILL" << endl;
    [[maybe_unused]] int line = 0;
    StrTrie vals;
    default_random_engine s_reng;
    string key;
    for (auto i = 0; i < 120; ++i) {
        key.resize(0);
        auto len = 8u; // s_reng() % 25;
        for (auto j = 0u; j < len; ++j)
            key += 'a' + char(s_reng() % 26);
        if (s_verbose)
            cout << i + 1 << " ";
        insert(&vals, key);
    }
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

    // internalTests();
    randomFill();

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
