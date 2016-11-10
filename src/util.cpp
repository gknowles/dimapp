// util.cpp - dim services
#include "pch.h"
#pragma hdrstop

using namespace std;
using namespace Dim;


/****************************************************************************
*
*   Variables
*
***/

// randomly generated key
const uint8_t s_key[] = {
    0xed,
    0x2e,
    0x24,
    0x9b,
    0x49,
    0x5f,
    0x10,
    0xf8,
    0xb8,
    0xca,
    0x13,
    0x3e,
    0xb4,
    0x99,
    0x56,
    0x5d,
};
static_assert(size(s_key) == crypto_shorthash_KEYBYTES, "");


/****************************************************************************
*
*   Public API
*
***/

//===========================================================================
size_t Dim::hashStr(const char src[]) {
    int64_t hash;
    crypto_shorthash((uint8_t *)&hash, (uint8_t *)src, strlen(src), s_key);
    return hash;
}

//===========================================================================
size_t Dim::hashStr(const char src[], size_t maxlen) {
    size_t len = min(strlen(src), maxlen);
    int64_t hash;
    crypto_shorthash((uint8_t *)&hash, (uint8_t *)src, len, s_key);
    return hash;
}

//===========================================================================
// How byte order mark detection works:
//   https://en.wikipedia.org/wiki/Byte_order_mark#Representations_of_byte
//      _order_marks_by_encoding
UtfType Dim::utfBomType(const char bytes[], size_t count) {
    if (count >= 2) {
        switch (bytes[0]) {
        case 0:
            if (count >= 4 && bytes[1] == 0 && bytes[2] == '\xfe' 
                && bytes[3] == '\xff') {
                return kUtf32BE;
            }
            break;
        case '\xef':
            if (count >= 3 && bytes[1] == '\xbb' && bytes[2] == '\xbf')
                return kUtf8;
            break;
        case '\xfe':
            if (bytes[1] == '\xff')
                return kUtf16BE;
            break;
        case '\xff':
            if (bytes[1] == '\xfe') {
                if (count >= 4 && bytes[2] == 0 && bytes[3] == 0)
                    return kUtf32LE;
                return kUtf16LE;
            }
            break;
        }
    }
    return kUtfUnknown;
}
