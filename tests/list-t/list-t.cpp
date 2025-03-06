// Copyright Glen Knowles 2017 - 2022.
// Distributed under the Boost Software License, Version 1.0.
//
// list-t.cpp - dim test list
#include "pch.h"
#pragma hdrstop

using namespace std;
using namespace Dim;

#define ANON_NAMESPACE namespace Dim::Anon::Test::List
ANON_NAMESPACE {}
using ANON_NAMESPACE;


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

ANON_NAMESPACE {

class TestNode : public ListLink<> {
public:
    unsigned m_value;
};

} // namespace


/****************************************************************************
*
*   Application
*
***/

//===========================================================================
static void app(Cli & cli) {
    int line = 0;

    List<TestNode> list;
    EXPECT(list.empty());
    {
        TestNode a;
        list.link(&a);
        EXPECT(list.size() == 1);
    }
    EXPECT(list.empty());
    TestNode b;
    TestNode c;
    list.link(&b);
    list.link(&c);
    EXPECT(list.size() == 2);
    TestNode d;
    list.linkAfter(&b, &d);
    EXPECT(list.size() == 3);
    int num = 0;
    for (auto && v : list) {
        num += v.m_value ? (bool) v.m_value : 1;
    }
    EXPECT(num == 3);
    list.unlinkAll();
    EXPECT(list.empty());

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
