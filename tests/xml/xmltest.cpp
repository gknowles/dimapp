// tlstest.cpp - dim test xml
#include "pch.h"
#pragma hdrstop

using namespace std;
using namespace Dim;


/****************************************************************************
*
*   External
*
***/

int main(int argc, char * argv[]) {
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
    _set_error_mode(_OUT_TO_MSGBOX);

    CharBuf out;
    XBuilder bld(out);
    bld.start("root");
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
