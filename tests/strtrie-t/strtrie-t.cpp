// Copyright Glen Knowles 2019 - 2025.
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
static void check(
    bool expr,
    const source_location base = source_location::current(),
    const source_location rel = source_location::current()
) {
    if (!expr) {
        auto os = logMsgError();
        os << "Line " << base.line();
        if (base.line() != rel.line())
            os << " (detected at " << rel.line() << ")";
        os << ": failed";
    }
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
    vals->debugStream(s_verbose ? &cout : nullptr);
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
    const vector<string_view> & ghosts = {},
    source_location loc = source_location::current()
) {
    StrTrie vals;
    for (auto&& str : strs) {
        check(insert(&vals, str), loc);
        check(vals.contains(str), loc);
    }
    for (auto&& str : strs) {
        check(vals.contains(str), loc);
        check(!insert(&vals, str), loc);
    }
    for (auto&& str : ghosts)
        check(!vals.contains(str), loc);
    for (auto&& str : strs) {
        check(erase(&vals, str), loc);
        check(!vals.contains(str), loc);
    }
    check(vals.empty(), loc);
}

//===========================================================================
inline static void internalTests() {
    if (s_verbose)
        cout << "\n> INTERNAL TESTS" << endl;
    StrTrie vals;
    string out;
    check(vals.empty());

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
    vals.clear();
    vals.clear(); // test clearing empty container
    check(vals.begin() == vals.end());

    // Search and iterate
    vector<string_view> keys = {
        "abc", "aw", "abd",
        "bbb",
        "cc", "ccc", "ccccc",
        "z"
    };
    for (auto&& key : keys)
        insert(&vals, key);
    ranges::sort(keys);
    // Forward iterators
    auto i = keys.begin();
    for (auto&& key : vals) {
        check(key == *i);
        i += 1;
    }
    check(i == keys.end());
    // Reverse iterators
    auto ri = keys.rbegin();
    for (auto rvi = vals.rbegin(); rvi != vals.rend(); ++rvi) {
        check(*rvi == *ri);
        ri += 1;
    }
    check(ri == keys.rend());

    auto vi = vals.find("abd");
    check(*vi == "abd");
    vi = vals.find("bb");
    check(!vi);
    vi = vals.find("cc");
    check(*vi == "cc");
    vi = vals.find("ccccc");
    check(*vi == "ccccc");

    vi = vals.findLess("abd");
    check(*vi == "abc");
    vi = vals.findLess("cccc");
    check(*vi == "ccc");
    vi = vals.findLess("cccd");
    check(*vi == "ccccc");
    vi = vals.findLess("ccccd");
    check(*vi == "ccccc");

    vi = vals.findLessEqual("abd");
    check(*vi == "abd");
    vi = vals.findLessEqual("ccccd");
    check(*vi == "ccccc");

    vi = vals.lowerBound("abcd");
    check(*vi == "abd");
    vi = vals.lowerBound("aa");
    check(*vi == "abc");
    vi = vals.lowerBound("ab");
    check(*vi == "abc");
    vi = vals.lowerBound("~");
    check(!vi);
    vi = vals.lowerBound("abd");
    check(*vi == "abd");

    vi = vals.upperBound("abd");
    check(*vi == "aw");
    vi = vals.upperBound("ccccb");
    check(*vi == "ccccc");

    auto ii = vals.equalRange("abc");
    check(*ii.first == "abc");
    check(*ii.second == "abd");
    ii = vals.equalRange("abe");
    check(*ii.first == "aw" && ii.first == ii.second);

    check(vals.front() == "abc");
    check(vals.back() == "z");

    for (auto&& key : keys) {
        erase(&vals, key);
        check(!vals.contains(key));
    }
    check(vals.empty());
}

//===========================================================================
static string toKey(uint64_t val) {
    const char * names[] = {
        "zero", "one", "two", "three", "four",
        "five", "six", "seven", "eight", "nine",
    };
    string out;
    for (; val >= 10; val /= 10) {
        out += names[val % 10];
        out += '.';
    }
    out += names[val];
    return out;
}

//===========================================================================
inline static void fillTests() {
    if (s_verbose)
        cout << "\n> FILL TESTS" << endl;
    StrTrie vals;
    for (auto i = 0; i < 1000; ++i)
        vals.insert(toKey(i));
    ostringstream out;
    vals.dumpStats(out);
    out.str({});
    vals.debugStream(&out);
    vals.StrTrieBase::clear();
    check(vals.empty());
}

//===========================================================================
inline static void randomFill(size_t count, size_t maxLen, size_t charVals) {
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
        check(vals.contains(key));
    }
    auto dist = (size_t) distance(vals.begin(), vals.end());
    check(inserted == dist);
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
        check(!vals.contains(key));
    }
    check(vals.empty());
    cout << endl;
}


/****************************************************************************
*
*   Application
*
***/

static int s_fill;
static bool s_test;

//===========================================================================
static void app(Cli & cli) {
    if (!s_test && !s_fill) {
        cout << "No tests run" << endl;
        return appSignalShutdown(EX_OK);
    }

    if (s_test) {
        internalTests();
        fillTests();
    }
    if (s_fill)
        randomFill(s_fill, 25, 26);

    testSignalShutdown();
    if (s_fill)
        logStopwatch();
}


/****************************************************************************
*
*   External
*
***/

//===========================================================================
int main(int argc, char * argv[]) {
    cout.imbue(locale(""));
    Cli cli;
    cli.helpNoArgs().action(app);
    cli.opt(&s_verbose, "v verbose.")
        .desc("Dump container state between tests.");
    cli.opt(&s_fill, "f fill").siUnits("")
        .desc("Randomly fill a container with this many values.");
    cli.opt(&s_test, "test", false).desc("Run internal unit tests");
    return appRun(argc, argv, kVersion, {}, fAppTest);
}
