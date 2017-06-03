// Copyright Glen Knowles 2015 - 2017.
// Distributed under the Boost Software License, Version 1.0.
//
// winthread.cpp - dim windows platform
#include "pch.h"
#pragma hdrstop

using namespace std;
using namespace Dim;


/****************************************************************************
*
*   Private declarations
*
***/

namespace {

using SetThreadDescriptionFn = HRESULT(WINAPI *)(HANDLE hThread, PCWSTR desc);

class SetName {
    SetThreadDescriptionFn m_setDesc;
public:
    SetName();
    void set(string_view name);
};

} // namespace


/****************************************************************************
*
*   SetName
*
***/

//===========================================================================
SetName::SetName() {
    loadProc(m_setDesc, "kernelbase", "SetThreadDescription", true);
}

//===========================================================================
void SetName::set(string_view name) {
    if (m_setDesc)
        m_setDesc(GetCurrentThread(), toWstring(name).c_str());
}


/****************************************************************************
*
*   Thread
*
***/

//===========================================================================
void Dim::iThreadSetName(string_view name) {
    static SetName s_setter;
    s_setter.set(name);
}
