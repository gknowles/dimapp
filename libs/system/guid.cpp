// Copyright Glen Knowles 2020 - 2025.
// Distributed under the Boost Software License, Version 1.0.
//
// guid.cpp - dim system
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
string Dim::toString(const Guid & val) {
    string out;
    char buf[4];
    hton32(buf, val.data1);
    hexAppendBytes(&out, string_view(buf, 4));
    out += '-';
    hton16(buf, val.data2);
    hexAppendBytes(&out, string_view(buf, 2));
    out += '-';
    hton16(buf, val.data3);
    hexAppendBytes(&out, string_view(buf, 2));
    out += '-';
    hexAppendBytes(&out, string_view((char *) val.data4, 2));
    out += '-';
    hexAppendBytes(&out, string_view((char *) val.data4 + 2, 6));
    return out;
}

//===========================================================================
bool Dim::parse(Guid * out, std::string_view val) {
    *out = strToGuid(val);
    return (bool) *out;
}
