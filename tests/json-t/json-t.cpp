// Copyright Glen Knowles 2018 - 2022.
// Distributed under the Boost Software License, Version 1.0.
//
// json-t.cpp - dim test json
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

struct CmdOpts {
    Path path;
    bool echo;

    CmdOpts();
};
static CmdOpts s_opts;

static int s_errors;


/****************************************************************************
*
*   Tests
*
***/

//===========================================================================
void internalTest() {
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
}


/****************************************************************************
*
*   CmdOpts
*
***/

//===========================================================================
CmdOpts::CmdOpts() {
    Cli cli;
    cli.versionOpt("1.0 (" __DATE__ ")");
    cli.opt(&path, "f file")
        .desc("File to check is well-formed");
    cli.opt(&echo, "echo")
        .desc("Echo json if well-formed (only if file given)");
}


/****************************************************************************
*
*   Application
*
***/

//===========================================================================
static void app(Cli & cli) {
    if (!s_opts.path) {
        internalTest();
        return testSignalShutdown();
    }

    // Parse file.
    s_opts.path.defaultExt("json");
    string content;
    if (auto ec = fileLoadBinaryWait(&content, s_opts.path); ec)
        return appSignalShutdown(EX_DATAERR);
    JDocument doc;
    auto root = doc.parse(content.data(), s_opts.path);
    if (!root || doc.errmsg()) {
        logParseError(
            "json: Parsing failed",
            s_opts.path,
            doc.errpos(),
            content
        );
        return appSignalShutdown(EX_DATAERR);
    }
    cout << "File '" << doc.filename() << "' is well-formed." << endl;

    // Echo file (if requested).
    if (s_opts.echo) {
        cout << endl;
        CharBuf out;
        JBuilder bld(&out);
        bld << *root;
        out.pushBack(0);
        cout << out.data();
    }

    appSignalShutdown();
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

    Cli().action(app);
    return appRun(argc, argv, fAppTest);
}
