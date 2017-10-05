// Copyright Glen Knowles 2016 - 2017.
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
constexpr void hashCombine(size_t & seed, size_t v) {
    seed ^= v + 0x9e3779b9 + (seed << 6) + (seed >> 2);
}

} // namespace
