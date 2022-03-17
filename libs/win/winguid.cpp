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
Guid Dim::strToGuid(std::string_view val) {
    Guid out;
    if (val.size() != 36
        || val[8] != '-'
        || val[12] != '-'
        || val[16] != '-'
        || val[20] != '-'
    ) {
        return {};
    }

    string buf;
    if (!hexToBytes(buf, val.substr(0, 8), false)
        || !hexToBytes(buf, val.substr(9, 4), true)
        || !hexToBytes(buf, val.substr(14, 4), true)
        || !hexToBytes(buf, val.substr(19, 4), true)
        || !hexToBytes(buf, val.substr(24, 12), true)
    ) {
        return {};
    }

    out.data1 = ntoh32(buf.data());
    out.data2 = ntoh16(buf.data() + 4);
    out.data3 = ntoh16(buf.data() + 6);
    memcpy(out.data4, buf.data() + 8, sizeof out.data4);
    return out;
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
