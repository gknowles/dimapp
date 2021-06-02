// Copyright Glen Knowles 2017 - 2021.
// Distributed under the Boost Software License, Version 1.0.
//
// usettest.cpp - dim test uset
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
static void memleak() {
    int line = 0;
    ostringstream os;

    // test.eight.seven.*
    UnsignedSet c0, c1;
    c0.assign("1-10000");
    os.str("");
    os << c0;
    EXPECT(os.str() == "1-10000");

    c1.assign("801-900 8001-9000");
    os.str("");
    os << c1;
    EXPECT(os.str() == "801-900 8001-9000");

    c0.intersect(c1);
    EXPECT(c0 == c1);

    os.str("");
    os << c0;
    EXPECT(os.str() == "801-900 8001-9000");
}

//===========================================================================
// Test combining Node::kVector nodes.
// Use too many values for kSmVector, but not so many it converts to kBitmap.
static void vectors() {
    int line = 0;
    UnsignedSet a;
    UnsignedSet b;
    ostringstream os;

    a.assign("1 3 5 7 9");
    b.assign("4 6 8");
    a.insert(b);
    EXPECT(a.size() == 8);
    os.str("");
    os << a;
    EXPECT(os.str() == "1 3-9");

    a.assign("1 3 5 7");
    b.assign("4-6");
    a.insert(b);
    EXPECT(a.size() == 6);
    os.str("");
    os << a;
    EXPECT(os.str() == "1 3-7");

    b.clear();
    b.insert(a);
    os.str("");
    os << b;
    EXPECT(os.str() == "1 3-7");
    b.insert(a);
    os.str("");
    os << b;
    EXPECT(os.str() == "1 3-7");

    a.assign("1-100");
    b.assign("90-200");
    a.insert(b);
    os.str("");
    os << a;
    EXPECT(os.str() == "1-200");
}

//===========================================================================
static void test() {
    int line = 0;

    UnsignedSet tmp;
    EXPECT(tmp.empty());
    tmp.insert(5);
    EXPECT(tmp.size() == 1);
    unsigned val = *tmp.begin();
    EXPECT(val == 5);
    vector<unsigned> v(tmp.begin(), tmp.end());
    EXPECT(v == vector<unsigned>{{5}});
    tmp.insert(3);
    v.assign(tmp.begin(), tmp.end());
    EXPECT(v == vector<unsigned>{{3, 5}});
    EXPECT(tmp.front() == 3);
    EXPECT(tmp.back() == 5);

    // Implementation detail defines: ++end() == begin()
    val = *++tmp.end();
    EXPECT(val == 3);
    val = *--tmp.end();
    EXPECT(val == 5);

    v.assign(tmp.rbegin(), tmp.rend());
    EXPECT(v == vector<unsigned>{{5, 3}});

    auto r = tmp.ranges();
    vector<pair<unsigned,unsigned>> vr(r.begin(), r.end());
    EXPECT(vr == vector<pair<unsigned,unsigned>>{{3,3},{5,5}});
    vr.assign(r.rbegin(), r.rend());
    EXPECT(vr == vector<pair<unsigned,unsigned>>{{5,5},{3,3}});

    UnsignedSet tmp2;
    tmp2.insert(5);
    tmp.intersect(tmp2);
    v.assign(tmp.begin(), tmp.end());
    EXPECT(v == vector<unsigned>{{5}});

    tmp.clear();
    EXPECT(tmp.empty());
    tmp.insert(0, 64);
    EXPECT(tmp.size() == 64);
    tmp.insert(4096, 64);
    EXPECT(tmp.size() == 128);
    tmp.insert(64);
    EXPECT(tmp.size() == 129);
    r = tmp.ranges();
    vr.assign(r.begin(), r.end());
    EXPECT(vr == vector<pair<unsigned,unsigned>>{{0,64},{4096,4159}});
    val = (++r.end())->first;
    EXPECT(val == 0);

    tmp.insert(4095);
    EXPECT(tmp.size() == 130);
    r = tmp.ranges();
    vr.assign(r.begin(), r.end());
    EXPECT(vr == vector<pair<unsigned,unsigned>>{{0,64},{4095,4159}});
    tmp.erase(4096);
    EXPECT(tmp.size() == 129);
    r = tmp.ranges();
    vr.assign(r.begin(), r.end());
    EXPECT(vr == vector<pair<unsigned,unsigned>>{
        {0,64},
        {4095,4095},
        {4097,4159}
    });
    tmp.insert(65, 4094 - 65 + 1);
    EXPECT(tmp.size() == 4159);
    r = tmp.ranges();
    vr.assign(r.begin(), r.end());
    EXPECT(vr == vector<pair<unsigned,unsigned>>{{0,4095},{4097,4159}});
    tmp2.clear();
    tmp2.insert(4097, 63);
    tmp.erase(tmp2);
    EXPECT(tmp.size() == 4096);
    r = tmp.ranges();
    vr.assign(r.begin(), r.end());
    EXPECT(vr == vector<pair<unsigned,unsigned>>{{0,4095}});
    tmp.erase(4095);
    r = tmp.ranges();
    vr.assign(r.begin(), r.end());
    EXPECT(vr == vector<pair<unsigned,unsigned>>{{0,4094}});

    tmp.assign("1-2");
    tmp2.assign("1 3");
    auto cmp = tmp.compare(tmp2);
    EXPECT(cmp < 0);
    tmp2.assign("1-3");
    cmp = tmp.compare(tmp2);
    EXPECT(cmp < 0); // was check for -2 before switch to std::strong_ordering
    tmp.assign("1-2 1000-1100");
    cmp = tmp.compare(tmp2);
    EXPECT(cmp > 0);

    tmp.assign("1-5000");
    tmp2.assign("10001-15000");
    tmp.intersect(tmp2);
    EXPECT(tmp.empty());
}

//===========================================================================
static void app(int argc, char *argv[]) {
    memleak();
    test();
    vectors();

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
