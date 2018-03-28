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
*   Helpers
*
***/

//===========================================================================
static void createEmptyFile(string_view path) {
    if (auto f = fileOpen(path, File::fCreat | File::fReadWrite))
        fileClose(f);
}

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
    if (!fileOpenView(base, file, File::kViewReadOnly, 0, 0, 1001 * psize))
        return appSignalShutdown(EX_DATAERR);

    fileExtendView(file, base, 1001 * psize);
    unsigned num = 0;
    static char v = 0;
    for (size_t i = 1; i < 1000; ++i) {
        v = 0;
        fileWriteWait(file, i * psize, "bbbb", 4);
        v = base[i * psize];
        if (v == 'b')
            num += 1;
    }

    fileExtendView(file, base, psize);
    static char buf[5] = {};
    for (unsigned i = 0; i < 100; ++i) {
        fileReadWait(buf, 4, file, psize);
    }

    fileCloseView(file, base);
    fileClose(file);

    fileRemove("filetest", true);
    fileCreateDirs("filetest");
    createEmptyFile("filetest/a.txt");
    fileCreateDirs("filetest/b");
    createEmptyFile("filetest/b/ba.txt");
    createEmptyFile("filetest/b.txt");
    fileCreateDirs("filetest/c");
    createEmptyFile("filetest/c.txt");
    vector<pair<Path, bool>> found;
    for (auto && e : FileIter(
        "filetest",
        "a.txt",
        FileIter::fDirsFirst | FileIter::fDirsLast
    )) {
        found.push_back({e.path, e.isdir});
    }
    vector<pair<Path, bool>> expected = {
        { Path{"filetest/a.txt"}, false },
        { Path{"filetest/b"}, true },
        { Path{"filetest/b/ba.txt"}, false },
        { Path{"filetest/b"}, true },
        { Path{"filetest/b.txt"}, false },
        { Path{"filetest/c"}, true },
        { Path{"filetest/c"}, true },
        { Path{"filetest/c.txt"}, false },
    };
    if (found != expected) {
        logMsgError() << "File search didn't match";
        for (auto i = 0; i < found.size() || i < expected.size(); ++i) {
            auto os = logMsgInfo();
            if (i < expected.size()) {
                os.width(30);
                os << expected[i].first;
                os << (expected[i].second ? '*' : ' ');
            } else {
                os.width(31);
                os << ' ';
            }
            if (i < found.size()) {
                os.width(30);
                os << found[i].first;
                os << (found[i].second ? '*' : ' ');
            }
        }
    }

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
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF
        | _CRTDBG_LEAK_CHECK_DF
        | _CRTDBG_DELAY_FREE_MEM_DF);
    _set_error_mode(_OUT_TO_MSGBOX);

    int code = appRun(app, argc, argv);
    return code;
}
