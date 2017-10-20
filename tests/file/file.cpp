// Copyright Glen Knowles 2017.
// Distributed under the Boost Software License, Version 1.0.
//
// file.cpp - dimapp test file
#include "pch.h"
#pragma hdrstop

using namespace std;
using namespace Dim;


/****************************************************************************
*
*   Application
*
***/

//===========================================================================
static void app(int argc, char *argv[]) {
    string fn = "filetest.tmp";

    size_t psize = filePageSize();
    auto file = fileOpen(
        fn,
        File::fCreat | File::fTrunc | File::fReadWrite | File::fBlocking
    );
    fileWriteWait(file, 0, "aaaa", 4);
    const char * base;
    if (!fileOpenView(base, file, 1001 * psize))
        return appSignalShutdown(EX_DATAERR);
    fileExtendView(file, 1001 * psize);
    unsigned num = 0;
    static char v; // cppcheck-suppress variableScope
    for (size_t i = 1; i < 1000; ++i) {
        v = 0;
        fileWriteWait(file, i * psize, "bbbb", 4);
        v = base[i * psize];
        if (v == 'b')
            num += 1;
    }
    fileExtendView(file, psize);
    static char buf[5] = {};
    for (unsigned i = 0; i < 100; ++i) {
        fileReadWait(buf, 4, file, psize);
    }
    fileClose(file);

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
*   main
*
***/

//===========================================================================
int main(int argc, char * argv[]) {
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
    _set_error_mode(_OUT_TO_MSGBOX);

    int code = appRun(app, argc, argv);
    return code;
}
