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
    num = trailingZeroBits(1);
    EXPECT(num == 0);
    num = trailingZeroBits(0x8000'0000'0000'0000);
    EXPECT(num == 63);
    num = leadingZeroBits(1);
    EXPECT(num == 63);
    num = leadingZeroBits(0x8000'0000'0000'0000);
    EXPECT(num == 0);

    char buf[100];
    double dval = 1.0f;
    auto ptr = htonf64(buf, dval);
    auto dout = ntohf64(ptr);
    EXPECT(dval == dout);
    ptr = hton16(buf, 32000);
    auto o16 = ntoh16(ptr);
    EXPECT(o16 == 32000);
    o16 = ntoh16(hton16(buf, 33000));
    EXPECT(o16 == 33000);
    o16 = ntoh16(hton16(buf, (unsigned) -2));
    EXPECT(o16 == 65534);

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
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF
        | _CRTDBG_LEAK_CHECK_DF
        | _CRTDBG_DELAY_FREE_MEM_DF);
    _set_error_mode(_OUT_TO_MSGBOX);
    return appRun(app, argc, argv);
}
