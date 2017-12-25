// Copyright Glen Knowles 2016 - 2017.
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
bool Dim::hexToBytes(string & out, string_view src, bool append) {
    if (src.size() % 2)
        return false;
    if (!append)
        out.clear();

    auto pos = out.size();
    out.resize(pos + src.size() / 2);
    for (size_t i = 0; i < src.size(); i += 2) {
        int high = s_hexToNibble[src[i]];
        int low = s_hexToNibble[src[i + 1]];
        if ((high | low) & 0x10)
            return false;
        out[pos++] = uint8_t((high << 4) + low);
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
