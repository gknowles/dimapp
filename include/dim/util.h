// util.h - dim core
#pragma once

#include "config.h"

#include <cassert>
#include <climits>
#include <experimental/filesystem>
#include <limits>
#include <sstream>
#include <type_traits>

namespace Dim {


/****************************************************************************
*
*   Hashing
*
***/

size_t hashStr(const char src[]);

// calculates hash up to trailing null or maxlen, whichever comes first
size_t hashStr(const char src[], size_t maxlen);

//===========================================================================
inline void hashCombine(size_t & seed, size_t v) {
    seed ^= v + 0x9e3779b9 + (seed << 6) + (seed >> 2);
}


/****************************************************************************
*
*   Math
*
***/

//===========================================================================
// Number of digits required to display a number in decimal
inline int digits10(uint32_t val) {
    const int DeBruijnBitPositionAdjustedForLog10[] = {
        0, 3, 0, 3, 4, 6, 0, 9, 3, 4, 5, 5, 6, 7, 1, 9,
        2, 3, 6, 8, 4, 5, 7, 2, 6, 8, 7, 2, 8, 1, 1, 9,
    };

    // round down to one less than a power of 2
    uint32_t v = val;
    v |= v >> 1;
    v |= v >> 2;
    v |= v >> 4;
    v |= v >> 8;
    v |= v >> 16;
    int r = DeBruijnBitPositionAdjustedForLog10[(v * 0x07c4acdd) >> 27];

    const uint32_t PowersOf10[] = {
        1,
        10,
        100,
        1000,
        10000,
        100000,
        1000000,
        10000000,
        100000000,
        1000000000,
    };
    r += PowersOf10[r] >= val;
    return r;
}

//===========================================================================
// Round up to power of 2
inline size_t pow2Ceil(uint64_t num) {
#if 0
    unsigned long k;
    _BitScanReverse64(&k, num);
    return (size_t) 1 << (k + 1);
#endif
#if 0
    size_t k = 0x8000'0000;
    k = (k > num) ? k >> 16 : k << 16;
    k = (k > num) ? k >> 8 : k << 8;
    k = (k > num) ? k >> 4 : k << 4;
    k = (k > num) ? k >> 2 : k << 2;
    k = (k > num) ? k >> 1 : k << 1;
    return k;
#endif
#if 1
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
#if 0
#pragma warning(disable : 4706) // assignment within conditional expression
    size_t j, k;
    (k = num & 0xFFFF'FFFF'0000'0000) || (k = num);
    (j = k & 0xFFFF'0000'FFFF'0000) || (j = k);
    (k = j & 0xFF00'FF00'FF00'FF00) || (k = j);
    (j = k & 0xF0F0'F0F0'F0F0'F0F0) || (j = k);
    (k = j & 0xCCCC'CCCC'CCCC'CCCC) || (k = j);
    (j = k & 0xAAAA'AAAA'AAAA'AAAA) || (j = k);
    return j << 1;
#endif
}


/****************************************************************************
*
*   Unicode
*
***/

enum UtfType {
    kUtfUnknown,
    kUtf8,
    kUtf16BE,
    kUtf16LE,
    kUtf32BE,
    kUtf32LE,
};

UtfType utfBomType(const char bytes[], size_t count);

//===========================================================================
constexpr size_t utfBomSize(UtfType type) {
    return type == kUtf8 ? 3
                         : (type == kUtf16BE || type == kUtf16LE)
            ? 2
            : (type == kUtf32BE || type == kUtf32LE) ? 4 : 0;
}


/****************************************************************************
*
*   String conversions
*
***/

//===========================================================================
template <typename T> constexpr int maxIntegralChars() {
    return numeric_limits<T>::is_signed
        ? 1 + ((CHAR_BIT * sizeof(T) - 1) * 301L + 999L) / 1000L
        : (CHAR_BIT * sizeof(T) * 301L + 999L) / 1000L;
}


//===========================================================================
// IntegralStr - convert integral type (char, long, uint16_t, etc) to string
//===========================================================================
template <typename T> class IntegralStr {
public:
    IntegralStr(T val);
    const char * set(T val);
    operator const char *() const;

private:
    using Signed = typename std::make_signed<T>::type;
    using Unsigned = typename std::make_unsigned<T>::type;

    const char * internalSet(Signed val);
    const char * internalSet(Unsigned val);

    char data[maxIntegralChars<T>() + 1];
};

//===========================================================================
template <typename T> IntegralStr<T>::IntegralStr(T val) {
    internalSet(val);
}

//===========================================================================
template <typename T> const char * IntegralStr<T>::set(T val) {
    return internalSet(val);
}

//===========================================================================
template <typename T> IntegralStr<T>::operator const char *() const {
    return data;
}

//===========================================================================
template <typename T> const char * IntegralStr<T>::internalSet(Unsigned val) {
    if (val < 10) {
        // optimize for 0 and 1... and 2 thru 9 since it's no more cost
        data[0] = static_cast<char>(val) + '0';
        data[1] = 0;
    } else {
        char * ptr = data;
        unsigned i = 0;
        for (;;) {
            ptr[i] = (val % 10) + '0';
            val /= 10;
            i += 1;
            if (!val)
                break;
        }
        ptr[i] = 0;
        for (; i > 1; i -= 2) {
            swap(*ptr, ptr[i]);
            ptr += 1;
        }
    }
    return data;
}

//===========================================================================
template <typename T> const char * IntegralStr<T>::internalSet(Signed val) {
    if (!val) {
        data[0] = '0';
        data[1] = 0;
    } else {
        char * ptr = data;
        if (val < 0) {
            *ptr++ = '-';
            // "-val" is undefined for MIN_INT, so use safe equivalent
            // "~(Unsigned)val + 1" (assumes 2's complement)
            val = ~(Unsigned)val + 1;
        }
        unsigned i = 0;
        for (;;) {
            ptr[i] = (val % 10) + '0';
            val /= 10;
            i += 1;
            if (!val)
                break;
        }
        ptr[i] = 0;
        for (; i > 1; i -= 2) {
            swap(*ptr, ptr[i]);
            ptr += 1;
        }
    }
    return data;
}


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
// hexToUnsigned - converts hex character (0-9, a-f, A-F) to unsigned (0-15)
//===========================================================================
inline unsigned hexToUnsigned(char ch) {
    return ((ch | 432) * 239'217'992 & 0xffff'ffff) >> 28;
}


/****************************************************************************
*
*   Containers
*
***/

template <typename T> class ForwardListIterator {
protected:
    T * m_current{nullptr};

public:
    ForwardListIterator(T * node)
        : m_current(node) {}
    bool operator!=(const ForwardListIterator & right) {
        return m_current != right.m_current;
    }
    ForwardListIterator & operator++() {
        m_current = m_current->m_next;
        return *this;
    }
    T & operator*() {
        assert(m_current);
        return *m_current;
    }
};

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
