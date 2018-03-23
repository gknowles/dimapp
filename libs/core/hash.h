// Copyright Glen Knowles 2016 - 2018.
// Distributed under the Boost Software License, Version 1.0.
//
// hash.h - dim core
#pragma once

#include "cppconf/cppconf.h"

namespace Dim {


/****************************************************************************
*
*   Hashing
*
***/

size_t hashBytes(const void * ptr, size_t count);

size_t hashStr(const char src[]);

// calculates hash up to trailing null or maxlen, whichever comes first
size_t hashStr(const char src[], size_t maxlen);

//===========================================================================
constexpr void hashCombine(size_t * seed, size_t v) {
    *seed ^= v + 0x9e3779b9 + (*seed << 6) + (*seed >> 2);
}

// CRC32c as described by RFC3720. Pass in ~0 when calculating a new CRC or
// pass in the previous result if continuing from a previous buffer.
uint32_t hash_crc32c(uint32_t crc, const void * ptr, size_t count);

// Helper to calculate a new CRC
inline uint32_t hash_crc32c(const void * ptr, size_t count) {
    return hash_crc32c((uint32_t) ~0, ptr, count);
}

} // namespace
