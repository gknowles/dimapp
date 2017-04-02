// Copyright Glen Knowles 2017.
// Distributed under the Boost Software License, Version 1.0.
//
// winenv.cpp - dim windows platform
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
string Dim::envGetExecPath() {
    string out;
    DWORD num = 0;
    while (num == out.size()) {
        out.resize(out.size() + MAX_PATH);
        num = GetModuleFileName(NULL, out.data(), (DWORD) out.size());
    }
    out.resize(num);
    return out;
}
