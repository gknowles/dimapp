// Copyright Glen Knowles 2015 - 2017.
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
        m_value = s_NtStatusToDosError(status);
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
        m_ntStatus = s_SecurityStatusToNtStatus(status);
        m_value = s_NtStatusToDosError(m_ntStatus);
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
std::ostream & Dim::operator<<(std::ostream & os, const WinError & val) {
    auto ntval = (WinError::NtStatus) val;
    auto secval = (WinError::SecurityStatus) val;
    int native = secval ? secval : ntval ? ntval : val;
    char buf[256] = {};
    FormatMessage(
        FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL, // source
        native,
        0, // language
        buf,
        (DWORD)size(buf),
        NULL);

    // trim trailing whitespace (i.e. \r\n)
    char * ptr = buf + strlen(buf) - 1;
    for (; ptr > buf && isspace((unsigned char)*ptr); --ptr)
        *ptr = 0;

    os << buf << " (" << (int) val;
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
    loadProc(s_NtStatusToDosError, "ntdll", "RtlNtStatusToDosError");
    loadProc(
        s_SecurityStatusToNtStatus, 
        "ntoskrnl.exe", 
        "RtlMapSecurityErrorToNtStatus"
    );
}
