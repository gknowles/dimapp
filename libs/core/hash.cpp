// Copyright Glen Knowles 2016 - 2023.
// Distributed under the Boost Software License, Version 1.0.
//
// hash.cpp - dim core
//
// Implements SipHash-2-4, as described by:
//   SipHash: a fast short-input PRF (https://131002.net/siphash/siphash.pdf)

#include "pch.h"
#pragma hdrstop

using namespace std;
using namespace Dim;


/****************************************************************************
*
*   Variables
*
***/

const unsigned kLittleEndian = 0x1234;
const unsigned kBigEndian = 0x4321;

const unsigned kByteOrder = kLittleEndian;
const bool kNativeUnaligned64 = true;


/****************************************************************************
*
*   Seed
*
***/

namespace {
struct Seed {
    // SipHash default seed
    uint64_t v0 = 0x736f'6d65'7073'6575 ^ 0x0706'0504'0302'0100;
    uint64_t v1 = 0x646f'7261'6e64'6f6d ^ 0x0f0e'0d0c'0b0a'0908;
    uint64_t v2 = 0x6c79'6765'6e65'7261 ^ 0x0706'0504'0302'0100;
    uint64_t v3 = 0x7465'6462'7974'6573 ^ 0x0f0e'0d0c'0b0a'0908;

    Seed();
};
} // namespace

//===========================================================================
Seed::Seed () {
    // set to random seed
    random_device rd;
    uint64_t k0 = ((uint64_t)rd() << 32) + rd();
    uint64_t k1 = ((uint64_t)rd() << 32) + rd();
    v0 = 0x736f'6d65'7073'6575 ^ k0;
    v1 = 0x646f'7261'6e64'6f6d ^ k1;
    v2 = 0x6c79'6765'6e63'7261 ^ k0;
    v3 = 0x7465'6462'7974'6573 ^ k1;
}


/****************************************************************************
*
*   Helpers
*
***/

//===========================================================================
static uint64_t get64le(const void * ptr) {
    if constexpr (kByteOrder == kLittleEndian) {
        if constexpr (kNativeUnaligned64) {
            return *static_cast<const uint64_t *>(ptr);
        } else {
            uint64_t out;
            memcpy(&out, ptr, sizeof out);
            return out;
        }
    } else {
        if constexpr (kNativeUnaligned64) {
            return byteswap(*static_cast<const uint64_t *>(ptr));
        } else {
            uint64_t out;
            memcpy(&out, ptr, sizeof out);
            return byteswap(out);
        }
    }
}

//===========================================================================
static uint64_t rotateLeft(uint64_t val, unsigned count) {
    return (val << count) | (val >> (64 - count));
}

//===========================================================================
static void sipRound(
    uint64_t & v0,
    uint64_t & v1,
    uint64_t & v2,
    uint64_t & v3
) {
    v0 += v1;
    v1 = rotateLeft(v1, 13);
    v1 ^= v0;
    v0 = rotateLeft(v0, 32);

    v2 += v3;
    v3 = rotateLeft(v3, 16);
    v3 ^= v2;

    v0 += v3;
    v3 = rotateLeft(v3, 21);
    v3 ^= v0;

    v2 += v1;
    v1 = rotateLeft(v1, 17);
    v1 ^= v2;
    v2 = rotateLeft(v2, 32);
}


/****************************************************************************
*
*   Public API
*
***/

//===========================================================================
size_t Dim::hashBytes(const void * ptr, size_t count) {
    static Seed seed;
    uint64_t v0 = seed.v0;
    uint64_t v1 = seed.v1;
    uint64_t v2 = seed.v2;
    uint64_t v3 = seed.v3;

    auto * src = static_cast<const unsigned char *>(ptr);
    size_t wlen = count & ~7;
    auto * esrc = src + wlen;
    uint64_t m;
    for (; src != esrc; src += 8) {
        m = get64le(src);
        v3 ^= m;
        sipRound(v0, v1, v2, v3);
        sipRound(v0, v1, v2, v3);
        v0 ^= m;
    }

    uint64_t b = count % 256;
    m = b << 56;
    switch (count & 7) {
    case 7: m |= (uint64_t)src[6] << 48; [[fallthrough]];
    case 6: m |= (uint64_t)src[5] << 40; [[fallthrough]];
    case 5: m |= (uint64_t)src[4] << 32; [[fallthrough]];
    case 4: m |= (uint64_t)src[3] << 24; [[fallthrough]];
    case 3: m |= (uint64_t)src[2] << 16; [[fallthrough]];
    case 2: m |= (uint64_t)src[1] << 8; [[fallthrough]];
    case 1: m |= (uint64_t)src[0];
    }
    v3 ^= m;
    sipRound(v0, v1, v2, v3);
    sipRound(v0, v1, v2, v3);
    v0 ^= m;

    v2 ^= 0xff;
    sipRound(v0, v1, v2, v3);
    sipRound(v0, v1, v2, v3);
    sipRound(v0, v1, v2, v3);
    sipRound(v0, v1, v2, v3);
    uint64_t out = v0 ^ v1 ^ v2 ^ v3;
    return (size_t) out;
}

//===========================================================================
size_t Dim::hashStr(const char src[]) {
    return hashBytes(src, strlen(src));
}

//===========================================================================
size_t Dim::hashStr(const char src[], size_t maxlen) {
    if (auto * eptr = static_cast<const char *>(memchr(src, 0, maxlen)))
        maxlen = eptr - src;
    return hashBytes(src, maxlen);
}
