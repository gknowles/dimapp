// Copyright Glen Knowles 2017.
// Distributed under the Boost Software License, Version 1.0.
//
// timetest.cpp - dim test time
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
#define EXPECT2(src, dst) parseTest(__LINE__, src, dst)


/****************************************************************************
*
*   Helpers
*
***/

//===========================================================================
static void parseTest(int line, string_view src, string_view dst) {
    TimePoint time;
    EXPECT(timeParse8601(&time, src));
    Time8601Str str(time, 7);
    EXPECT(str.view() == dst);
}


/****************************************************************************
*
*   Application
*
***/

//===========================================================================
static void app(int argc, char *argv[]) {
    int line [[maybe_unused]] = 0;

    //-----------------------------------------------------------------------
    EXPECT2("1970-01-01", "1970-01-01T00:00:00.0000000Z");
    EXPECT2("1970-01-01T00:00:00Z", "1970-01-01T00:00:00.0000000Z");
    EXPECT2("2000-99-99T99:99:99Z", "2008-06-11T04:40:39.0000000Z");
    EXPECT2("1970-01-01T00:00:00.123Z", "1970-01-01T00:00:00.1230000Z");
    EXPECT2("1970-01-01T00:00:00.123456789Z", "1970-01-01T00:00:00.1234567Z");
    EXPECT2("2000-06-15T12:00:00-07:00", "2000-06-15T19:00:00.0000000Z");

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
