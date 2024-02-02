// Copyright Glen Knowles 2017 - 2024.
// Distributed under the Boost Software License, Version 1.0.
//
// math-t.cpp - dim test core
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

    auto num = digits10(1'000'000'000);
    EXPECT(num == 10);
    num = digits10(999'999'999);
    EXPECT(num == 9);

    char buf[100];
    double dval = 1.0f;
    htonf64(buf, dval);
    auto dout = ntohf64(buf);
    EXPECT(dval == dout);
    hton16(buf, 32000);
    auto o16 = ntoh16(buf);
    EXPECT(o16 == 32000);
    hton16(buf, 33000);
    o16 = ntoh16(buf);
    EXPECT(o16 == 33000);
    hton16(buf, (uint16_t) -2);
    o16 = ntoh16(buf);
    EXPECT(o16 == 65534);
    hton32(buf, 123456789);
    auto o32 = ntoh32(buf);
    EXPECT(o32 == 123456789);

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
