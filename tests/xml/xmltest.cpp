// Copyright Glen Knowles 2016 - 2017.
// Distributed under the Boost Software License, Version 1.0.
//
// xmltest.cpp - dim test xml
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
    XBuilder bld(out);
    bld.start("root");
    bld.text("  ROOT  ");

    bld.start("cdata") << "";
    out.append(" x <![CDATA[text]]> y ");
    bld.end();

    bld << start("value") << attr("attr") << "attr text\nwith '&'" << endAttr
        << "text content" << end;

    bld.start("value")
        .startAttr("a")
        .text("atext")
        .endAttr()
        .attr("b", "btext")
        .end();

    bld.start("non-cdata") << "text: ";
    bld << "> ]]>" << ']' << ']' << '>' << ']' << "]>" << ']' << '>' << '>'
        << end;

    bld.start("value").attr("a", " leading and trailing spaces ").end();

    bld.end();

    string str = toString(out);
    char * data = str.data();
    EXPECT(str == 1 + R"(
<root>  ROOT  <cdata> x <![CDATA[text]]> y </cdata>
<value attr="attr text
with '&amp;'">text content</value>
<value a="atext" b="btext"/>
<non-cdata>text: > ]]&gt;]]&gt;]]&gt;]>></non-cdata>
<value a=" leading and trailing spaces "/>
</root>
)");

    XDocument doc;
    auto root = doc.parse(data);
    assert(root);

    bld.clear();
    bld << *root;
    out.pushBack(0);
    char * data2 = out.data();
    str = string(data2);
    EXPECT(str == 1 + R"(
<root>ROOT<cdata>x text y</cdata>
<value attr="attr text with '&amp;'">text content</value>
<value a="atext" b="btext"/>
<non-cdata>text: > ]]&gt;]]&gt;]]&gt;]>></non-cdata>
<value a=" leading and trailing spaces "/>
</root>
)");
    unsigned num = 0;
    for (auto && node [[maybe_unused]] : nodes(root, XType::kElement)) {
        assert(node.name);
        num += 1;
    }
    EXPECT(num == 5);

    if (s_errors) {
        cerr << "*** TEST FAILURES: " << s_errors << endl;
        return EX_SOFTWARE;
    }
    cout << "All tests passed" << endl;
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
    auto & path =
        cli.opt<Path>("[xml file]").desc("File to check is well-formed");
    auto & test = cli.opt<bool>("test.").desc("Run internal unit tests");
    auto & echo = cli.opt<bool>("echo").desc("Echo xml if well-formed");
    cli.versionOpt("1.0 (" __DATE__ ")");
    if (!cli.parse(cerr, argc, argv))
        return cli.exitCode();
    if (*test)
        return internalTest();
    if (path->empty())
        return cli.printHelp(cout);

    path->defaultExt("xml");
    auto bytes = (size_t) fileSize(*path);
    string content;
    content.resize(bytes + 1);
    ifstream in(path->str(), ios_base::in | ios_base::binary);
    in.read(content.data(), bytes);
    if (!in) {
        logMsgError() << "xml: Error reading file: " << *path;
        return EX_DATAERR;
    }
    in.close();

    XDocument doc;
    auto root = doc.parse(content.data(), *path);
    if (root && !doc.errmsg()) {
        cout << "File '" << doc.filename() << "' is well-formed." << endl;
        if (*echo) {
            cout << endl;
            CharBuf out;
            XBuilder bld(out);
            bld << *root;
            out.pushBack(0);
            cout << out.data();
        }
        return EX_OK;
    }

    logParseError(
        "xml: parsing failed",
        *path,
        doc.errpos(),
        content
    );
    return EX_DATAERR;
}
