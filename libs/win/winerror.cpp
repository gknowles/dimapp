// Copyright Glen Knowles 2015 - 2020.
// Distributed under the Boost Software License, Version 1.0.
//
// winerror.cpp - dim windows platform
#include "pch.h"
#pragma hdrstop

using namespace std;
using namespace Dim;


/****************************************************************************
*
*   Private declarations
*
***/

using NtStatusToDosErrorFn = ULONG(WINAPI *)(DWORD ntStatus);
using SecurityStatusToNtStatusFn = ULONG(WINAPI *)(DWORD securityStatus);


/****************************************************************************
*
*   Variables
*
***/

static NtStatusToDosErrorFn s_NtStatusToDosError;
static SecurityStatusToNtStatusFn s_SecurityStatusToNtStatus;


/****************************************************************************
*
*   Helpers
*
***/

//===========================================================================
static int getNtStatus(WinError::SecurityStatus status) {
    switch ((int) status) {
    case SEC_I_CONTINUE_NEEDED:
        return 0xC0000016L; // STATUS_MORE_PROCESSING_REQUIRED;
    default:
        return s_SecurityStatusToNtStatus(status);
    }
}

//===========================================================================
static int getDosError(WinError::NtStatus status) {
    auto err = s_NtStatusToDosError(status);
    if (err == ERROR_MR_MID_NOT_FOUND) {
        logMsgDebug() << "Conversion of unknown NTSTATUS: "
            << hex << (int) status;
    }
    return err;
}


/****************************************************************************
*
*   WinError
*
***/

//===========================================================================
WinError::WinError() {
    m_value = GetLastError();
}

//===========================================================================
WinError::WinError(int error)
    : m_value{error}
{}

//===========================================================================
WinError::WinError(NtStatus status) {
    m_secStatus = 0;
    if (!status) {
        m_ntStatus = 0;
        m_value = 0;
    } else {
        m_ntStatus = status;
        m_value = getDosError((NtStatus) status);
    }
}

//===========================================================================
WinError::WinError(SecurityStatus status) {
    if (!status) {
        m_secStatus = 0;
        m_ntStatus = 0;
        m_value = 0;
    } else {
        m_secStatus = status;
        m_ntStatus = getNtStatus(status);
        m_value = getDosError((NtStatus) m_ntStatus);
    }
}

//===========================================================================
WinError::WinError(HResult result) {
    m_secStatus = 0;
    if (SUCCEEDED(result)) {
        m_ntStatus = 0;
        m_value = 0;
    } else if (result & FACILITY_NT_BIT) {
        m_ntStatus = HRESULT_CODE(result);
        m_value = getDosError((NtStatus) m_ntStatus);
    } else if (HRESULT_FACILITY(result) == FACILITY_WIN32) {
        m_ntStatus = 0;
        m_value = HRESULT_CODE(result);
    } else {
        logMsgDebug() << "Unknown HRESULT: " << hex << result;
        m_ntStatus = 0;
        m_value = ERROR_MR_MID_NOT_FOUND;
    }
}

//===========================================================================
WinError & WinError::set() {
    m_secStatus = 0;
    m_ntStatus = 0;
    m_value = GetLastError();
    return *this;
}

//===========================================================================
namespace Dim {
ostream & operator<<(ostream & os, const WinError & val);
}
ostream & Dim::operator<<(ostream & os, const WinError & val) {
    auto ntval = (WinError::NtStatus) val;
    auto secval = (WinError::SecurityStatus) val;
    int native = secval ? secval : ntval ? ntval : val;
    wchar_t wbuf[256] = {};
    FormatMessageW(
        FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL, // source
        native,
        0, // language
        wbuf,
        (DWORD) size(wbuf),
        NULL
    );

    // trim trailing whitespace (i.e. \r\n)
    auto ptr = wbuf + wcslen(wbuf) - 1;
    for (; ptr > wbuf && iswspace(*ptr); --ptr)
        *ptr = 0;

    os << utf8(wbuf) << " (" << (int) val;
    if (ntval || secval) {
        os << hex << ':' << ntval << ':' << secval << dec;
    }
    os << ')';
    return os;
}


/****************************************************************************
*
*   Internal API
*
***/

//===========================================================================
void Dim::winErrorInitialize() {
    winLoadProc(&s_NtStatusToDosError, "ntdll", "RtlNtStatusToDosError");
    winLoadProc(
        &s_SecurityStatusToNtStatus,
        "ntdll",
        "RtlMapSecurityErrorToNtStatus"
    );
}
