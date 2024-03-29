// Copyright Glen Knowles 2017 - 2023.
// Distributed under the Boost Software License, Version 1.0.
//
// path-t.cpp - dimapp test path
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
        logMsgError() << "Line " << source_location::current().line()       \
                      << ": EXPECT(" << #__VA_ARGS__ << ") failed";         \
    }


/****************************************************************************
*
*   Application
*
***/

//===========================================================================
static void app(int argc, char * argv[]) {
    Path p;

    struct {
        string_view in;
        string_view fname;
        string_view out;
        source_location sloc = source_location::current();
    } setFilenameTests[] = {
        { "/one/two", "2", "/one/2" },
        { "/one/two", "", "/one" },
        { "c:one", "1", "c:1" },
        { "c:/one", "1", "c:/1" },
        { "c:/one", "", "c:/" },
        { "c:/", "", "c:/" },
        { "c:/", "1", "c:/1" },
    };
    for (auto && t : setFilenameTests) {
        p.assign(t.in);
        auto out = p.setFilename(t.fname);
        if (out != t.out) {
            logMsgError() << "Line " << t.sloc.line() << ": Path(" << t.in
                << ").setFilename(" << t.fname << ") == '" << out
                << "', should be '" << t.out << "'";
        }
    }

    struct {
        string_view in;
        string_view out;
        source_location sloc = source_location::current();
    } parentTests[] = {
        { "/one/two/three", "/one/two" },
        { "c:one", "c:" },
        { "c:/one", "c:/" },
    };
    for (auto && t : parentTests) {
        p.assign(t.in);
        auto out = p.parentPath();
        if (out != t.out) {
            logMsgError() << "Line " << t.sloc.line() << ": Path(" << t.in
                << ").parentPath() == '" << out << "', should be '" << t.out
                << "'";
        }
    }

    struct {
        string_view in;
        string_view out;
        source_location sloc = source_location::current();
    } normalizeTests[] = {
        { "../../a", "../../a" },
        { "../a/..", ".." },
        { "/..", "/" },
        { "..", ".." },
        { "..\\a\\b\\..\\base.ext", "../a/base.ext" },
        { "one/two/", "one/two" },
        { "a/.", "a" },
        { "../", ".." },
    };
    for (auto && t : normalizeTests) {
        p.assign(t.in);
        if (p != t.out) {
            logMsgError() << "Line " << t.sloc.line() << ": Path(" << t.in
                << ") == '" << p << "', should be '" << t.out << "'";
        }
    }

    struct {
        string_view base;
        string_view rel;
        string_view out;
        source_location sloc = source_location::current();
    } resolveTests[] = {
        { "",        "rel",    "rel"         },
        { "base",    "rel",    "base/rel"    },
        { "base",    "/rel",   "/rel"        },
        { "base",    "c:rel",  "c:rel"       },
        { "base",    "c:/rel", "c:/rel"      },
        { "/base",   "rel",    "/base/rel"   },
        { "/base",   "/rel",   "/rel"        },
        { "/base",   "c:rel",  "c:rel"       },
        { "/base",   "c:/rel", "c:/rel"      },
        { "c:",      "rel",    "c:rel"       },
        { "c:base",  "rel",    "c:base/rel"  },
        { "c:base",  "/rel",   "c:/rel"      },
        { "c:base",  "c:rel",  "c:base/rel"  },
        { "c:base",  "c:/rel", "c:/rel"      },
        { "c:base",  "a:rel",  "a:rel"       },
        { "c:base",  "a:/rel", "a:/rel"      },
        { "c:/base", "rel",    "c:/base/rel" },
        { "c:/base", "/rel",   "c:/rel"      },
        { "c:/base", "c:rel",  "c:/base/rel" },
        { "c:/base", "c:/rel", "c:/rel"      },
        { "c:/base", "a:rel",  "a:rel"       },
        { "c:/base", "a:/rel", "a:/rel"      },
    };
    for (auto && t : resolveTests) {
        p.assign(t.rel);
        p.resolve(t.base);
        if (p != t.out) {
            logMsgError() << "Line " << t.sloc.line() << ": "
                << t.base << " / " << t.rel << " == '" << p
                << "', should be '" << t.out << "'";
        }
    }

    p = "hello";
    p.defaultExt("txt");
    EXPECT(p == "hello.txt"sv);

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
