// Copyright Glen Knowles 2017 - 2023.
// Distributed under the Boost Software License, Version 1.0.
//
// intset-t.cpp - dim test intset
#include "pch.h"
#pragma hdrstop

using namespace std;
using namespace Dim;


/****************************************************************************
*
*   Declarations
*
***/

static int s_passed;

#define EXPECT(...)                                                         \
    if (!bool(__VA_ARGS__)) {                                               \
        logMsgError() << "Line " << (line ? line : __LINE__) << ": EXPECT(" \
                      << #__VA_ARGS__ << ") failed";                        \
    } else {                                                                \
        ++s_passed;                                                         \
    }


/****************************************************************************
*
*   Application
*
***/

template <typename T>
void expect(
    const IntegralSet<T> & set,
    string val,
    source_location loc = source_location::current()
) {
    if constexpr (is_signed_v<T>)
        val = regex_replace(val, regex("(\\d)-(\\d)"), "$1..$2");
    ostringstream os;
    os << set;
    if (val != os.view()) {
        logMsgError() << "Line " << loc.line() << ": EXPECT(" << val
            << ") failed";
    } else {
        ++s_passed;
    }
}

//===========================================================================
template <typename T>
static void memleak() {
    int line = 0;
    ostringstream os;

    // test.eight.seven.*
    IntegralSet<T> c0, c1;
    c0.assign("1-10000");
    expect(c0, "1-10000");

    c1.assign("801-900 8001-9000");
    expect(c1, "801-900 8001-9000");

    c0.intersect(c1);
    EXPECT(c0 == c1);
    expect(c0, "801-900 8001-9000");
}

//===========================================================================
// Test combining Node::kVector nodes.
// Use too many values for kSmVector, but not so many it converts to kBitmap.
template <typename T>
static void vectors() {
    int line = 0;
    IntegralSet<T> a;
    IntegralSet<T> b;

    a.assign("1 3 5 7 9");
    b.assign("4 6 8");
    a.insert(b);
    EXPECT(a.size() == 8);
    expect(a, "1 3-9");

    a.assign("1 3 5 7");
    b.assign("4-6");
    a.insert(b);
    EXPECT(a.size() == 6);
    expect(a, "1 3-7");

    b.clear();
    b.insert(a);
    expect(b, "1 3-7");
    b.insert(a);
    expect(b, "1 3-7");

    a.assign("1-100");
    b.assign("90-200");
    a.insert(b);
    expect(a, "1-200");
}

//===========================================================================
template <typename T>
static void test() {
    int line = 0;

    IntegralSet<T> tmp;
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

    IntegralSet<T> tmp2;
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
    auto rb = r.begin();
    auto re = r.end();
    vr.assign(rb, re);
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

    tmp.assign("1 3 5");
    tmp2.assign("2 4 6");
    EXPECT(!tmp.intersects(tmp2));
    tmp.insert(4);
    EXPECT(tmp.intersects(tmp2));

    // Swap with meta node.
    tmp.clear();
    tmp2.assign("1-5000 10001-15000 20001-25000");
    swap(tmp, tmp2);
    tmp.erase(2, 4999);
    tmp.erase(10002, 4999);
    tmp.erase(20002, 4999);
    size_t cnt = tmp.count();
    EXPECT(cnt == 3);
    cnt = 0;
    for ([[maybe_unused]] auto&& v : tmp)
        cnt += 1;
    EXPECT(cnt == 3);
}

//===========================================================================
template <typename T>
static void allTests() {
    memleak<T>();
    test<T>();
    vectors<T>();
}

//===========================================================================
static void app(Cli & cli) {
    allTests<int>();
    allTests<unsigned>();
    testSignalShutdown();
}


/****************************************************************************
*
*   External
*
***/

//===========================================================================
int main(int argc, char * argv[]) {
    Cli().action(app);
    return appRun(argc, argv);
}
