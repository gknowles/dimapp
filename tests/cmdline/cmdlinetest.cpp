// cmdlinetest.cpp - dim test cmdline
#include "pch.h"
#pragma hdrstop

using namespace Dim;
using namespace std;

//===========================================================================
bool parseTest(CmdParser & cmd, vector<char *> args) {
    args.insert(args.begin(), "test.exe");
    return cmd.parse(cerr, size(args), data(args));
}

//===========================================================================
int main(int argc, char * argv[]) {
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
    _set_error_mode(_OUT_TO_MSGBOX);

    bool result;
    CmdParser cmd;
    auto & num = cmd.addArg<int>("n number", 1);
    auto & special = cmd.addArg<bool>("s special", false);
    auto & name = cmd.addArgVec<string>("name");
    cmd.addArgVec<string>("[key]");
    result = parseTest(cmd, {"-n3"});
    result = parseTest(cmd, {"--name", "two"});
    result = parseTest(cmd, {"--name=three"});
    result = parseTest(cmd, {"-s-name=four", "key", "--name", "four"});
    result = parseTest(cmd, {"key", "extra"});
    *num += 2;
    *special = name->empty();

    cmd = {};
    int count;
    bool help;
    cmd.addArg(&count, "c count");
    cmd.addArg(&help, "? h help");
    result = parseTest(cmd, {"-hc2", "-?"});
    if (result) {
        cout << "Last test passed" << endl;
        return EX_OK;
    }
    return EX_SOFTWARE;
}
