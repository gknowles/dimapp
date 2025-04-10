// Copyright Glen Knowles 2015 - 2025.
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
using GetThreadDescriptionFn = HRESULT(WINAPI *)(HANDLE hThread, PWSTR * desc);

class ThreadName {
    SetThreadDescriptionFn m_setDesc;
    GetThreadDescriptionFn m_getDesc;
public:
    static ThreadName & instance();
public:
    void set(string_view name);
    string get() const;
private:
    ThreadName();
};

} // namespace


/****************************************************************************
*
*   ThreadName
*
***/

//===========================================================================
// static
ThreadName & ThreadName::instance() {
    static ThreadName s_name;
    return s_name;
}

//===========================================================================
ThreadName::ThreadName() {
    winLoadProc(&m_setDesc, "kernelbase", "SetThreadDescription", true);
    winLoadProc(&m_getDesc, "kernelbase", "GetThreadDescription", true);
}

//===========================================================================
void ThreadName::set(string_view name) {
    if (m_setDesc) {
        auto result = m_setDesc(GetCurrentThread(), toWstring(name).c_str());
        if (FAILED(result))
            logMsgFatal() << "SetThreadDescription: " << WinError{result};
    }
}

//===========================================================================
string ThreadName::get() const {
    string out;
    if (m_getDesc) {
        wchar_t * name;
        auto result = m_getDesc(GetCurrentThread(), &name);
        if (FAILED(result))
            logMsgFatal() << "GetThreadDescription: " << WinError{result};
        out = toString(wstring_view(name));
        LocalFree(name);
    }
    return out;
}


/****************************************************************************
*
*   Thread
*
***/

//===========================================================================
void Dim::iThreadInitialize() {
    winCrashInitializeThread();
}

//===========================================================================
void Dim::iThreadSetName(string_view name) {
    auto & s_name = ThreadName::instance();
    s_name.set(name);
}

//===========================================================================
string Dim::iThreadGetName() {
    auto & s_name = ThreadName::instance();
    return s_name.get();
}
