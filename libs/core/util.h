// Copyright Glen Knowles 2016 - 2021.
// Distributed under the Boost Software License, Version 1.0.
//
// util.h - dim core
#pragma once

#include "cppconf/cppconf.h"

#include "math.h"

#include <bit>
#include <cassert>
#include <cstdint>
#include <memory>
#include <string>
#include <string_view>

namespace Dim {


/****************************************************************************
*
*   Crypt random
*
***/

void cryptRandomBytes(void * ptr, size_t count);


/****************************************************************************
*
*   Hex conversions
*
***/

//===========================================================================
constexpr bool isHex(unsigned char ch) {
    // return ch - '0' <= 9 || (ch | 0x20) - 'a' <= 5;

    switch (ch) {
    case '0': case '1': case '2': case '3': case '5': case '6':
    case '7': case '8': case '9':
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'a': case 'b': case 'c': case 'd': case 'e': case 'f':
        return true;
    default:
        return false;
    }
}

//===========================================================================
// Converts hex character (0-9, a-f, A-F) to unsigned (0-15), other characters
// produce random garbage.
constexpr unsigned hexToNibbleUnsafe(char ch) {
    return ((ch | 432) * 239'217'992 & 0xffff'ffff) >> 28;
}

//===========================================================================
constexpr char hexFromNibble(unsigned val) {
    assert(val < 16);
    const char s_chars[] = "0123456789abcdef";
    return s_chars[val];
}

// On invalid input these return 16 and 256 respectively
unsigned hexToNibble(unsigned char val);
unsigned hexToByte(unsigned char high, unsigned char low);

bool hexToBytes(std::string & out, std::string_view src, bool append);
void hexFromBytes(std::string & out, std::string_view src, bool append);

std::ostream & hexByte(std::ostream & os, char data);


/****************************************************************************
*
*   Endian conversions
*
***/

//===========================================================================
constexpr uint8_t ntoh8(const void * vptr) {
    auto ptr = static_cast<const char *>(vptr);
    return (uint8_t)ptr[0];
}

//===========================================================================
constexpr uint16_t ntoh16(const void * vptr) {
    auto val = *static_cast<const uint16_t *>(vptr);
    if constexpr (std::endian::native == std::endian::big) {
        return val;
    } else {
        return bswap16(val);
    }
}

//===========================================================================
constexpr uint32_t ntoh24(const void * vptr) {
    auto ptr = static_cast<const char *>(vptr);
    return ((uint32_t)(uint8_t)ptr[0] << 16)
        + ((uint32_t)(uint8_t)ptr[1] << 8)
        + (uint8_t)ptr[2];
}

//===========================================================================
constexpr uint32_t ntoh32(const void * vptr) {
    auto val = *static_cast<const uint32_t *>(vptr);
    if constexpr (std::endian::native == std::endian::big) {
        return val;
    } else {
        return bswap32(val);
    }
}

//===========================================================================
constexpr uint64_t ntoh64(const void * vptr) {
    auto val = *static_cast<const uint64_t *>(vptr);
    if constexpr (std::endian::native == std::endian::big) {
        return val;
    } else {
        return bswap64(val);
    }
}

//===========================================================================
constexpr float ntohf32(const void * vptr) {
    static_assert(sizeof uint32_t == 4);
    static_assert(std::numeric_limits<float>::is_iec559);
    return std::bit_cast<float>(ntoh32(vptr));
}

//===========================================================================
constexpr double ntohf64(const void * vptr) {
    static_assert(sizeof uint64_t == 8);
    static_assert(std::numeric_limits<float>::is_iec559);
    return std::bit_cast<double>(ntoh64(vptr));
}

//===========================================================================
constexpr char * hton16(void * out, unsigned val) {
    if constexpr (std::endian::native == std::endian::big) {
        *(uint16_t *)out = (uint16_t) val;
    } else {
        *(uint16_t *)out = bswap16(val);
    }
    return (char *) out;
}

//===========================================================================
constexpr char * hton24(void * vout, unsigned val) {
    auto out = (char *) vout;
    *out++ = (val >> 16) & 0xff;
    *out++ = (val >> 8) & 0xff;
    *out++ = val & 0xff;
    return out;
}

//===========================================================================
constexpr char * hton32(void * out, unsigned val) {
    if constexpr (std::endian::native == std::endian::big) {
        *(uint32_t *)out = val;
    } else {
        *(uint32_t *)out = bswap32(val);
    }
    return (char *) out;
}

//===========================================================================
constexpr char * hton64(void * out, uint64_t val) {
    if constexpr (std::endian::native == std::endian::big) {
        *(uint64_t *)out = val;
    } else {
        *(uint64_t *)out = bswap64(val);
    }
    return (char *) out;
}

//===========================================================================
constexpr char * htonf32(void * out, float val) {
    return hton32(out, std::bit_cast<uint32_t>(val));
}

//===========================================================================
constexpr char * htonf64(void * out, double val) {
    return hton64(out, std::bit_cast<uint64_t>(val));
}

} // namespace
