// Copyright Glen Knowles 2020.
// Distributed under the Boost Software License, Version 1.0.
//
// winguid.cpp - dim windows platform
#include "pch.h"
#pragma hdrstop

using namespace std;
using namespace Dim;


/****************************************************************************
*
*   Guid
*
***/

//===========================================================================
Guid Dim::newGuid() {
    Guid val;
    auto result = UuidCreate((UUID *) &val);
    if (result != RPC_S_OK)
        logMsgFatal() << "UuidCreate: " << WinError{};

    return val;
}

//===========================================================================
string Dim::toString(const Guid & val) {
    string out;
    char buf[4];
    hton32(buf, val.data1);
    hexFromBytes(out, string_view(buf, 4), true);
    out += '-';
    hton16(buf, val.data2);
    hexFromBytes(out, string_view(buf, 2), true);
    out += '-';
    hton16(buf, val.data3);
    hexFromBytes(out, string_view(buf, 2), true);
    out += '-';
    hexFromBytes(out, string_view((char *) val.data4, 2), true);
    out += '-';
    hexFromBytes(out, string_view((char *) val.data4 + 2, 6), true);
    return out;
}
