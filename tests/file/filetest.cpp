// Copyright Glen Knowles 2017 - 2018.
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

    auto file = fileOpen(
        fn,
        File::fCreat | File::fTrunc | File::fReadWrite | File::fBlocking
    );
    if (!file)
        return appSignalShutdown(EX_DATAERR);
    size_t psize = filePageSize(file);
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

    auto content = string(10, '#');
    if (!fileResize(file, size(content)))
        return appSignalShutdown(EX_DATAERR);
    fileWriteWait(file, 0, content.data(), content.size());
    struct Reader : IFileReadNotify {
        bool onFileRead(
            size_t * bytesUsed,
            std::string_view data,
            int64_t offset,
            FileHandle f
        ) override {
            m_out += data;
            *bytesUsed = data.size();
            return true;
        }
        void onFileEnd(int64_t offset, FileHandle f) override {
            m_done = true;
            m_cv.notify_all();
        }

        string m_out;
        mutex m_mut;
        condition_variable m_cv;
        bool m_done{false};
    } reader;
    fileRead(
        &reader,
        buf,
        size(buf) - 1,
        file,
        0,
        size(content),
        taskComputeQueue()
    );
    unique_lock lk{reader.m_mut};
    while (!reader.m_done)
        reader.m_cv.wait(lk);

    if (reader.m_out != content)
        logMsgError() << "File content mismatch";

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
