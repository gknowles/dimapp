// Copyright Glen Knowles 2018 - 2026.
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

struct CmdOpts {
    bool full = false;
    CmdOpts();
};
static CmdOpts s_opts;


/****************************************************************************
*
*   Helpers
*
***/

//===========================================================================
static bool checkErrorLimit() {
    static bool s_aborted = false;
    auto errs = logGetMsgCount(kLogTypeError);
    if (errs < 20)
        return true;
    if (!s_aborted) {
        s_aborted = true;
        logMsgError() << "Too many errors (" << errs << "): tests aborted.";
    }
    return false;
}

//===========================================================================
static void invert(string * val) {
    for (auto i = 0; i < val->size(); ++i)
        (*val)[i] = ~(unsigned char)(*val)[i];
}

//===========================================================================
// The sequences (<num>) and [<num>] are replaced with <num> '0' or 'f'
// characters respectively.
//
// Examples:
//  "5(2)6" -> "5006"
//  "[3]e(1)d" -> "fffe0d"
static string expand(const string & val) {
    string out;
    auto ptr = val.data();
    auto eptr = ptr + val.size();
    char * next = {};

    for (; ptr != eptr; ++ptr) {
        if (*ptr == '(') {
            auto num = strToUint(ptr + 1, &next);
            if (*next == ')') {
                ptr = next;
                out.append(num, '0');
                continue;
            }
        } else if (*ptr == '[') {
            auto num = strToUint(ptr + 1, &next);
            if (*next == ']') {
                ptr = next;
                out.append(num, 'f');
                continue;
            }
        }
        out += *ptr;
    }
    return out;
}

//===========================================================================
// Encodes consecutive sequences of 4 or more '0' or 'f' characters as (<num>)
// or [<num>] respecitvely.
static string reduce(const string & val) {
    string out;
    auto ptr = val.data();
    auto eptr = ptr + val.size();
    auto n = 0;
    char nch = 0;

IN_RAW:
    for (; ptr != eptr; ++ptr) {
        switch (*ptr) {
        case '0':
        case 'f':
            goto IN_KEY;
        case '1': case '2': case '3': case '4': case '5': case '6': case '7':
        case '8': case '9':
        case 'a': case 'b': case 'c': case 'd': case 'e':
            out += *ptr;
            break;
        default:
            assert(!"Invalid chars in hex string");
            out += *ptr;
            break;
        }
    }
    return out;

IN_KEY:
    nch = *ptr++;
    n = 1;
    for (; ptr != eptr; ++ptr) {
        if (*ptr != nch)
            break;
        n += 1;
    }
    if (n < 4) {
        out.append(n, nch);
    } else if (nch == '0') {
        out += "(" + toString(n) + ")";
    } else {
        assert(nch == 'f');
        out += "[" + toString(n) + "]";
    }
    goto IN_RAW;
}


/****************************************************************************
*
*   Tests - copy
*
***/

struct TestMem {
    string raw;
    size_t pos;
    size_t align;
};
struct CopyTest {
    size_t cnt;
    TestMem dst;
    TestMem src;
    string out;
    source_location sloc = source_location::current();
};

enum TestMode {
    kNormal,
    kInverted,
};

//===========================================================================
static bool test(const CopyTest & t, TestMode mode = kNormal) {
    string src, dst, out;
    if (!hexToBytes(&src, expand(t.src.raw))
        || !hexToBytes(&dst, expand(t.dst.raw))
        || !hexToBytes(&out, expand(t.out))
    ) {
        logMsgError() << "Line " << t.sloc.line()
            << ": Invalid copy test definition.";
        return false;
    }
    if (mode == kInverted) {
        invert(&src);
        invert(&dst);
        invert(&out);
    }
    dst.insert(0, t.dst.align, '\0');
    auto ddat = dst.data() + t.dst.align;
    src.insert(0, t.src.align, '\0');
    auto sdat = src.data() + t.src.align;
    BitSpan::copy(ddat, t.dst.pos, sdat, t.src.pos, t.cnt);
    if (dst != out) {
        logMsgError() << "Line " << t.sloc.line()
            << (mode == kInverted ? " inverted" : "")
            << ": output '" << reduce(hexFromBytes(dst)) << "'"
            << ", expected '" << reduce(hexFromBytes(out)) << "'";
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
        {  11, {"fe003f", 7 },  {"ffe0", 0 },    "[6]" },
        // cnt    dst        src       result
        // bitcpy8
        {   0, {"ff", 0 },   {"00", 0 },   "ff" },
        {   8, {"00", 0 },   {"ff", 0 },   "ff" },
        {   8, {"ff00", 8 }, {"00ff", 8 }, "[4]" },

        {   1, {"7f", 0 }, {"40", 1 }, "ff" },
        {   1, {"bf", 1 }, {"40", 1 }, "ff" },
        {   1, {"df", 2 }, {"40", 1 }, "ff" },
        {   1, {"fe", 7 }, {"40", 1 }, "ff" },
        {   1, {"fe", 7 }, {"80", 0 }, "ff" },
        {   1, {"fe", 7 }, {"01", 7 }, "ff" },
        {   7, {"01", 0 }, {"7f", 1 }, "ff" },
        {   7, {"80", 1 }, {"fe", 0 }, "ff" },

        {   3, {"1f", 0 }, {"01c0", 7 }, "ff" },
        {   3, {"e3", 3 }, {"0380", 6 }, "ff" },
        {   3, {"f8", 5 }, {"01c0", 7 }, "ff" },

        {   8, {"e01f", 3 },   {"1fe0", 3 },   "[4]" },
        {  16, {"e0001f", 3 }, {"1fffe0", 3 }, "[6]" },

        {   3, {"fe3f", 7 },    {"e0", 0 },      "[4]" },
        {   3, {"fc7f", 6 },    {"07", 5 },      "[4]" },
        {   3, {"fe3f", 7 },    {"0380", 6 },    "[4]" },
        {  11, {"fe003f", 7 },  {"ffe0", 0 },    "[6]" },
        {  11, {"fe003f", 7 },  {"03ff80", 6 },  "[6]" },
        {  11, {"fc007f", 6 },  {"01ffc0", 7 },  "[6]" },
        {  22, {"80(2)01", 1 }, {"ff[2]fc", 0 }, "[6]" },
        {  22, {"80(2)01", 1 }, {"7f[2]fe", 1 }, "[6]" },
        {  22, {"80(2)01", 1 }, {"3f[2]ff", 2 }, "[6]" },

        //-------------------------------------------------------------------
        // bitcpy64
        {   0,  {"[16]", 0 },      {"(16)", 0 },      "[16]" },
        {  64,  {"[16]", 0 },      {"[16]", 0 },      "[16]" },
        {  64,  {"[16](16)", 64 }, {"(16)[16]", 64 }, "[32]" },

        // 8 bytes from 9 bytes
        {  63,  {"(14)01", 0 },  {"01[14]fc", 7 },  "[16]" },
        {  63,  {"80(14)", 1 },  {"3f[14]80", 2 },  "[16]" },

        // Multiple qwords
        {  67,  {"[14]fe(16)3f[14]", 63 },  {"[16]e0", 0},  "[48]" },
        {  67,  {"[14]fe(16)3f[14]", 63 },  {"00[16]e0", 8},  "[48]" },
        {  67,  {"[14]fe(16)3f[14]", 63 },  {"[16]e0", 0, 2},  "[48]" },

        //{  51,  "fc0000000000007f", 6,
        //        "07ffffffffffff00", 5,
        //        "#" },
        //{ 115,  "fe.0000000000003f", 7,
        //        "03#ffffffffffff80", 6,
        //        "##" },
    };
    for (auto&& t : tests) {
        test(t);
        test(t, kInverted);
    }
}


/****************************************************************************
*
*   Tests - mismatch
*
***/

//===========================================================================
static void mistestStep(
    BitSpan & a,
    size_t apos,
    BitSpan & b,
    size_t bpos,
    size_t cnt,
    size_t pos,
    const source_location & sloc = source_location::current()
) {
    b.set(bpos + pos);
    auto adat = a.data();
    auto bdat = b.data();
    auto pre = BitSpan::mismatch(adat, apos, cnt, bdat, bpos, cnt);
    auto expect = pos >= cnt
        ? cnt   // No mark within field, expect length of entire field.
        : pos;  // Mark within field, expect distance from start to mark.
    if (pre != expect) {
        logMsgError() << "Line " << __LINE__ << " mismatch ( "
            << "{ " << apos << ", " << cnt << " }, "
            << "{ " << bpos << ", " << cnt << " } diff @" << pos
            << " ): output " << pre << ", expected " << expect;
    }
    b.reset(bpos + pos);
}

//===========================================================================
[[maybe_unused]] static void mistest(
    BitSpan & a,
    size_t apos,
    BitSpan & b,
    size_t bpos,
    size_t cnt,
    size_t pos,
    const source_location & sloc = source_location::current()
) {
    a.set();
    a.reset(apos, cnt);
    mistestStep(a, apos, b, bpos, cnt, pos, sloc);
}

//===========================================================================
static void mismatchTests() {
    uint64_t da[4];
    uint64_t db[4];
    BitSpan a(da, size(da));
    BitSpan b(db, size(db));
    b.reset();

    //                  cnt  pos
    mistest(a, 4, b, 2,   3,   3);
    mistest(a, 4, b, 2,   3,   1);
    mistest(a, 2, b, 4,   0,   0);
    mistest(a, 1, b, 1, 128, 127);
    mistest(a, 1, b, 1,  64,  63);
    mistest(a, 1, b, 1,   8,   7);
    mistest(a, 1, b, 0, 128, 127);
    mistest(a, 0, b, 9, 120, 119);
    mistest(a, 0, b, 9,  64,   0);
    mistest(a, 0, b, 8,  65,   0);
    mistest(a, 0, b, 8,   1,   0);
    mistest(a, 0, b, 4,   3,   3);
    mistest(a, 0, b, 4,   3,   2);
    mistest(a, 0, b, 4,   1,   1);
    mistest(a, 0, b, 4,   1,   0);
    mistest(a, 0, b, 1,  65,  64);
    mistest(a, 0, b, 1,  65,   0);
    mistest(a, 0, b, 1,  64,   1);

    if (s_opts.full) {
        for (size_t oa = 0; oa < 64; ++oa) {
            for (size_t ob = 0; ob < 64; ++ob) {
                for (size_t cnt = 0; cnt < 3 * 64; ++cnt) {
                    a.set();
                    a.reset(oa, cnt);
                    for (size_t pos = 0; pos <= cnt; ++pos) {
                        mistestStep(a, oa, b, ob, cnt, pos);
                        if (!checkErrorLimit())
                            return;
                    }
                }
            }
        }
    }
}


/****************************************************************************
*
*   Tests - rmismatch
*
***/

//===========================================================================
static void rmistestStep(
    BitSpan & a,
    size_t apos,
    BitSpan & b,
    size_t bpos,
    size_t cnt,
    size_t pos,
    const source_location & sloc = source_location::current()
) {
    b.set(bpos + pos);
    auto adat = a.data();
    auto bdat = b.data();
    auto pre = BitSpan::rmismatch(adat, apos, cnt, bdat, bpos, cnt);
    auto expect = pos >= cnt
        ? cnt            // No mark in field, expect length of entire field.
        : cnt - pos - 1; // Mark in field, expect distance after mark to end.
    if (pre != expect) {
        logMsgError() << "Line " << sloc.line() << " rmismatch ( "
            << "{ " << apos << ", " << cnt << " }, "
            << "{ " << bpos << ", " << cnt << " } diff @" << pos
            << " ): output " << pre << ", expected " << expect;
    }
    b.reset(bpos + pos);
}

//===========================================================================
static void rmistest(
    BitSpan & a,
    size_t apos,
    BitSpan & b,
    size_t bpos,
    size_t cnt,
    size_t pos,
    const source_location & sloc = source_location::current()
) {
    a.set();
    a.reset(apos, cnt);
    rmistestStep(a, apos, b, bpos, cnt, pos, sloc);
}

//===========================================================================
static void rmismatchTests() {
    uint64_t da[4];
    uint64_t db[4];
    BitSpan a(da, size(da));
    BitSpan b(db, size(db));
    b.reset();

    //                   cnt  pos
    rmistest(a, 0, b, 0,   1,   0);
    rmistest(a, 0, b, 1, 127,  63);
    rmistest(a, 2, b, 4,   0,   0);

    if (s_opts.full) {
        for (size_t oa = 0; oa < 64; ++oa) {
            for (size_t ob = 0; ob < 64; ++ob) {
                for (size_t cnt = 0; cnt < 3 * 64; ++cnt) {
                    a.set();
                    a.reset(oa, cnt);
                    for (size_t pos = 0; pos <= cnt; ++pos) {
                        rmistestStep(a, oa, b, ob, cnt, pos);
                        if (!checkErrorLimit())
                            return;
                    }
                }
            }
        }
    }
}


/****************************************************************************
*
*   Tests - setBit
*
***/

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


/****************************************************************************
*
*   Tests - find
*
***/

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
*   CmdOpts
*
***/

//===========================================================================
CmdOpts::CmdOpts() {
    Cli cli;
    cli.opt(&full, "f full").desc("Perform exhaustive tests.");
}


/****************************************************************************
*
*   Application
*
***/

//===========================================================================
static void app(Cli & cli) {
    for (auto&& fn : {
        setBitTests,
        findTests,
        copyTests,
        mismatchTests,
        rmismatchTests,
    }) {
        fn();
        if (!checkErrorLimit())
            break;
    }
    testSignalShutdown();
    if (s_opts.full)
        logStopwatch();
}


/****************************************************************************
*
*   External
*
***/

//===========================================================================
int main(int argc, char * argv[]) {
    Cli().action(app);
    return appRun(argc, argv, fAppTest);
}
