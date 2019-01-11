// Copyright Glen Knowles 2018 - 2019.
// Distributed under the Boost Software License, Version 1.0.
//
// bitviewtest.cpp - dim test bitview
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
    string_view sv((char *) buf, sizeof(buf));
    EXPECT(v.size() == 3);
    EXPECT(v.count() == 0);
    v.set().set(24, 64, (uint64_t) 0x0123'4567'89ab'cdef);
    EXPECT(sv.substr(0, 16) ==
        "\xff\xff\xff\x01\x23\x45\x67\x89"
        "\xab\xcd\xef\xff\xff\xff\xff\xff"
    );
    v.set(16, 16, 0x1234);
    EXPECT(sv.substr(0, 8) == "\xff\xff\x12\x34\x23\x45\x67\x89");
    v.set(64, 64, 0);
    EXPECT(buf[1] == 0);
    v.set(24, 8, 0xef);
    EXPECT(sv.substr(0, 8) == "\xff\xff\x12\xef\x23\x45\x67\x89");
    v.reset().set(18, 1, 1);
    EXPECT(v[18] == 1);
    v.set(0, 64, 0x0123'4567'89ab'cdef);
    auto a = v.get(0, 64);
    EXPECT(a == 0x0123'4567'89ab'cdef);
    v.set(64, 64, a);
    v.set(128, 64, a);
    a = v.get(16, 64);
    EXPECT(a == 0x4567'89ab'cdef'0123);
    a = v.get(72, 8);
    EXPECT(a == 0x23);
    a = v.get(160, 48);
    EXPECT(a == 0x89ab'cdef'0000);
    a = v.get(144, 48);
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
