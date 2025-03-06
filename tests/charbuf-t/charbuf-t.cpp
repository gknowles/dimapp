// Copyright Glen Knowles 2016 - 2025.
// Distributed under the Boost Software License, Version 1.0.
//
// charbuf-t.cpp - dim test charbuf
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
static void app(Cli & cli) {
    int line = 0;

    CharBuf buf;
    buf.assign("abcdefgh");
    EXPECT(toString(buf) == "abcdefgh"); // toString

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

    // insert into non-first block
    auto len = 3 * blkLen / 2;
    buf.assign(len, 'a');
    buf.insert(blkLen - 2, "x");
    EXPECT(buf.size() == len + 1);
    EXPECT(buf.compare(blkLen - 4, 5, "aaxaa") == 0);

    // append when exactly on block boundary
    buf.clear();
    buf.assign(2 * blkLen, 'a');
    buf.append("b");
    EXPECT(buf.size() == 2 * blkLen + 1);

    size_t count = 0;
    for (auto && view : buf.views()) {
        count += view.size();
    }
    EXPECT(count == buf.size());

    CharBuf buf2;
    buf2.assign(2 * blkLen, 'b');
    buf.insert(blkLen / 2, buf2);
    EXPECT(buf.size() == count + buf2.size());

    buf.assign("a");
    buf.resize(0);

    buf.pushBack('b');
    EXPECT(buf == "b");
    buf.pushFront('a');
    EXPECT(buf == "ab");
    buf.popFront();
    EXPECT(buf == "b");
    buf.popFront();
    EXPECT(buf == "");

    // Multiblock find, skip, and trim
    buf.clear();
    buf.append(blkLen / 2, 'a');
    buf.append(blkLen / 2, 'b');
    buf2 = buf;
    buf2.append(buf);
    EXPECT(buf2.find('a') == 0);
    EXPECT(buf2.find('b') == blkLen / 2);
    EXPECT(buf2.find('c') == -1);
    EXPECT(buf2.find('a', blkLen - 1) == blkLen);
    EXPECT(buf2.skip('b') == 0);
    EXPECT(buf2.skip('a') == blkLen / 2);
    EXPECT(buf2.skip('b', blkLen / 2) == blkLen);
    EXPECT(buf2.rskip('b') == 3 * blkLen / 2);
    EXPECT(buf2.rskip('a', blkLen / 2 - 1) == -1);
    buf2.ltrim('a').rtrim('b');
    EXPECT(buf2.size() == blkLen);

    // Multiblock move and view
    buf2.assign(2 * blkLen, 'b');
    buf = buf2;
    auto buf3 = move(buf);
    auto v3 = buf3.view();
    buf = v3;
    buf3 = move(buf);
    EXPECT(buf2.size() == buf3.size());

    {
        // Copy construct and destroy CharBuf that's greater than block size
        buf2.assign(2 * blkLen, 'b');
        auto tmp = buf2;
    }

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
