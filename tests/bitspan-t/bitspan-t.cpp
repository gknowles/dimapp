// Copyright Glen Knowles 2018 - 2025.
// Distributed under the Boost Software License, Version 1.0.
//
// bitspan-t.cpp - dim test bitspan
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
*   Helpers
*
***/

//===========================================================================
static void invert(string * val) {
    for (auto i = 0; i < val->size(); ++i)
        (*val)[i] = ~(unsigned char)(*val)[i];
}

/****************************************************************************
*
*   Tests
*
***/

struct CopyTest {
    size_t cnt;
    string dst;
    size_t dpos;
    string src;
    size_t spos;
    string out;
    source_location sloc = source_location::current();
};

enum TestMode {
    kNormal,
    kInverted,
};

//===========================================================================
static bool execTest(const CopyTest & t, TestMode mode = kNormal) {
    string src, dst, out;
    if (!hexToBytes(&src, t.src)
        || !hexToBytes(&dst, t.dst)
        || !hexToBytes(&out, t.out)
        ) {
        logMsgError() << "Line " << t.sloc.line()
            << ": Invalid test definition.";
        return false;
    }
    if (mode == kInverted) {
        invert(&src);
        invert(&dst);
        invert(&out);
    }
    auto ddat = dst.data();
    auto sdat = src.data();
    BitSpan::copy(ddat, t.dpos, sdat, t.spos, t.cnt);
    if (dst != out) {
        logMsgError() << "Line " << t.sloc.line()
            << (mode == kInverted ? " inverted" : "")
            << ": output '" << hexFromBytes(dst) << "', expected '"
            << hexFromBytes(out);
        return false;
    }
    return true;
}

//===========================================================================
static void copyTests() {
    int line = 0;

    uint64_t buf[3] = {};
    BitSpan v(buf, size(buf));
    EXPECT(true);

    CopyTest tests[] = {
        // cnt     dst        src       result
        {   0,   "ff", 0,   "00", 0,    "ff" },
        {   8,   "00", 0,   "ff", 0,    "ff" },
        {   8,   "ff00", 8, "00ff", 8,  "ffff" },

        {   1,   "7f", 0,   "40", 1,   "ff" },
        {   1,   "bf", 1,   "40", 1,   "ff" },
        {   1,   "df", 2,   "40", 1,   "ff" },
        {   1,   "fe", 7,   "40", 1,   "ff" },
        {   1,   "fe", 7,   "80", 0,   "ff" },
        {   1,   "fe", 7,   "01", 7,   "ff" },
        {   7,   "01", 0,   "7f", 1,   "ff" },
        {   7,   "80", 1,   "fe", 0,   "ff" },

        {   3,   "1f", 0,   "01c0", 7, "ff" },
        {   3,   "e3", 3,   "0380", 6, "ff" },
        {   3,   "f8", 5,   "01c0", 7, "ff" },

        {   8,   "e01f",   3,   "1fe0",   3,   "ffff" },
        {  16,   "e0001f", 3,   "1fffe0", 3,   "ffffff" },

        {   3,   "fe3f", 7,   "e0",   0,   "ffff" },
        {   3,   "fc7f", 6,   "07",   5,   "ffff" },
        {   3,   "fe3f", 7,   "0380", 6,   "ffff" },

        {  11,   "fe003f", 7, "ffe0", 0,   "ffffff" },
        {  11,   "fe003f", 7, "03ff80", 6, "ffffff" },
        {  11,   "fc007f", 6, "01ffc0", 7, "ffffff" },
        {  22,   "800001", 1, "fffffc", 0, "ffffff" },
        {  22,   "800001", 1, "7ffffe", 1, "ffffff" },
        {  22,   "800001", 1, "3fffff", 2, "ffffff" },
    };
    for (auto&& t : tests) {
        execTest(t);
        execTest(t, kInverted);
    }
}

//===========================================================================
static void setBitTests() {
    int line = 0;

    uint64_t buf[3] = {};
    BitSpan v(buf, size(buf));
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
}

//===========================================================================
static void findTests() {
    int line = 0;

    uint64_t buf[3] = {};
    BitSpan v(buf, size(buf));
    v.set();
    EXPECT(v.find(0) == 0);
    EXPECT(v.find(1) == 1);
    EXPECT(v.find(64 * size(buf) - 1) == 64 * size(buf) - 1);
    EXPECT(v.find(64 * size(buf)) == BitView::npos);
    EXPECT(v.rfind(0) == 0);
    EXPECT(v.rfind(1) == 1);
    EXPECT(v.rfind(64 * size(buf) - 1) == 64 * size(buf) - 1);
    EXPECT(v.rfind() == 64 * size(buf) - 1);
    EXPECT(v.findZero() == v.npos);
    EXPECT(v.rfindZero() == v.npos);

    v.reset().set(1).set(3);
    EXPECT(v.find(0) == 1);
    EXPECT(v.find(2) == 3);
    EXPECT(v.find(4) == BitView::npos);
    EXPECT(v.rfind(2) == 1);
    EXPECT(v.rfind(4) == 3);
    EXPECT(v.rfind(48) == 3);
    EXPECT(v.rfind(0) == v.npos);
    EXPECT(v.findZero(0) == 0);
    EXPECT(v.findZero(1) == 2);
    EXPECT(v.rfindZero() == 64 * size(buf) - 1);
    EXPECT(v.rfindZero(2) == 2);
    EXPECT(v.rfindZero(1) == 0);
}


/****************************************************************************
*
*   Application
*
***/

//===========================================================================
static void app(Cli & cli) {
    copyTests();
    setBitTests();
    findTests();

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
