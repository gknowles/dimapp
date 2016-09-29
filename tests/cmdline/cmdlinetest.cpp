// cmdlinetest.cpp - dim test cmdline
#include "pch.h"
#pragma hdrstop

using namespace Dim::CmdLine;
using namespace std;

int main(int argc, char * argv[]) {
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
    _set_error_mode(_OUT_TO_MSGBOX);

    bool result;
    Option<int> num{"n number", 1};
    Option<bool> special{"s special", false};
    Option<string> name{"name"};
    Argument<string> key{"key"};
    char * t1[] = { "test.exe", "-n3" };
    result = parseOptions(size(t1), t1);
    char * t2[] = { "test.exe", "--name", "two" };
    result = parseOptions(size(t2), t2);
    char * t3[] = { "test.exe", "--name=three" };
    result = parseOptions(size(t3), t3);
    char * t4[] = { "test.exe", "-s-name=four" };
    result = parseOptions(size(t4), t4);
    *num += 2;
    *special = name->empty();
    return 0;
}
