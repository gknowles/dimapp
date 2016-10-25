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
    bld << start("value") << attr("attr") << "attr text" << endAttr
        << "text content" << end;
    bld.start("value")
        .startAttr("a")
        .text("atext")
        .endAttr()
        .attr("b", "btext")
        .end();
    bld.end();
    out.pushBack(0);
    cout << out.data();
    XDocument doc;
    auto root = doc.parse(out.data());
    (void)root;

    return EX_OK;
}
