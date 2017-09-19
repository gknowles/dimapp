// Copyright Glen Knowles 2017.
// Distributed under the Boost Software License, Version 1.0.
//
// strtest.cpp - dim test str
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
    char * eptr;

    //-----------------------------------------------------------------------
    // int
    EXPECT(strToInt("1234", &eptr) == 1234 && *eptr == 0);
    EXPECT(strToInt(" 1234 ", &eptr) == 1234 && *eptr == ' ');
    EXPECT(strToInt("+1234") == 1234);
    EXPECT(strToInt(" +1234") == 1234);
    EXPECT(strToInt("1234a", &eptr) == 1234 && *eptr == 'a');
    EXPECT(strToInt("+ 1234") == 0);
    EXPECT(strToInt("01234") == 1234);
    EXPECT(strToInt("-1234") == -1234);

    // clamp overflow
    EXPECT(strToInt("2147483647") == INT_MAX);
    EXPECT(strToInt("2147483648", &eptr) == INT_MAX && *eptr == 0); 
    auto num = strToInt("9876543210", &eptr);
    EXPECT(num == INT_MAX && *eptr == 0); 
    num = strToInt("-9876543210", &eptr);
    EXPECT(num == INT_MIN && *eptr == 0);

    //-----------------------------------------------------------------------
    // int64
    EXPECT(strToInt64("1234", &eptr) == 1234 && !*eptr);

    // clamp int64 overflow
    EXPECT(strToInt64("9223372036854775807") == INT64_MAX);
    EXPECT(strToInt64("9223372036854775808", &eptr) == INT64_MAX && !*eptr); 
    EXPECT(strToInt64("12345678901234567890", &eptr) == INT64_MAX && !*eptr); 
    EXPECT(strToInt64("-12345678901234567890", &eptr) == INT64_MIN && !*eptr);

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
