// Copyright Glen Knowles 2018 - 2022.
// Distributed under the Boost Software License, Version 1.0.
//
// bitview-t.cpp - dim test bitview
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

    uint64_t buf[3] = {};
    BitView v(buf, size(buf));
    string_view sv((char *) buf, sizeof buf);
    EXPECT(v.size() == 3);
    EXPECT(v.count() == 0);
    v.set().setBits(24, 64, (uint64_t) 0x0123'4567'89ab'cdef);
    EXPECT(sv.substr(0, 16) ==
        "\xff\xff\xff\x01\x23\x45\x67\x89"
        "\xab\xcd\xef\xff\xff\xff\xff\xff"
    );
    v.setBits(16, 16, 0x1234);
    EXPECT(sv.substr(0, 8) == "\xff\xff\x12\x34\x23\x45\x67\x89");
    v.setBits(64, 64, 0);
    EXPECT(buf[1] == 0);
    v.setBits(24, 8, 0xef);
    EXPECT(sv.substr(0, 8) == "\xff\xff\x12\xef\x23\x45\x67\x89");
    v.reset().setBits(18, 1, 1);
    EXPECT(v[18] == 1);
    v.setBits(0, 64, 0x0123'4567'89ab'cdef);
    auto a = v.getBits(0, 64);
    EXPECT(a == 0x0123'4567'89ab'cdef);
    v.setBits(64, 64, a);
    v.setBits(128, 64, a);
    a = v.getBits(16, 64);
    EXPECT(a == 0x4567'89ab'cdef'0123);
    a = v.getBits(72, 8);
    EXPECT(a == 0x23);
    a = v.getBits(160, 48);
    EXPECT(a == 0x89ab'cdef'0000);
    a = v.getBits(144, 48);
    EXPECT(a == 0x4567'89ab'cdef);

    v.set();
    EXPECT(v.find(0) == 0);
    EXPECT(v.find(1) == 1);
    EXPECT(v.find(64 * size(buf) - 1) == 64 * size(buf) - 1);
    EXPECT(v.find(64 * size(buf)) == BitView::npos);
    EXPECT(v.rfind(0) == 0);
    EXPECT(v.rfind(1) == 1);
    EXPECT(v.rfind(64 * size(buf) - 1) == 64 * size(buf) - 1);
    EXPECT(v.rfind() == 64 * size(buf) - 1);

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
