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
    if (!bool(__VA_ARGS__)) \
        failed(line ? line : __LINE__, #__VA_ARGS__)


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
static void failed(int line, const char msg[]) {
    logMsgError() << "Line " << line << ": EXPECT(" << msg << ") failed";
}

//===========================================================================
static bool insert(StrTrie * vals, string_view val) {
    if (s_verbose) {
        cout << "---\ninsert, len=" << val.size() << ":\n";
        hexDump(cout, val);
        vals->debug(true);
    }
    auto rc = vals->insert(val);
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
    EXPECT(!vals.contains("abb"));
    EXPECT(!vals.contains("abd"));
    EXPECT(!vals.contains("abcd"));

    b = insert(&vals, "a");
    EXPECT(b);
    EXPECT(vals.contains("a"));
    EXPECT(vals.contains("abc"));
    EXPECT(!vals.contains("ab"));
    EXPECT(!vals.contains("abcd"));

    b = insert(&vals, "ac");
    EXPECT(vals.contains("a"));
    EXPECT(vals.contains("abc"));
    EXPECT(vals.contains("ac"));

    b = insert(&vals, "ade");
    EXPECT(vals.contains("ade"));

    b = insert(&vals, "abcdefghijklmnopqrstuvwxyz");
    EXPECT(vals.contains("abcdefghijklmnopqrstuvwxyz"));
    EXPECT(vals.contains("abc"));

    vals.clear();
    insert(&vals, "abc");
    insert(&vals, "aw");
    insert(&vals, "abd");
}

//===========================================================================
inline static void randomFill() {
    if (s_verbose)
        cout << "\n> RANDOM FILL" << endl;
    [[maybe_unused]] int line = 0;
    StrTrie vals;
    default_random_engine s_reng;
    string key;
    auto count = 1000;
    for (auto i = 0; i < count; ++i) {
        key.resize(0);
        auto len = s_reng() % 25;
        for (auto j = 0u; j < len; ++j) {
            key += 'a' + char(s_reng() % 26);
            //key += "abyz"[s_reng() % 4];
        }
        //if (key < "tqb" || key >= "tqd")
        //    continue;
        if (i == count - 1)
            s_verbose = true;
        if (s_verbose)
            cout << i + 1 << " ";
        insert(&vals, key);
        EXPECT(vals.contains(key));
    }
    cout << "--- Stats\n";
    vals.dumpStats(cout);
    cout << endl;
}


/****************************************************************************
*
*   Application
*
***/

//===========================================================================
static void app(int argc, char *argv[]) {
    cout.imbue(locale(""));
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
    randomFill();

    if (int errs = logGetMsgCount(kLogTypeError)) {
        ConsoleScopedAttr attr(kConsoleError);
        cerr << "*** TEST FAILURES: " << errs << endl;
        appSignalShutdown(EX_SOFTWARE);
    } else {
        cout << "All tests passed" << endl;
        appSignalShutdown(EX_OK);
    }
    logStopwatch();
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
