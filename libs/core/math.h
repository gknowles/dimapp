// Copyright Glen Knowles 2016 - 2021.
// Distributed under the Boost Software License, Version 1.0.
//
// math.h - dim core
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
}


/****************************************************************************
*
*   Byte swap
*
***/

//===========================================================================
constexpr uint16_t bswap16(uint16_t uval) {
    auto val = (uint16_t) uval;
    return (val >> 8) | (val << 8);
}

//===========================================================================
constexpr uint32_t bswap32(uint32_t uval) {
    auto val = (uint32_t) uval;
    return (val >> 24)
        | ((val & 0x00ff'0000) >> 8)
        | ((val & 0x0000'ff00) << 8)
        | (val << 24);
}

//===========================================================================
constexpr uint64_t bswap64(uint64_t val) {
    if (std::is_constant_evaluated()) {
        return (val >> 56)
            | ((val & 0x00ff'0000'0000'0000) >> 40)
            | ((val & 0x0000'ff00'0000'0000) >> 24)
            | ((val & 0x0000'00ff'0000'0000) >> 8)
            | ((val & 0x0000'0000'ff00'0000) << 8)
            | ((val & 0x0000'0000'00ff'0000) << 24)
            | ((val & 0x0000'0000'0000'ff00) << 40)
            | (val << 56);
    } else {
        return _byteswap_uint64(val);
    }
}

//===========================================================================
constexpr auto bswap(std::unsigned_integral auto val) {
    if constexpr (sizeof val == 1) {
        return val;
    } else if constexpr (sizeof val == 2) {
        return bswap16(val);
    } else if constexpr (sizeof val == 4) {
        return bswap32(val);
    } else if constexpr (sizeof val == 8) {
        return bswap64(val);
    } else {
        static_assert(!"Must be 8, 16, 32, or 64 bit unsigned integer");
    }
}

} // namespace
