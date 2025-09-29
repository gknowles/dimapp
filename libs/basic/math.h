// Copyright Glen Knowles 2016 - 2024.
// Distributed under the Boost Software License, Version 1.0.
//
// math.h - dim basic
#pragma once

#include "cppconf/cppconf.h"

#include <bit>
#include <cassert>
#include <concepts>
#include <cstdint>
#include <intrin.h>
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
constexpr uint16_t reverseBits(uint16_t x) {
    x = ((x & 0xaaaa) >> 1)|((x & 0x5555) << 1);
    x = ((x & 0xcccc) >> 2)|((x & 0x3333) << 2);
    x = ((x & 0xf0f0) >> 4)|((x & 0x0f0f) << 4);
    return (x >> 8) | (x << 8);
}

//===========================================================================
constexpr uint32_t reverseBits(uint32_t x) {
    x = ((x & 0xaaaa'aaaa) >> 1)|((x & 0x5555'5555) << 1);
    x = ((x & 0xcccc'cccc) >> 2)|((x & 0x3333'3333) << 2);
    x = ((x & 0xf0f0'f0f0) >> 4)|((x & 0x0f0f'0f0f) << 4);
    x = ((x & 0xff00'ff00) >> 8)|((x & 0x00ff'00ff) << 8);
    return (x >> 16) | (x << 16);
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


namespace Detail {

//===========================================================================
constexpr int floorDigits10(uint32_t val) {
    constexpr uint8_t kTable[] = {
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
    return kTable[uint32_t(v * UINT64_C(0x07c4'acdd)) >> 27];
}

//===========================================================================
constexpr int roundupDigits10(int digits, uint32_t val) {
    constexpr uint32_t kPowersOf10[] = {
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
    return digits + (val >= kPowersOf10[digits]);
}

} // namespace

//===========================================================================
// Number of digits required to display a number in decimal
constexpr int digits10(uint32_t val) {
    return Detail::roundupDigits10(Detail::floorDigits10(val), val);

    // Alternate implementation:
    //  return floor(log10(val)) + 1;
}

//===========================================================================
// Number of digits required to display a number in hexadecimal
constexpr int digits16(uint64_t val) {
    return (std::bit_width(val) + 3) / 4 + 1;

    // Alternate implementation:
    //  return floor(log16(val)) + 1;
}

} // namespace
