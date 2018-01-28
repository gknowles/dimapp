// Copyright Glen Knowles 2016 - 2018.
// Distributed under the Boost Software License, Version 1.0.
//
// charbuftest.cpp - dim test charbuf
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

    CharBuf buf;
    buf.assign("abcdefgh");
    EXPECT(toString(buf) == "abcdefgh"); // to_string

    // replace in the middle with sz
    buf.replace(3, 3, "DEF"); // same size
    EXPECT(buf == "abcDEFgh");
    buf.replace(3, 3, "MN"); // shrink
    EXPECT(buf == "abcMNgh");
    buf.replace(3, 2, "def"); // expand
    EXPECT(buf == "abcdefgh");

    // replace at the end with sz
    buf.replace(5, 3, "FGH"); // same size
    EXPECT(buf == "abcdeFGH");
    buf.replace(5, 3, "fg"); // shrink
    EXPECT(buf == "abcdefg");
    buf.replace(5, 2, "FGH"); // expand
    EXPECT(buf == "abcdeFGH");

    // replace in the middle with len
    buf.replace(3, 3, "XYZ", 3); // same size
    EXPECT(buf == "abcXYZGH");
    buf.replace(3, 3, "DE", 2); // shrink
    EXPECT(buf == "abcDEGH");
    buf.replace(3, 2, "def", 3); // expand
    EXPECT(buf == "abcdefGH");

    // replace at the end with len
    buf.replace(6, 2, "gh", 2); // same size
    EXPECT(buf == "abcdefgh");
    buf.replace(5, 3, "FG", 2); // shrink
    EXPECT(buf == "abcdeFG");
    buf.replace(5, 2, "fgh", 3); // expand
    EXPECT(buf == "abcdefgh");

    // insert amount smaller than tail
    buf.replace(4, 0, "x"); // insert one char
    EXPECT(buf == "abcdxefgh");
    buf.replace(4, 1, ""); // erase one char
    EXPECT(buf == "abcdefgh");

    auto blkLen = buf.defaultBlockSize();
    auto len = 3 * blkLen / 2;
    buf.assign(len, 'a');
    buf.insert(blkLen - 2, "x");
    EXPECT(buf.size() == len + 1);
    EXPECT(buf.compare(blkLen - 4, 5, "aaxaa") == 0);

    size_t count = 0;
    for (auto && view : buf.views()) {
        count += view.size();
    }
    EXPECT(count == buf.size());

    CharBuf buf2;
    buf2.assign(2 * blkLen, 'b');
    buf.insert(blkLen / 2, buf2);
    EXPECT(buf.size() == count + buf2.size());

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
