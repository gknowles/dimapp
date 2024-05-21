// Copyright Glen Knowles 2016 - 2022.
// Distributed under the Boost Software License, Version 1.0.
//
// hex.cpp - dim core
#include "pch.h"
#pragma hdrstop

using namespace std;
using namespace Dim;


/****************************************************************************
*
*   Hex conversions
*
***/

static const unsigned char s_hexToNibble[256] = {
    16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
    16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
    16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
    0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  16, 16, 16, 16, 16, 16,
    16, 10, 11, 12, 13, 14, 15, 16, 16, 16, 16, 16, 16, 16, 16, 16,
    16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
    16, 10, 11, 12, 13, 14, 15, 16, 16, 16, 16, 16, 16, 16, 16, 16,
    16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
    16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
    16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
    16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
    16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
    16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
    16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
    16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
    16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
};

//===========================================================================
unsigned Dim::hexToNibble(unsigned char val) {
    return s_hexToNibble[val];
}

//===========================================================================
unsigned Dim::hexToByte(unsigned char high, unsigned char low) {
    int highbyte = hexToNibble(high);
    int lowbyte = hexToNibble(low);
    if (highbyte == 16 || lowbyte == 16)
        return 256;
    return 16 * highbyte + lowbyte;
}

//===========================================================================
bool Dim::hexToBytes(string & out, string_view src, bool append) {
    if (src.size() % 2)
        return false;
    if (!append)
        out.clear();

    auto pos = out.size();
    out.resize(pos + src.size() / 2);
    for (size_t i = 0; i < src.size(); i += 2) {
        auto val = hexToByte(src[i], src[i + 1]);
        if (val == 256)
            return false;
        out[pos++] = (char) val;
    }
    return true;
}

//===========================================================================
void Dim::hexFromBytes(string & out, string_view src, bool append) {
    if (!append)
        out.clear();

    auto pos = out.size();
    out.resize(pos + 2 * src.size());
    for (unsigned char ch : src) {
        out[pos++] = hexFromNibble(ch >> 4);
        out[pos++] = hexFromNibble(ch & 0x0f);
    }
}

//===========================================================================
ostream & Dim::hexByte(ostream & os, char data) {
    os << hexFromNibble((unsigned char) data >> 4)
        << hexFromNibble(data & 0x0f);
    return os;
}

//===========================================================================
ostream & Dim::hexDumpLine(ostream & os, string_view data, size_t pos) {
    assert(pos < data.size());
    auto ptr = (const unsigned char *) data.data() + pos;
    os << setw(6) << pos << ':';
    for (unsigned i = 0; i < 16; ++i) {
        if (i % 2 == 0) os.put(' ');
        if (pos + i < data.size()) {
            hexByte(os, ptr[i]);
        } else {
            os << "  ";
        }
    }
    os << "  ";
    for (unsigned i = 0; i < 16; ++i) {
        if (pos + i < data.size())
            os.put(isprint(ptr[i]) ? (char) ptr[i] : '.');
    }
    return os;
}

//===========================================================================
ostream & Dim::hexDump(ostream & os, string_view data) {
    for (unsigned pos = 0; pos < data.size(); pos += 16) {
        hexDumpLine(os, data, pos);
        os << '\n';
    }
    return os;
}
