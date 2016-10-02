// cmdlinetest.cpp - dim test cmdline
#include "pch.h"
#pragma hdrstop

using namespace Dim;
using namespace std;

//===========================================================================
bool parseTest(vector<char *> args) {
    args.insert(args.begin(), "test.exe");
    return cmdParse(size(args), data(args));
}

//===========================================================================
int main(int argc, char * argv[]) {
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
    _set_error_mode(_OUT_TO_MSGBOX);

    bool result;
    CmdOpt<int> num{"n number", 1};
    CmdOpt<bool> special{"s special", false};
    CmdOptVec<string> name{"name"};
    CmdArgVec<string> key{"key"};
    result = parseTest({"-n3"});
    result = parseTest({"--name", "two"});
    result = parseTest({"--name=three"});
    result = parseTest({"-s-name=four", "key", "--name", "four"});
    result = parseTest({"key", "extra"});
    *num += 2;
    *special = name->empty();

    CmdParser parser;
    int count;
    bool help;
    parser.addOpt(&count, "c count");
    parser.addOpt(&help, "? h help");
    char * t1[] = {"test.exe", "-hc2", "-?"};
    result = parser.parse(cerr, size(t1), t1);
    return 0;
}
