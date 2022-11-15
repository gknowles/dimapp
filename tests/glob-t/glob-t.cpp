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
static vector<pair<int, int>> diff(
    const vector<string_view> & from,
    const vector<string_view> & to
) {
    vector<pair<int, int>> out;
    auto a = 0;
    auto b = 0;
    while (a < from.size() && b < to.size()) {
        if (from[a] == to[b]) {
            out.push_back({a, b});
            a += 1;
            b += 1;
        } else {
            out.push_back({a, -1});
            out.push_back({-1, b});
            a += 1;
            b += 1;
        }
    }
    while (a < from.size()) {
        out.push_back({a, -1});
        a += 1;
    }
    while (b < to.size()) {
        out.push_back({-1, b});
        b += 1;
    }
    return out;
}

//===========================================================================
static void astTests() {
    vector<tuple<string, string>> tests = {
        { "abc", "abc" },

        { "*", "*" },
        { "a*", "a*" },
        { "*a", "*a" },
        { "a*b", "a*b" },
        { "**", "**" },
        { "***", "*" },
        { "a**", "a*" },
        { "?", "?" },
        { "\\?", "\\?" },

        { "[a]", "a" },
        { "[a]b", "ab" },
        { "a[b]", "ab" },
        { "a[bc]d", "a[bc]d" },
        { "[a][b][c]", "abc" },
        { "[xm-na-f1-3]", "[1-3a-fmnx]" },
        { "[x-xn-m3-1]", "[1-3mnx]" },
        { "[a\\-b\\]]", "[\\-\\]ab]" },
        { "[a-pm-z]", "[a-z]" },
        { "[-b]", "[\\-b]" },
        { "[b-]", "[\\-b]" },
        { "[^a-z]", "[^a-z]" },
        { "[", "\\[" },
        { "abc[xyz", "abc\\[xyz" },

        { "a/", "a/" },
        { "/x", "/x" },
        { "a/x", "a/x" },
        { "a/x/", "a/x/" },
        { "/a/x", "/a/x" },
        { "/a/x/", "/a/x/" },
        { "a/b/c", "a/b/c" },
        { "?/[ab]/*", "?/[ab]/*" },

        { "{a}", "a" },
        { "{a", "\\{a" },
        { "a}", "a}" },
        { "a{b}", "ab" },
        { "{a}b", "ab" },
        { "a{b}c", "abc" },
        { "a{b,c}d", "abd\nacd" },
        { "{a,b,c}", "a\nb\nc" },
        { "1{{a,b},2{x,y}}", "1a\n12x\n1b\n12y" },

        { "[{a}b]", "[ab]" },
        { "[{a]", "[a\\{]" },
        { "[a{]", "[a\\{]" },
        { "[a{b]c,cd]e}x", "[ab]cx\n[acd]ex" },

        { "a{b,/b/}c", "abc\na/b/c" },
    };
    for (auto && [in, expected] : tests) {
        Glob::Info info;
        string found;
        if (Glob::parse(&info, in))
            found = toString(info);
        if (found != expected) {
            logMsgError() << "Glob parse didn't match for: " << in;
            auto os = logMsgInfo();
            vector<string_view> foundLines, expectedLines;
            split(&foundLines, found);
            split(&expectedLines, expected);
            auto diffs = diff(expectedLines, foundLines);
            for (auto && [a, b] : diffs) {
                if (b == -1) {
                    os << "- " << expectedLines[a] << '\n';
                } else if (a == -1) {
                    os << "+ " << foundLines[b] << '\n';
                } else {
                    os << "  " << expectedLines[a] << '\n';
                }
            }
        }
    }
}

//===========================================================================
static void fileTests() {
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
        Glob::fDirsFirst | Glob::fDirsLast
    )) {
        found.push_back({e.path, e.isdir});
    }
    vector<pair<Path, bool>> expected = {
        { Path{"file-t/a.txt"},     false   },
        { Path{"file-t/b"},         true    },
        { Path{"file-t/b/ba.txt"},  false   },
        { Path{"file-t/b"},         true    },
        { Path{"file-t/b.txt"},     false   },
        { Path{"file-t/c"},         true    },
        { Path{"file-t/c"},         true    },
        { Path{"file-t/c.txt"},     false   },
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
}

//===========================================================================
static void app(int argc, char *argv[]) {
    astTests();
    fileTests();
    testSignalShutdown();
}


/****************************************************************************
*
*   main
*
***/

//===========================================================================
int main(int argc, char * argv[]) {
    // _CrtSetBreakAlloc(2109);

    int code = appRun(app, argc, argv);
    return code;
}
