// Copyright Glen Knowles 2017 - 2022.
// Distributed under the Boost Software License, Version 1.0.
//
// path.cpp - dimapp test path
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
static void app(int argc, char * argv[]) {
    int line = 0;
    Path p;

    struct {
        string_view in;
        string_view fname;
        string_view out;
        int line;
    } setFilenameTests[] = {
        { "/one/two", "2", "/one/2", __LINE__ },
        { "/one/two", "", "/one", __LINE__ },
        { "c:one", "1", "c:1", __LINE__ },
        { "c:/one", "1", "c:/1", __LINE__ },
        { "c:/one", "", "c:/", __LINE__ },
        { "c:/", "", "c:/", __LINE__ },
        { "c:/", "1", "c:/1", __LINE__ },
    };
    for (auto && t : setFilenameTests) {
        p.assign(t.in);
        auto out = p.setFilename(t.fname);
        if (out != t.out) {
            logMsgError() << "Line " << t.line << ": Path(" << t.in
                << ").setFilename(" << t.fname << ") == '" << out
                << "', should be '" << t.out << "'";
        }
    }

    struct {
        string_view in;
        string_view out;
        int line;
    } parentTests[] = {
        { "/one/two/three", "/one/two", __LINE__ },
        { "c:one", "c:", __LINE__ },
        { "c:/one", "c:/", __LINE__ },
    };
    for (auto && t : parentTests) {
        p.assign(t.in);
        auto out = p.parentPath();
        if (out != t.out) {
            logMsgError() << "Line " << t.line << ": Path(" << t.in
                << ").parentPath() == '" << out << "', should be '" << t.out
                << "'";
        }
    }

    struct {
        string_view in;
        string_view out;
        int line;
    } normalizeTests[] = {
        { "../../a", "../../a", __LINE__ },
        { "../a/..", "..", __LINE__ },
        { "/..", "/", __LINE__ },
        { "..", "..", __LINE__ },
        { "..\\a\\b\\..\\base.ext", "../a/base.ext", __LINE__ },
        { "one/two/", "one/two", __LINE__ },
        { "a/.", "a", __LINE__ },
        { "../", "..", __LINE__ },
    };
    for (auto && t : normalizeTests) {
        p.assign(t.in);
        if (p != t.out) {
            logMsgError() << "Line " << t.line << ": Path(" << t.in
                << ") == '" << p << "', should be '" << t.out << "'";
        }
    }

    struct {
        string_view base;
        string_view rel;
        string_view out;
        int line;
    } resolveTests[] = {
        { "",        "rel",    "rel",           __LINE__ },
        { "base",    "rel",    "base/rel",      __LINE__ },
        { "base",    "/rel",   "/rel",          __LINE__ },
        { "base",    "c:rel",  "c:rel",         __LINE__ },
        { "base",    "c:/rel", "c:/rel",        __LINE__ },
        { "/base",   "rel",    "/base/rel",     __LINE__ },
        { "/base",   "/rel",   "/rel",          __LINE__ },
        { "/base",   "c:rel",  "c:rel",         __LINE__ },
        { "/base",   "c:/rel", "c:/rel",        __LINE__ },
        { "c:",      "rel",    "c:rel",         __LINE__ },
        { "c:base",  "rel",    "c:base/rel",    __LINE__ },
        { "c:base",  "/rel",   "c:/rel",        __LINE__ },
        { "c:base",  "c:rel",  "c:base/rel",    __LINE__ },
        { "c:base",  "c:/rel", "c:/rel",        __LINE__ },
        { "c:base",  "a:rel",  "a:rel",         __LINE__ },
        { "c:base",  "a:/rel", "a:/rel",        __LINE__ },
        { "c:/base", "rel",    "c:/base/rel",   __LINE__ },
        { "c:/base", "/rel",   "c:/rel",        __LINE__ },
        { "c:/base", "c:rel",  "c:/base/rel",   __LINE__ },
        { "c:/base", "c:/rel", "c:/rel",        __LINE__ },
        { "c:/base", "a:rel",  "a:rel",         __LINE__ },
        { "c:/base", "a:/rel", "a:/rel",        __LINE__ },
    };
    for (auto && t : resolveTests) {
        p.assign(t.rel);
        p.resolve(t.base);
        if (p != t.out) {
            logMsgError() << "Line " << t.line << ": "
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
