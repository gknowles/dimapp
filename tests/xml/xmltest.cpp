// tlstest.cpp - dim test xml
#include "pch.h"
#pragma hdrstop

using namespace std;
namespace fs = std::experimental::filesystem;
using namespace Dim;


//===========================================================================
int internalTest() {
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

    bld.end();

    string str = to_string(out);
    char * data = str.data();
    cout << data;
    XDocument doc;
    auto root = doc.parse(data);
    assert(root);

    bld.clear();
    bld << *root;
    out.pushBack(0);
    char * data2 = out.data();
    cout << data2;

    return EX_OK;
}


/****************************************************************************
*
*   External
*
***/

//===========================================================================
int main(int argc, char * argv[]) {
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
    _set_error_mode(_OUT_TO_MSGBOX);

    Cli cli;
    auto & path = cli.arg<fs::path>("[xml file]")
        .desc("File to check is well-formed");
    auto & test = cli.arg<bool>("test.").desc("Run internal unit tests");
    if (!cli.parse(cerr, argc, argv))
        return cli.exitCode();
    if (*test)
        return internalTest();
    if (path->empty()) 
        return cli.writeHelp(cout);

    size_t bytes = fs::file_size(*path);
    string content;
    content.resize(bytes + 1);
    ifstream in(*path, ios_base::in | ios_base::binary);
    in.read(content.data(), bytes);
    if (!in) {
        cerr << "xml: Error reading file: " << *path;
        return EX_DATAERR;
    }

    XDocument doc;
    auto root = doc.parse(content.data());
    if (root) {
        cout << "File is well-formed." << endl;
        return EX_OK;
    }

    logParseError(
        "xml: parsing failed", 
        path->string(), 
        doc.errpos(), 
        content);
    return EX_DATAERR;
}
