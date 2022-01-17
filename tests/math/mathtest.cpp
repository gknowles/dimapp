// Copyright Glen Knowles 2017 - 2022.
// Distributed under the Boost Software License, Version 1.0.
//
// mathtest.cpp - dim test core
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
    auto ptr = htonf64(buf, dval);
    auto dout = ntohf64(ptr);
    EXPECT(dval == dout);
    ptr = hton16(buf, 32000);
    auto o16 = ntoh16(ptr);
    EXPECT(o16 == 32000);
    o16 = ntoh16(hton16(buf, 33000));
    EXPECT(o16 == 33000);
    o16 = ntoh16(hton16(buf, (uint16_t) -2));
    EXPECT(o16 == 65534);

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
