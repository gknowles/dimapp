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
    ListMemberLinkBase<TestNode> m_base;
    //ListMemberLink<> m_link(&TestNode::m_base);
};

template <typename T> struct get_class;
template <typename T, typename V> struct get_class<V T::*> { using type = T; };

//template <ListMemberLinkBase<T> T::*M> 
//class Thing {
//public:
//    Thing(decltype(M) ignore) {}
//};

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

//===========================================================================
void Application::onAppRun() {
    int line = 0;
    line += 1;

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
