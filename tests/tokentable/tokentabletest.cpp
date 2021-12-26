// Copyright Glen Knowles 2017 - 2020.
// Distributed under the Boost Software License, Version 1.0.
//
// tokentabletest.cpp - dim test tokentable
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

    const TokenTable::Token numbers[] = {
        {1, "one"},
        {2, "two"},
        {3, "three"},
        {4, "four"},
        {5, "five"},
        {21, "twenty-one"},
        {22, "twenty-two"},
    };
    const TokenTable numberTbl(numbers, size(numbers));

    EXPECT(numberTbl.find("invalid", 0) == 0);
    for (auto && tok : numbers) {
        EXPECT(numberTbl.find(tok.name, 0) == tok.id);
    }

    auto num = 0;
    for (auto && tok : numberTbl) {
        if (tok.id)
            num += 1;
    }
    EXPECT((size_t) num == size(numbers));

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
