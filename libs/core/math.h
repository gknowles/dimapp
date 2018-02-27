// Copyright Glen Knowles 2016 - 2017.
// Distributed under the Boost Software License, Version 1.0.
//
// math.h - dim core
#pragma once

#include "cppconf/cppconf.h"

#include <cassert>
#include <cstdint>
#include <limits>

namespace Dim {


/****************************************************************************
*
*   Math
*
***/

//===========================================================================
constexpr uint8_t reverseBits(uint8_t x) {
    x = ((x & 0xaa) >> 1) | ((x & 0x55) << 1);
    x = ((x & 0xcc) >> 2) | ((x & 0x33) << 2);
    return (x >> 4) | (x << 4);
}

//===========================================================================
constexpr uint64_t reverseBits(uint64_t x) {
    x = ((x & 0xaaaa'aaaa'aaaa'aaaa) >> 1)|((x & 0x5555'5555'5555'5555) << 1);
    x = ((x & 0xcccc'cccc'cccc'cccc) >> 2)|((x & 0x3333'3333'3333'3333) << 2);
    x = ((x & 0xf0f0'f0f0'f0f0'f0f0) >> 4)|((x & 0x0f0f'0f0f'0f0f'0f0f) << 4);
    x = ((x & 0xff00'ff00'ff00'ff00) >> 8)|((x & 0x00ff'00ff'00ff'00ff) << 8);
    x = ((x & 0xffff'0000'ffff'0000) >>16)|((x & 0x0000'ffff'0000'ffff) <<16);
    return (x >> 32) | (x << 32);
}

//===========================================================================
constexpr int trailingZeroBits(uint64_t val) {
    assert(val != 0);
#if 0
    unsigned long count = 0;
    _BitScanForward64(&count, val);
    return count;
#else
    int i = 0;
    for (;;) {
        if (val & 1)
            break;
        if (val & 2)
            return i + 1;
        val >>= 2;
        i += 2;
    }
    return i;
#endif
}

//===========================================================================
constexpr int leadingZeroBits(uint64_t val) {
    assert(val != 0);
#if 0
    return _CountLeadingZeros64(val);
#else
    auto mask = (uint64_t) 1 << 63;
    int i = 0;
    for (;;) {
        if (val & mask)
            break;
         val <<= 1;
         i += 1;
    }
    return i;
#endif
}

//===========================================================================
// Round up to power of 2
constexpr uint64_t pow2Ceil(uint64_t num) {
#if 0
    unsigned long k;
    _BitScanReverse64(&k, num);
    return (size_t) 1 << (k + 1);
#else
    num -= 1;
    num |= (num >> 1);
    num |= (num >> 2);
    num |= (num >> 4);
    num |= (num >> 8);
    num |= (num >> 16);
    num |= (num >> 32);
    num += 1;
    return num;
#endif
}

//===========================================================================
// Number of bits in the number that are set
constexpr int hammingWeight(uint64_t x) {
#if 0
    return _CountOneBits64(x);
#else
    x = x - ((x >> 1) & 0x5555'5555'5555'5555);
    x = (x & 0x3333'3333'3333'3333) + ((x >> 2) & 0x3333'3333'3333'3333);
    x = (x + (x >> 4)) & 0x0f0f'0f0f'0f0f'0f0f;
    x += x >> 8;
    x += x >> 16;
    x += x >> 32;
    return x & 0x7f;
#endif
}

//===========================================================================
// Number of digits required to display a number in decimal
constexpr int digits10(uint32_t val) {
    const uint8_t kTable[] = {
        0, 3, 0, 3, 4, 6, 0, 9, 3, 4, 5, 5, 6, 7, 1, 9,
        2, 3, 6, 8, 4, 5, 7, 2, 6, 8, 7, 2, 8, 1, 1, 9,
    };

    // round up to one less than a power of 2
    uint32_t v = val;
    v |= v >> 1;
    v |= v >> 2;
    v |= v >> 4;
    v |= v >> 8;
    v |= v >> 16;
    int r = kTable[uint32_t(v * UINT64_C(0x07c4'acdd)) >> 27];

    const uint32_t kPowersOf10[] = {
                    1,
                   10,
                  100,
                1'000,
               10'000,
              100'000,
            1'000'000,
           10'000'000,
          100'000'000,
        1'000'000'000,
    };
    r += val >= kPowersOf10[r];
    return r;
}


/****************************************************************************
*
*   Byte swap
*
***/

//===========================================================================
constexpr uint16_t bswap16(unsigned uval) {
    auto val = (uint16_t) uval;
    return ((val & 0xff00) >> 8)
        | (val << 8);
}

//===========================================================================
constexpr uint32_t bswap32(unsigned uval) {
    auto val = (uint32_t) uval;
    return ((val & 0xff000000) >> 24)
        | ((val & 0x00ff0000) >> 8)
        | ((val & 0x0000ff00) << 8)
        | (val << 24);
}

//===========================================================================
constexpr uint64_t bswap64(uint64_t val) {
    return ((val & 0xff00'0000'0000'0000) >> 56)
        | ((val & 0x00ff'0000'0000'0000) >> 40)
        | ((val & 0x0000'ff00'0000'0000) >> 24)
        | ((val & 0x0000'00ff'0000'0000) >> 8)
        | ((val & 0x0000'0000'ff00'0000) << 8)
        | ((val & 0x0000'0000'00ff'0000) << 24)
        | ((val & 0x0000'0000'0000'ff00) << 40)
        | (val << 56);
}

} // namespace
