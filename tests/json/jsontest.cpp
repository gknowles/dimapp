// Copyright Glen Knowles 2018 - 2022.
// Distributed under the Boost Software License, Version 1.0.
//
// jsontest.cpp - dim test json
#include "pch.h"
#pragma hdrstop

using namespace std;
using namespace Dim;


/****************************************************************************
*
*   Declarations
*
***/

#define EXPECT(...) \
    if (!bool(__VA_ARGS__)) { \
        cerr << "Line " << (line ? line : __LINE__) << ": EXPECT(" \
            << #__VA_ARGS__ << ") failed\n"; \
        s_errors += 1; \
    }


/****************************************************************************
*
*   Variables
*
***/

static int s_errors;


/****************************************************************************
*
*   Helpers
*
***/

//===========================================================================
int internalTest() {
    int line = 0;

    CharBuf out;
    JBuilder bld(&out);
    bld.object();
    bld.member("a");
    bld.array() << 1 << 2 << end;
    bld.member("b", 3);
    bld.end();

    const char kJsonText[] = "{\"a\":[1,\n2\n],\n\"b\":3\n}\n";

    string str = toString(out);
    char * data = str.data();
    EXPECT(str == kJsonText);

    JDocument doc;
    auto root = doc.parse(data);
    assert(root);

    bld.clear();
    bld << *root;
    auto str2 = toString(out);
    EXPECT(str2 == kJsonText);
    unsigned num = 0;
    vector<string_view> names;
    for ([[maybe_unused]] auto && node : nodes(root)) {
        names.push_back(nodeName(&node));
        num += 1;
    }
    EXPECT(num == 2);

    bld.clear();
    bld.array().startValue() << 'a' << ' ' << 'b' << end;
    bld.startValue() << 1 << '+' << 1 << '=' << 2 << end;
    bld.end();
    const char kTest2[] = "[\"a b\",\n\"1+1=2\"\n]\n";
    str2 = toString(out);
    EXPECT(str2 == kTest2);

    if (s_errors) {
        cerr << "*** TEST FAILURES: " << s_errors << " (json)" << endl;
        return EX_SOFTWARE;
    }
    cout << "All tests passed (json)" << endl;
    return EX_OK;
}


/****************************************************************************
*
*   External
*
***/

//===========================================================================
int main(int argc, char * argv[]) {
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF
        | _CRTDBG_LEAK_CHECK_DF
        | _CRTDBG_DELAY_FREE_MEM_DF);
    _set_error_mode(_OUT_TO_MSGBOX);

    Cli cli;
    auto & path = cli.opt<Path>("[json file]")
        .desc("File to check is well-formed");
    auto & test = cli.opt<bool>("test.")
        .desc("Run internal unit tests");
    auto & echo = cli.opt<bool>("echo")
        .desc("Echo json if well-formed");
    cli.versionOpt("1.0 (" __DATE__ ")");
    if (!cli.parse(cerr, argc, argv))
        return cli.exitCode();
    if (*test)
        return internalTest();
    if (path->empty())
        return cli.printHelp(cout);

    path->defaultExt("json");
    auto bytes = (size_t) fileSize(*path);
    string content;
    content.resize(bytes + 1);
    ifstream in(path->str(), ios_base::in | ios_base::binary);
    in.read(content.data(), bytes);
    if (!in) {
        logMsgError() << "json: Error reading file: " << *path;
        return EX_DATAERR;
    }
    in.close();

    JDocument doc;
    auto root = doc.parse(content.data(), *path);
    if (root && !doc.errmsg()) {
        cout << "File '" << doc.filename() << "' is well-formed." << endl;
        if (*echo) {
            cout << endl;
            CharBuf out;
            JBuilder bld(&out);
            bld << *root;
            out.pushBack(0);
            cout << out.data();
        }
        return EX_OK;
    }

    logParseError(
        "json: Parsing failed",
        *path,
        doc.errpos(),
        content
    );
    return EX_DATAERR;
}
