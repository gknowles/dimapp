// Copyright Glen Knowles 2017 - 2020.
// Distributed under the Boost Software License, Version 1.0.
//
// winutil.cpp - dim windows platform
#include "pch.h"
#pragma hdrstop

using namespace std;
using namespace Dim;


/****************************************************************************
*
*   Public LoadProc API
*
***/

//===========================================================================
FARPROC Dim::winLoadProc(const char lib[], const char proc[], bool optional) {
    FARPROC fn = nullptr;
    auto wlib = toWstring(lib);
    HMODULE mod = LoadLibraryW(wlib.c_str());
    if (!mod) {
        if (!optional)
            logMsgFatal() << "LoadLibraryW(" << lib << "): " << WinError{};
        return fn;
    }

    fn = GetProcAddress(mod, proc);
    if (!fn) {
        if (!optional)
            logMsgFatal() << "GetProcAddress(" << proc << "): " << WinError{};
    }
    return fn;
}


/****************************************************************************
*
*   Public Privileges API
*
***/

//===========================================================================
bool Dim::winEnablePrivilege(const wchar_t wname[], bool enable) {
    auto proc = GetCurrentProcess();
    HANDLE token;
    if (!OpenProcessToken(proc, TOKEN_ADJUST_PRIVILEGES, &token))
        return false;

    bool success = false;
    TOKEN_PRIVILEGES tp;
    tp.PrivilegeCount = 1;
    tp.Privileges[0].Attributes = enable ? SE_PRIVILEGE_ENABLED : 0;
    if (LookupPrivilegeValueW(
        NULL,
        wname,
        &tp.Privileges[0].Luid
    )) {
        if (AdjustTokenPrivileges(
            token,
            false,  // disable all
            &tp,
            0,      // size of buffer to receive previous state
            NULL,   // pointer to buffer to previous state
            NULL    // size of previous state placed in buffer
        )) {
            WinError err;
            success = (err == ERROR_SUCCESS);
        }
    }
    CloseHandle(token);
    return success;
}


/****************************************************************************
*
*   Public Overlapped API
*
***/

//===========================================================================
void Dim::winSetOverlapped(
    OVERLAPPED & overlapped,
    int64_t off,
    HANDLE event
) {
    overlapped = {};
    overlapped.Offset = (DWORD)off;
    overlapped.OffsetHigh = (DWORD)(off >> 32);
    if (event != INVALID_HANDLE_VALUE)
        overlapped.hEvent = event;
}


/****************************************************************************
*
*   IWinOverlappedNotify
*
***/

//===========================================================================
IWinOverlappedNotify::IWinOverlappedNotify(TaskQueueHandle hq) {
    m_evt.notify = this;
    m_evt.hq = hq;
}

//===========================================================================
void IWinOverlappedNotify::pushOverlappedTask(void * completionKey) {
    m_evt.completionKey = completionKey;
    if (m_evt.hq) {
        taskPush(m_evt.hq, m_evt.notify);
    } else {
        taskPushEvent(m_evt.notify);
    }
}

//===========================================================================
IWinOverlappedNotify::WinOverlappedResult
IWinOverlappedNotify::decodeOverlappedResult() {
    DWORD bytes = 0;
    if (!GetOverlappedResult(
        INVALID_HANDLE_VALUE,
        &m_evt.overlapped,
        &bytes,
        false
    )) {
        return {{}, bytes};
    }
    return {0, bytes};
}


/****************************************************************************
*
*   WinSid
*
***/

//===========================================================================
WinSid & WinSid::operator=(const SID & sid) {
    CopySid(sizeof m_data, &m_data.Sid, (SID *) &sid);
    return *this;
}

//===========================================================================
WinSid::operator bool() const {
    return !IsValidSid((SID *) &m_data.Sid);
}

//===========================================================================
bool WinSid::operator==(const WinSid & right) const {
    return EqualSid((SID *) &**this, (SID *) &*right);
}

//===========================================================================
bool WinSid::operator==(WELL_KNOWN_SID_TYPE type) const {
    return IsWellKnownSid((SID *) &**this, type);
}

//===========================================================================
void WinSid::reset() {
    memset(&m_data, 0, sizeof m_data);
}

//===========================================================================
WinSid Dim::winCreateSid(string_view account) {
    auto wacct = toWstring(account);
    SID_NAME_USE use;
    WinSid out;
    DWORD count = sizeof out;
    wchar_t domain[256];
    auto domLen = (DWORD) size(domain);
    if (!LookupAccountNameW(
        nullptr,
        wacct.c_str(),
        &*out,
        &count,
        domain,
        &domLen,
        &use
    )) {
        out.reset();
    }
    return out;
}

//===========================================================================
WinSid Dim::winCreateSid(WELL_KNOWN_SID_TYPE type, const SID * domain) {
    WinSid out;
    DWORD cb = sizeof out;
    if (!CreateWellKnownSid(
        type,
        (SID *) domain,
        &*out,
        &cb
    )) {
        logMsgFatal() << "CreateWellKnownSid: " << WinError{};
        out.reset();
    }
    return out;
}

//===========================================================================
bool Dim::winGetAccountName(
    string * name,
    string * domain,
    const SID & sid
) {
    if (name)
        name->clear();
    if (domain)
        domain->clear();
    DWORD nameLen = 0;
    DWORD domLen = 0;
    SID_NAME_USE use;
    if (!LookupAccountSidW(
        NULL,
        (PSID) &sid,
        NULL, &nameLen,
        NULL, &domLen,
        &use
    )) {
        WinError err;
        if (err != ERROR_INSUFFICIENT_BUFFER)
            return false;
    }
    wstring wname(nameLen, 0);
    wstring wdom(domLen, 0);
    if (!LookupAccountSidW(
        NULL,
        (PSID) &sid,
        wname.data(), &nameLen,
        wdom.data(), &domLen,
        &use
    )) {
        WinError err;
        return false;
    }
    if (name)
        *name = toString(wname);
    if (domain)
        *domain = toString(wdom);
    return true;
}

//===========================================================================
string Dim::toString(const SID & sid) {
    wchar_t * str;
    if (!ConvertSidToStringSidW((SID *) &sid, &str)) {
        return "InvalidSid";
    } else {
        return toString(str);
    }
}

//===========================================================================
bool Dim::parse(WinSid * out, string_view src) {
    PSID tmp;
    auto wsrc = toWstring(src);
    if (!ConvertStringSidToSidW(wsrc.c_str(), &tmp)) {
        out->reset();
        return false;
    } else {
        *out = *(SID *)tmp;
        return true;
    }
}
