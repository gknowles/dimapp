// Copyright Glen Knowles 2015 - 2017.
// Distributed under the Boost Software License, Version 1.0.
//
// wintime.cpp - dim windows platform
#include "pch.h"
#pragma hdrstop

using namespace std;
using namespace Dim;


/****************************************************************************
*
*   Time
*
***/

//===========================================================================
int64_t Dim::iClockGetTicks() {
    LARGE_INTEGER out;
    FILETIME ft;
    GetSystemTimeAsFileTime(&ft);
    out.HighPart = ft.dwHighDateTime;
    out.LowPart = ft.dwLowDateTime;
    return out.QuadPart;
}
