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
    return uint8_t(
        ((x * 0x8020'0802ull) & 0x8'8442'2110ull) * 0x01'0101'0101ull >> 32
    );
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
// Taken from "With separated LS1B" as shown at:
// http://chessprogramming.wikispaces.com/BitScan
constexpr int trailingZeroBits(uint64_t val) {
    const int kTable[] = {
         0, 47,  1, 56, 48, 27,  2, 60,
        57, 49, 41, 37, 28, 16,  3, 61,
        54, 58, 35, 52, 50, 42, 21, 44,
        38, 32, 29, 23, 17, 11,  4, 62,
        46, 55, 26, 59, 40, 36, 15, 53,
        34, 51, 20, 43, 31, 22, 10, 45,
        25, 39, 14, 33, 19, 30,  9, 24,
        13, 18,  8, 12,  7,  6,  5, 63,
    };
    assert(val != 0);
    return kTable[((val ^ (val - 1)) * 0x3f79d71b4cb0a89) >> 58];
}

//===========================================================================
// Taken from "MS1B separation" as shown at:
// http://chessprogramming.wikispaces.com/BitScan
constexpr int leadingZeroBits(uint64_t val) {
    const int kTable[] = {
         0, 47,  1, 56, 48, 27,  2, 60,
        57, 49, 41, 37, 28, 16,  3, 61,
        54, 58, 35, 52, 50, 42, 21, 44,
        38, 32, 29, 23, 17, 11,  4, 62,
        46, 55, 26, 59, 40, 36, 15, 53,
        34, 51, 20, 43, 31, 22, 10, 45,
        25, 39, 14, 33, 19, 30,  9, 24,
        13, 18,  8, 12,  7,  6,  5, 63,
    };
    assert(val != 0);
    val |= val >> 1;
    val |= val >> 2;
    val |= val >> 4;
    val |= val >> 8;
    val |= val >> 16;
    val |= val >> 32;
    return kTable[(val * 0x3f79d71b4cb0a89) >> 58];
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
    const int kDeBruijnBitPosAdjustedForLog10[] = {
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
    int r = kDeBruijnBitPosAdjustedForLog10[
        uint32_t(v * UINT64_C(0x07c4'acdd)) >> 27
    ];

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

} // namespace
