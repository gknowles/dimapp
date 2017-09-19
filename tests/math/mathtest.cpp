// Copyright Glen Knowles 2017.
// Distributed under the Boost Software License, Version 1.0.
//
// mathtest.cpp - dim test core
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
static void app(int argc, char *argv[]) {
    int line = 0;

    auto num = digits10(1'000'000'000);
    EXPECT(num == 10);
    num = digits10(999'999'999);
    EXPECT(num == 9);
    num = hammingWeight(0xffff'ffff'ffff'ffff);
    EXPECT(num == 64);
    num = hammingWeight(0xff00'0000'0000'0000);
    EXPECT(num == 8);

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
    return appRun(app, argc, argv);
}
