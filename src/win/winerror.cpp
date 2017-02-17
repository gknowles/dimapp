// winerror.cpp - dim core - windows platform
#include "pch.h"
#pragma hdrstop

using namespace std;
using namespace Dim;


/****************************************************************************
*
*   Private declarations
*
***/

using RtlNtStatusToDosErrorFn = ULONG(WINAPI *)(DWORD ntStatus);


/****************************************************************************
*
*   Variables
*
***/

static RtlNtStatusToDosErrorFn s_RtlNtStatusToDosError;


/****************************************************************************
*
*   Helpers
*
***/


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
    : m_value{error} {}

//===========================================================================
WinError::WinError(NtStatus status) {
    operator=(status);
}

//===========================================================================
WinError & WinError::operator=(int error) {
    m_value = error;
    return *this;
}

//===========================================================================
WinError & WinError::operator=(NtStatus status) {
    if (!status) {
        m_value = 0;
    } else {
        m_value = s_RtlNtStatusToDosError(status);
    }
    return *this;
}

//===========================================================================
std::ostream & Dim::operator<<(std::ostream & os, const WinError & val) {
    char buf[256];
    FormatMessage(
        FORMAT_MESSAGE_FROM_SYSTEM,
        NULL, // source
        val,
        0, // language
        buf,
        (DWORD)size(buf),
        NULL);

    // trim trailing whitespace (i.e. \r\n)
    char * ptr = buf + strlen(buf) - 1;
    for (; ptr > buf && isspace((unsigned char)*ptr); --ptr)
        *ptr = 0;

    os << buf;
    return os;
}


/****************************************************************************
*
*   Internal API
*
***/

//===========================================================================
void Dim::winErrorInitialize() {
    loadProc(s_RtlNtStatusToDosError, "ntdll", "RtlNtStatusToDosError");
}
