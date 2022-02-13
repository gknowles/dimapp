// Copyright Glen Knowles 2017 - 2022.
// Distributed under the Boost Software License, Version 1.0.
//
// file-t.cpp - dimapp test file
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
    FileHandle file;
    if (auto ec = fileOpen(&file, path, File::fCreat | File::fReadWrite); !ec)
        fileClose(file);
}

/****************************************************************************
*
*   Application
*
***/

//===========================================================================
static void app(int argc, char *argv[]) {
    string fn = "file-t.tmp";

    FileHandle file;
    auto ec = fileOpen(
        &file,
        fn,
        File::fCreat | File::fTrunc | File::fReadWrite | File::fBlocking
    );
    if (ec)
        return appSignalShutdown(EX_DATAERR);
    size_t psize = filePageSize(file);
    fileWriteWait(nullptr, file, 0, "aaaa", 4);

    const char * base;
    ec = fileOpenView(base, file, File::View::kReadOnly, 0, 0, 1001 * psize);
    if (ec)
        return appSignalShutdown(EX_DATAERR);

    ec = fileExtendView(file, base, 1001 * psize);
    if (ec)
        return appSignalShutdown(EX_DATAERR);

    unsigned num = 0;
    static char v = 0;
    for (size_t i = 1; i < 1000; ++i) {
        v = 0;
        fileWriteWait(nullptr, file, i * psize, "bbbb", 4);
        v = base[i * psize];
        if (v == 'b')
            num += 1;
    }

    ec = fileExtendView(file, base, psize);
    static char buf[5] = {};
    for (unsigned i = 0; i < 100; ++i) {
        fileReadWait(nullptr, buf, 4, file, psize);
    }
    ec = fileCloseView(file, base);

    auto content = string(10, '#');
    if (auto ec = fileResize(file, size(content)); ec)
        return appSignalShutdown(EX_DATAERR);
    fileWriteWait(nullptr, file, 0, content.data(), content.size());
    struct Reader : IFileReadNotify {
        bool onFileRead(
            size_t * bytesUsed,
            std::string_view data,
            bool more,
            int64_t offset,
            FileHandle f,
            error_code ec
        ) override {
            m_out += data;
            *bytesUsed = data.size();
            if (!more) {
                m_done = true;
                m_cv.notify_all();
            }
            return true;
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

    fileRemove("file-t", true);
    fileCreateDirs("file-t");
    createEmptyFile("file-t/a.txt");
    fileCreateDirs("file-t/b");
    createEmptyFile("file-t/b/ba.txt");
    createEmptyFile("file-t/b.txt");
    fileCreateDirs("file-t/c");
    createEmptyFile("file-t/c.txt");
    vector<pair<Path, bool>> found;
    for (auto && e : FileIter(
        "file-t",
        "a.txt",
        FileIter::fDirsFirst | FileIter::fDirsLast
    )) {
        found.push_back({e.path, e.isdir});
    }
    vector<pair<Path, bool>> expected = {
        { Path{"file-t/a.txt"}, false },
        { Path{"file-t/b"}, true },
        { Path{"file-t/b/ba.txt"}, false },
        { Path{"file-t/b"}, true },
        { Path{"file-t/b.txt"}, false },
        { Path{"file-t/c"}, true },
        { Path{"file-t/c"}, true },
        { Path{"file-t/c.txt"}, false },
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

    testSignalShutdown();
}


/****************************************************************************
*
*   main
*
***/

//===========================================================================
int main(int argc, char * argv[]) {
    int code = appRun(app, argc, argv);
    return code;
}
