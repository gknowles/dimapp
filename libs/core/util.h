// Copyright Glen Knowles 2016 - 2017.
// Distributed under the Boost Software License, Version 1.0.
//
// util.h - dim core
#pragma once

#include "cppconf/cppconf.h"

#include <cassert>
#include <climits>
#include <cstdint>
#include <experimental/filesystem>
#include <limits>
#include <sstream>
#include <type_traits>

namespace Dim {


/****************************************************************************
*
*   Crypt random
*
***/

void cryptRandomBytes(void * ptr, size_t count);


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
    x = ((x & 0xaaaa'aaaa'aaaa'aaaa) >> 1) | ((x & 0x5555'5555'5555'5555) << 1);
    x = ((x & 0xcccc'cccc'cccc'cccc) >> 2) | ((x & 0x3333'3333'3333'3333) << 2);
    x = ((x & 0xf0f0'f0f0'f0f0'f0f0) >> 4) | ((x & 0x0f0f'0f0f'0f0f'0f0f) << 4);
    x = ((x & 0xff00'ff00'ff00'ff00) >> 8) | ((x & 0x00ff'00ff'00ff'00ff) << 8);
    x = ((x & 0xffff'0000'ffff'0000) >>16) | ((x & 0x0000'ffff'0000'ffff) <<16);
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
    return kTable[((val ^ (val - 1)) * 0x3f79d71b4cb0a89) >> 58];
}

//===========================================================================
// Find 1 bit in array of unsigned integrals.
// Returns true and position of bit, or false if no 1's are found.
constexpr bool findBit(
    int * out, 
    const uint64_t arr[], 
    size_t arrlen, 
    int bitpos = 0
) {
    const int kIntBits = sizeof(*arr) * 8;
    int pos = bitpos / kIntBits;
    if (pos >= arrlen)
        return false;
    auto ptr = arr + pos;
    auto val = *ptr & (std::numeric_limits<uint64_t>::max() << pos % kIntBits);
    if (!val) {
        *out = pos * kIntBits + trailingZeroBits(val);
        return true;
    }
    auto last = arr + arrlen;
    for (; ptr != last; ++ptr) {
        if (auto val = *ptr) {
            *out = int(ptr - arr) * kIntBits + trailingZeroBits(val);
            return true;
        }
    }
    return false;
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
// Number of bits in num that are set
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
    const int kDeBruijnBitPositionAdjustedForLog10[] = {
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
    int r = kDeBruijnBitPositionAdjustedForLog10[(v * 0x07c4acdd) >> 27];

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
*   String conversions
*
***/

//===========================================================================
// stringTo - converts from string to T
//===========================================================================
template <typename T>
auto stringTo_impl(T & out, const std::string & src, int)
    -> decltype(out = src, bool()) {
    out = src;
    return true;
}

//===========================================================================
template <typename T>
bool stringTo_impl(T & out, const std::string & src, long) {
    std::stringstream interpreter;
    if (!(interpreter << src) || !(interpreter >> out)
        || !(interpreter >> std::ws).eof()) {
        out = {};
        return false;
    }
    return true;
}

//===========================================================================
template <typename T> bool stringTo(T & out, const std::string & src) {
    // prefer the version of stringTo_impl taking an int as it's third
    // parameter, if that doesn't exist for T (because no out=src assignment
    // operator exists) the version taking a long is called.
    return stringTo_impl(out, src, 0);
}


//===========================================================================
// hexToNibble - converts hex character (0-9, a-f, A-F) to unsigned (0-15)
//===========================================================================
constexpr unsigned hexToNibble(char ch) {
    return ((ch | 432) * 239'217'992 & 0xffff'ffff) >> 28;
}

//===========================================================================
constexpr char hexFromNibble(unsigned val) {
    assert(val < 16);
    const char s_chars[] = "0123456789abcdef";
    return s_chars[val];
}


/****************************************************************************
*
*   IFactory
*
*   If a service defines Base* and accepts IFactory<Base>* in its interface, 
*   clients that define their own class (Derived) derived from Base can use
*   getFactory<Base, Derived>() to get a pointer to a suitable static factory.
*   
*   Example usage:
*   // Service interface
*   void socketListen(IFactory<ISocketNotify> * fact, const Endpoint & e);
*
*   // Client implementation
*   class MySocket : public ISocketNotify {
*   };
*
*   socketListen(getFactory<ISocketNotify, MySocket>(), endpoint);
*
***/

template<typename T>
class IFactory {
public:
    virtual ~IFactory() {}
    virtual std::unique_ptr<T> onFactoryCreate() = 0;
};

//===========================================================================
template<typename Base, typename Derived> inline
std::enable_if_t<std::is_base_of_v<Base, Derived>, IFactory<Base>*>
getFactory() {
    class Factory : public IFactory<Base> {
        std::unique_ptr<Base> onFactoryCreate() override {
            return std::make_unique<Derived>();
        }
    };
    // As per http://en.cppreference.com/w/cpp/language/inline
    // "In an inline function, function-local static objects in all function
    // definitions are shared across all translation units (they all refer to
    // the same object defined in one translation unit)" 
    //
    // Note that this is a difference between C and C++
    static Factory s_factory;
    return &s_factory;
}

} // namespace


/****************************************************************************
*
*   filesystem::path
*
***/

//===========================================================================
template <>
inline std::experimental::filesystem::path::path(const std::string & from) {
    *this = u8path(from);
}

//===========================================================================
template <>
inline std::experimental::filesystem::path::path(
    const char * first, 
    const char * last
) {
    *this = u8path(first, last);
}
