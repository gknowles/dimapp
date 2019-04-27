// Copyright Glen Knowles 2016 - 2018.
// Distributed under the Boost Software License, Version 1.0.
//
// util.cpp - dim core
#include "pch.h"
#pragma hdrstop

using namespace std;
using namespace Dim;


/****************************************************************************
*
*   Variables
*
***/

static random_device s_rdev;


/****************************************************************************
*
*   Crypt random
*
***/

//===========================================================================
void Dim::cryptRandomBytes(void * vptr, size_t count) {
    auto * ptr = static_cast<char *>(vptr);
    for (; count >= sizeof(unsigned); count -= sizeof(unsigned)) {
        unsigned val = s_rdev();
        memcpy(ptr, &val, sizeof(val));
        ptr += sizeof(unsigned);
    }
    if (count) {
        unsigned val = s_rdev();
        switch (count) {
        case 3: *ptr++ = val & 0xff; val >>= 8;
        case 2: *ptr++ = val & 0xff; val >>= 8;
        case 1: *ptr++ = val & 0xff;
        }
    }
}


/****************************************************************************
*
*   Hex conversions
*
***/

static unsigned char const s_hexToNibble[256] = {
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
    for (int ch : src) {
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
