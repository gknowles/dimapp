// Copyright Glen Knowles 2022.
// Distributed under the Boost Software License, Version 1.0.
//
// test.cpp - dim tools
#include "pch.h"
#pragma hdrstop

using namespace std;
using namespace Dim;


/****************************************************************************
*
*   Test helper functions
*
***/

//===========================================================================
void Dim::testSignalShutdown() {
    if (int errs = logGetMsgCount(kLogTypeError)) {
        ConsoleScopedAttr attr(kConsoleError);
        cerr << "*** TEST FAILURES: " << errs
            << " (" << appBaseName() << ")" << endl;
        appSignalShutdown(EX_SOFTWARE);
    } else {
        cout << "All tests passed"
            << " (" << appBaseName() << ")" << endl;
        appSignalShutdown(EX_OK);
    }
}
