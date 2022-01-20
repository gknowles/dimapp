// Copyright Glen Knowles 2017 - 2022.
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
#define EXPECT_PARSE(src, dst) parseTest(__LINE__, src, dst)


/****************************************************************************
*
*   Helpers
*
***/

//===========================================================================
static void parseTest(int line, string_view src, string_view dst) {
    TimePoint time;
    EXPECT(timeParse8601(&time, src));
    Time8601Str str(time, 7, 0);
    EXPECT(str.view() == dst);
}


/****************************************************************************
*
*   Application
*
***/

//===========================================================================
static void app(int argc, char *argv[]) {
    [[maybe_unused]] int line = 0;

    //-----------------------------------------------------------------------
    EXPECT_PARSE("1970-01-01", "1970-01-01T00:00:00.0000000Z");
    EXPECT_PARSE("1970-01-01T00:00:00Z", "1970-01-01T00:00:00.0000000Z");
    EXPECT_PARSE("2000-99-99T99:99:99Z", "2008-06-11T04:40:39.0000000Z");
    EXPECT_PARSE("1970-01-01T00:00:00.123Z", "1970-01-01T00:00:00.1230000Z");
    EXPECT_PARSE("1970-01-01T00:00:00.123456789Z", "1970-01-01T00:00:00.1234567Z");
    EXPECT_PARSE("2000-06-15T12:00:00-07:00", "2000-06-15T19:00:00.0000000Z");

    testSignalShutdown();
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
