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
        case 1: *ptr++ = val & 0xff; val >>= 8;
        }
    }
}


/****************************************************************************
*
*   Unicode
*
***/

//===========================================================================
// Wikipedia article on byte order marks:
//   https://en.wikipedia.org/wiki/Byte_order_mark#Representations_of_byte_order_marks_by_encoding
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
