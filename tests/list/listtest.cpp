// Copyright Glen Knowles 2017.
// Distributed under the Boost Software License, Version 1.0.
//
// listtest.cpp - dim test list
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

namespace {

class TestNode {
public:
    unsigned m_value;
    ListMemberLink<TestNode> m_link;
};

} // namespace


/****************************************************************************
*
*   Helpers
*
***/


/****************************************************************************
*
*   Application
*
***/

namespace {

class Application : public IAppNotify {
    // IAppNotify
    void onAppRun() override;
};

} // namespace

//template <typename T, int T::*P> class L1;
//
//template <auto P> class L2;
//template <typename T, int T::*P> class L2<int T::*P>;

//===========================================================================
void Application::onAppRun() {
    int line = 0;

    List<TestNode, &TestNode::m_link> list;
    EXPECT(list.empty());
    {
        TestNode a;
        list.pushBack(&a);
        EXPECT(list.size() == 1);
    }
    EXPECT(list.empty());
    TestNode b;
    list.pushBack(&b);
    list.unlinkAll();
    EXPECT(list.empty());

    //Thing<&TestNode::m_base> t(&TestNode::m_base);

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
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
    _set_error_mode(_OUT_TO_MSGBOX);
    Application app;
    return appRun(app, argc, argv);
}
