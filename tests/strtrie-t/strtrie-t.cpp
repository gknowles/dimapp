// Copyright Glen Knowles 2019 - 2022.
// Distributed under the Boost Software License, Version 1.0.
//
// strtrie-t.cpp - dim test strtrie
#include "pch.h"
#pragma hdrstop

using namespace std;
using namespace Dim;


/****************************************************************************
*
*   Tuning parameters
*
***/

const VersionInfo kVersion = { 1 };


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
static void printAction(
    StrTrie * vals, 
    string_view val, 
    string_view action
) {
    if (s_verbose) {
        cout << "---\n" << action << ", len=" << val.size() << ":\n";
        hexDump(cout, val);
    }
    vals->debug(s_verbose);
}

//===========================================================================
static bool insert(StrTrie * vals, string_view val) {
    printAction(vals, val, __func__);
    auto rc = vals->insert(val);
    return rc;
}

//===========================================================================
static bool erase(StrTrie * vals, string_view val) {
    printAction(vals, val, __func__);
    auto rc = vals->erase(val);
    return rc;
}

//===========================================================================
static void insertTest(
    const vector<string_view> & strs, 
    const vector<string_view> & ghosts = {}
) {
    int line = 0;
    StrTrie vals;
    for (auto&& str : strs) {
        EXPECT(insert(&vals, str));
        EXPECT(vals.contains(str));
    }
    for (auto&& str : strs) {
        EXPECT(vals.contains(str));
        EXPECT(!insert(&vals, str));
    }
    for (auto&& str : ghosts) 
        EXPECT(!vals.contains(str));
    for (auto&& str : strs) {
        EXPECT(erase(&vals, str));
        EXPECT(!vals.contains(str));
    }
    EXPECT(vals.empty());
}

//===========================================================================
inline static void internalTests() {
    if (s_verbose)
        cout << "\n> INTERNAL TESTS" << endl;
    int line = 0;
    StrTrie vals;
    string out;
    EXPECT(vals.empty());

    insertTest({"abc"}, {"", "b", "ab", "abb", "abd", "abcd"});

    // SEG NODE
    // Fork last seg with end of new key
    insertTest({"abc", ""}, {"0", "a", "abcd"});            // start of seg
    insertTest({"abc", "a"}, {"", "0", "b", "ab", "ac"});   // high mid 
    insertTest({"abc", "ab"}, {"a", "ac"});                 // high end

    // Fork last seg with key
    insertTest({"abc", "x123"});    // high start of seg
    insertTest({"abc", "b123"});    // low start
    insertTest({"abc", "ax123"});   // high mid
    insertTest({"abc", "ac123"});   // low mid
    insertTest({"abc", "abw123"});  // high end
    insertTest({"abc", "abd123"});  // low end
    insertTest({"abc", "abc123"});  // after end

    // Fork last seg with tailless key
    insertTest({"abc", "x"});    // high start of seg
    insertTest({"abc", "b"});    // low start
    insertTest({"abc", "ax"});   // high mid
    insertTest({"abc", "ac"});   // low mid
    insertTest({"abc", "abw"});  // high end
    insertTest({"abc", "abd"});  // low end

    // Fork middle seg with key
    insertTest({"abcdefghijklmnopqrstuvwxyz", "abw123"});   // high mid of seg
    insertTest({"abcdefghijklmnopqrstuvwxyz", "abd123"});   // low mid of seg


    //-----------------------------------------------------------------------
    // Search and iterate
    vector<string_view> keys = { "abc", "aw", "abd" };
    vals.clear();
    for (auto&& key : keys)
        insert(&vals, key);
    ranges::sort(keys);
    auto ri = keys.begin();
    for (auto&& key : vals) {
        EXPECT(key == *ri);
        ri += 1;
    }
    EXPECT(ri == keys.end());

    auto vi = vals.lowerBound("abcd");
    EXPECT(*vi == "abd");
    vi = vals.find("abd");
    EXPECT(*vi == "abd");
    vi = vals.lowerBound("abd");
    EXPECT(*vi == "abd");
    vi = vals.upperBound("abd");
    EXPECT(*vi == "aw");
    vi = vals.findLess("abd");
    EXPECT(*vi == "abc");
    vi = vals.findLessEqual("abd");
    EXPECT(*vi == "abd");
    
    EXPECT(vals.back() == "aw");

    for (auto&& key : keys) {
        erase(&vals, key);
        EXPECT(!vals.contains(key));
    }
    EXPECT(vals.empty());
}

//===========================================================================
inline static void randomFill(size_t count, size_t maxLen, size_t charVals) {
    [[maybe_unused]] int line = 0;
    if (!count)
        return;
    if (s_verbose) {
        cout << "\n> RANDOM FILL: "
            << "count = " << count 
            << ", maxLen = " << maxLen
            << ", charVals = " << charVals
            << endl;
    }
    StrTrie vals;
    default_random_engine s_reng;
    string key;
    size_t inserted = 0;
    vector<string> keys;
    keys.reserve(count);
    for (auto i = 0; i < count; ++i) {
        key.resize(0);
        auto len = s_reng() % maxLen;
        for (auto j = 0u; j < len; ++j) {
            key += 'a' + char(s_reng() % charVals);
            //key += "abyz"[s_reng() % 4];
        }
        if (i == count - 1)
            s_verbose = true;
        if (s_verbose)
            cout << i + 1 << " ";
        if (insert(&vals, key)) {
            inserted += 1;
            keys.push_back(key);
        }
        EXPECT(vals.contains(key));
    }
    auto dist = (size_t) distance(vals.begin(), vals.end());
    EXPECT(inserted == dist);
    cout << "--- Stats\n";
    vals.dumpStats(cout);
    if (s_verbose)
        cout << endl;

    s_verbose = false;
    for (auto i = 0; i < keys.size(); ++i) {
        if (i == keys.size() - 1)
            s_verbose = true;
        if (s_verbose)
            cout << i + 1 << " ";
        auto & key = keys[i];
        erase(&vals, key);
        EXPECT(!vals.contains(key));
    }
    EXPECT(vals.empty());
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
    cli.opt<bool>(&s_verbose, "v verbose.")
        .desc("Dump container state between tests.");
    auto & fill = cli.opt<int>("f fill").siUnits("")
        .desc("Randomly fill a container with this many values.");
    auto & test = cli.opt<bool>("test", false).desc("Run internal unit tests");
    if (!cli.parse(argc, argv))
        return appSignalUsageError();
    if (!*test && !*fill) {
        cout << "No tests run" << endl;
        return appSignalShutdown(EX_OK);
    }

    if (*test)
        internalTests();
    if (*fill)
        randomFill(*fill, 25, 26);

    testSignalShutdown();
    if (*fill)
        logStopwatch();
}


/****************************************************************************
*
*   External
*
***/

//===========================================================================
int main(int argc, char * argv[]) {
    return appRun(app, argc, argv, kVersion);
}