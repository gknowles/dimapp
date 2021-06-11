// Copyright Glen Knowles 2017 - 2021.
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
    EXPECT(os.view() == "5");
}

//===========================================================================
static void testParse() {
    int line = 0;

    //-----------------------------------------------------------------------
    // parse
    string to0;
    int to1;
    bool result = parse(&to0, "100");
    EXPECT(result && to0 == "100");
    result = parse(&to1, "100");
    EXPECT(result && to1 == 100);
}

//===========================================================================
static void testSplit() {
    vector<string_view> out;

    //-----------------------------------------------------------------------
    // split(ch)
    struct {
        string_view src;
        vector<string_view> out;
        int line;
    } chTests[] = {
        { "",     { "" },           __LINE__ },
        { " ",    { "", "" },       __LINE__ },
        { "a b ", { "a", "b", "" }, __LINE__ },
        { " a b", { "", "a", "b" }, __LINE__ },
    };
    for (auto && t : chTests) {
        split(&out, t.src);
        if (out != t.out) {
            logMsgError() << "Line " << t.line << ": "
                << "'" << t.src << "' / ' ' == '" << toString(out)
                << "', should be '" << toString(t.out) << "'";
        }
    }

    //-----------------------------------------------------------------------
    // split(str)
    for (auto && t : chTests) {
        split(&out, t.src, " ");
        if (out != t.out) {
            logMsgError() << "Line " << t.line << ": "
                << "'" << t.src << "' / ' ' == '" << toString(out)
                << "', should be '" << toString(t.out) << "'";
        }
    }
}

//===========================================================================
static void testCopy() {
    //char buf[10];
    wchar_t wbuf[10];
    struct {
        string_view src;
        wstring_view dst;
        int line;
    } tests[] = {
        { "abcdefghi",  L"abcdefghi", __LINE__ },
        { "abcdefghij", L"abcdefghi", __LINE__ },
    };
    for (auto&& t : tests) {
        memset(wbuf, '\xE', size(wbuf) * sizeof *wbuf);
        strCopy(wbuf, size(wbuf), t.src);
        if (t.dst != wbuf) {
            logMsgError() << "Line " << t.line << ": "
                << "'" << t.src << "' == '" << utf8(wbuf)
                << "', should be '" << utf8(t.dst) << "'";
        }
    }
}

//===========================================================================
static void app(int argc, char *argv[]) {
    testParse();
    testStrToInt();
    testIntegralStr();
    testSplit();
    testCopy();

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
    return appRun(app, argc, argv);
}
