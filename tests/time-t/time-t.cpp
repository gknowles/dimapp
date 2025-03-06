// Copyright Glen Knowles 2017 - 2022.
// Distributed under the Boost Software License, Version 1.0.
//
// time-t.cpp - dim test time
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
static void app(Cli & cli) {
    [[maybe_unused]] int line = 0;

    //-----------------------------------------------------------------------
    // timeParse8601
    EXPECT_PARSE("1970-01-01", "1970-01-01T00:00:00.0000000Z");
    EXPECT_PARSE("1970-01-01T00:00:00Z", "1970-01-01T00:00:00.0000000Z");
    EXPECT_PARSE("2000-99-99T99:99:99Z", "2008-06-11T04:40:39.0000000Z");
    EXPECT_PARSE("1970-01-01T00:00:00.123Z", "1970-01-01T00:00:00.1230000Z");
    EXPECT_PARSE("1970-01-01T00:00:00.123456789Z", "1970-01-01T00:00:00.1234567Z");
    EXPECT_PARSE("2000-06-15T12:00:00-07:00", "2000-06-15T19:00:00.0000000Z");

    //-----------------------------------------------------------------------
    // toString
    EXPECT(toString(1min, DurationFormat::kOnePart) == "1m");
    EXPECT(toString(72s, DurationFormat::kOnePart) == "1.2m");
    EXPECT(toString(80s, DurationFormat::kOnePart) == "1.333m");
    EXPECT(toString(800s, DurationFormat::kOnePart) == "13.33m");
    EXPECT(toString(1200ms, DurationFormat::kOnePart) == "1.2s");
    EXPECT(toString(200ms, DurationFormat::kOnePart) == "200ms");

    EXPECT(toString(1min, DurationFormat::kTwoPart) == "1m");
    EXPECT(toString(72s, DurationFormat::kTwoPart) == "1m 12s");
    EXPECT(toString(80s, DurationFormat::kTwoPart) == "1m 20s");
    EXPECT(toString(800s, DurationFormat::kTwoPart) == "13m 20s");
    EXPECT(toString(24 * 500h, DurationFormat::kTwoPart) == "1y 19w");
    EXPECT(toString(1200ms, DurationFormat::kTwoPart) == "1s 200ms");
    EXPECT(toString(200ms, DurationFormat::kTwoPart) == "200ms");

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
