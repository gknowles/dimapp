// Copyright Glen Knowles 2018.
// Distributed under the Boost Software License, Version 1.0.
//
// winres.cpp - dim windows platform
#include "pch.h"
#pragma hdrstop

using namespace std;
using namespace Dim;


/****************************************************************************
*
*   Public API
*
***/

//===========================================================================
int Dim::execElevated(string_view prog, string_view args) {
    SHELLEXECUTEINFOW ei = { sizeof(ei) };
    ei.lpVerb = L"RunAs";
    auto wexe = toWstring(prog);
    ei.lpFile = wexe.c_str();
    auto wargs = toWstring(args);
    ei.lpParameters = wargs.c_str();
    ei.fMask = SEE_MASK_NOASYNC
        | SEE_MASK_NOCLOSEPROCESS
        | SEE_MASK_FLAG_NO_UI
        ;
    ei.nShow = SW_HIDE;
    if (!ShellExecuteExW(&ei)) {
        WinError err;
        if (err == ERROR_CANCELLED) {
            logMsgError() << "Operation canceled.";
        } else {
            logMsgError() << "ShellExecuteExW: " << WinError();
        }
        return -1;
    }

    DWORD rc = EX_OSERR;
    if (WAIT_OBJECT_0 == WaitForSingleObject(ei.hProcess, INFINITE))
        GetExitCodeProcess(ei.hProcess, &rc);
    return rc;
}
