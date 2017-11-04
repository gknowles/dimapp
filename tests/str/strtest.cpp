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
static void testStrToInt() {
    int line = 0;
    char * eptr;

    //-----------------------------------------------------------------------
    // dec int
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
    // dec int64
    EXPECT(strToInt64("1234", &eptr) == 1234 && !*eptr);

    // clamp int64 overflow
    EXPECT(strToInt64("9223372036854775807") == INT64_MAX);
    EXPECT(strToInt64("9223372036854775808", &eptr) == INT64_MAX && !*eptr); 
    EXPECT(strToInt64("12345678901234567890", &eptr) == INT64_MAX && !*eptr); 
    EXPECT(strToInt64("-12345678901234567890", &eptr) == INT64_MIN && !*eptr);

    //-----------------------------------------------------------------------
    // oct & hex
    EXPECT(strToInt("1234", &eptr, 8) == 668 && *eptr == 0);
    EXPECT(strToInt("0678", &eptr, 0) == 55 && *eptr == '8');
    EXPECT(strToInt("0678", &eptr, 8) == 55 && *eptr == '8');
    EXPECT(strToInt("67", &eptr, 8) == 55 && *eptr == 0);
    EXPECT(strToInt("0x8aceg", &eptr, 0) == 0x8ace && *eptr == 'g');
    EXPECT(strToInt("0x1234", &eptr, 0) == 0x1234 && !*eptr);
    EXPECT(strToInt("0x1234", &eptr, 16) == 0x1234 && !*eptr);
    EXPECT(strToInt("1234", &eptr, 16) == 0x1234 && !*eptr);
    EXPECT(strToInt("12345678", &eptr, 16) == 0x12345678 && !*eptr);
    EXPECT(strToInt("012345678", &eptr, 16) == 0x12345678 && !*eptr);
    EXPECT(strToInt("123456789", &eptr, 16) == INT_MAX && !*eptr);
}

//===========================================================================
static void testIntegralStr() {
    int line = 0;

    //-----------------------------------------------------------------------
    // unsigned
    EXPECT(StrFrom<unsigned>(1234) == "1234"sv);
    EXPECT(StrFrom<unsigned>(0xffff'ffff) == "4294967295"sv);
    EXPECT(StrFrom<unsigned>(3) == "3"sv);

    //-----------------------------------------------------------------------
    // char
    EXPECT(StrFrom<char>(1) == "1"sv);
    EXPECT(StrFrom<char>(127) == "127"sv);
    EXPECT(StrFrom<char>(-3) == "-3"sv);
    EXPECT(StrFrom<char>(-128) == "-128"sv);

    //-----------------------------------------------------------------------
    // double
    EXPECT(StrFrom<double>(-0.101234567890123456789e+101) == 
        "-1.0123456789012345e+100"sv);

    //-----------------------------------------------------------------------
    // ostream
    ostringstream os;
    os << StrFrom<int>(5);
    EXPECT(os.str() == "5"sv);
}

//===========================================================================
static void app(int argc, char *argv[]) {
    testStrToInt();
    testIntegralStr();

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
