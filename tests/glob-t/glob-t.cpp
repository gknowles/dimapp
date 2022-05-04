// Copyright Glen Knowles 2022.
// Distributed under the Boost Software License, Version 1.0.
//
// glob-t.cpp - dimapp test glob-t
#include "pch.h"
#pragma hdrstop

using namespace std;
using namespace Dim;
using fm = Dim::File::OpenMode;


/****************************************************************************
*
*   Helpers
*
***/

//===========================================================================
static void createEmptyFile(string_view path) {
    FileHandle file;
    if (auto ec = fileOpen(&file, path, fm::fCreat | fm::fReadWrite); !ec)
        fileClose(file);
}

/****************************************************************************
*
*   Application
*
***/

//===========================================================================
static void app(int argc, char *argv[]) {
    fileRemove("file-t", true);
    fileCreateDirs("file-t");
    createEmptyFile("file-t/a.txt");
    fileCreateDirs("file-t/b");
    createEmptyFile("file-t/b/ba.txt");
    createEmptyFile("file-t/b.txt");
    fileCreateDirs("file-t/c");
    createEmptyFile("file-t/c.txt");
    vector<pair<Path, bool>> found;
    for (auto && e : fileGlob(
        "file-t",
        "a.txt",
        GlobMode::fDirsFirst | GlobMode::fDirsLast
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
