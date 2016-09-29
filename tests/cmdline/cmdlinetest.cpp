// cmdlinetest.cpp - dim test cmdline
#include "pch.h"
#pragma hdrstop

using namespace Dim::CmdLine;
using namespace std;

//===========================================================================
bool parseTest(vector<char *> args) {
    args.insert(args.begin(), "test.exe");
    return parseOptions(size(args), data(args));
}

//===========================================================================
int main(int argc, char * argv[]) {
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
    _set_error_mode(_OUT_TO_MSGBOX);

    bool result;
    Option<int> num{"n number", 1};
    Option<bool> special{"s special", false};
    Option<string> name{"name"};
    ArgumentVector<string> key{"key"};
    result = parseTest({ "-n3" });
    result = parseTest({ "--name", "two" });
    result = parseTest({ "--name=three" });
    result = parseTest({ "-s-name=four", "key" });
    result = parseTest({ "key", "extra" });
    *num += 2;
    *special = name->empty();
    return 0;
}
