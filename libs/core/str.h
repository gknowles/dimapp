// Copyright Glen Knowles 2017.
// Distributed under the Boost Software License, Version 1.0.
//
// str.h - dim core
#pragma once

#include "cppconf/cppconf.h"

#include <cstdint>
#include <limits>
#include <string_view>

namespace Dim {


template <typename T> constexpr int maxIntegralChars() {
    return numeric_limits<T>::is_signed
        ? 1 + ((CHAR_BIT * sizeof(T) - 1) * 301L + 999L) / 1000L
        : (CHAR_BIT * sizeof(T) * 301L + 999L) / 1000L;
}


/****************************************************************************
*
*   IntegralStr
*
*   Convert integral type (char, long, uint16_t, etc) to char array
*
***/

template <typename T> class IntegralStr {
public:
    IntegralStr(T val);
    std::string_view set(T val);
    operator std::string_view() const;

private:
    using Signed = typename std::make_signed<T>::type;
    using Unsigned = typename std::make_unsigned<T>::type;

    std::string_view internalSet(Signed val);
    std::string_view internalSet(Unsigned val);

    char data[maxIntegralChars<T>() + 1];
};

//===========================================================================
template <typename T> IntegralStr<T>::IntegralStr(T val) {
    internalSet(val);
}

//===========================================================================
template <typename T> std::string_view IntegralStr<T>::set(T val) {
    return internalSet(val);
}

//===========================================================================
template <typename T> IntegralStr<T>::operator std::string_view() const {
    return std::string_view(data, data[sizeof(data) - 1]);
}

//===========================================================================
template <typename T> 
std::string_view IntegralStr<T>::internalSet(Unsigned val) {
    if (val < 10) {
        // optimize for 0 and 1... and 2 thru 9 since it's no more cost
        data[0] = static_cast<char>(val) + '0';
        data[1] = 0;
        data[sizeof(data) - 1] = 1;
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
        data[sizeof(data) - 1] = (char) i;
        for (; i > 1; i -= 2) {
            swap(*ptr, ptr[i]);
            ptr += 1;
        }
    }
    return *this;
}

//===========================================================================
template <typename T> 
std::string_view IntegralStr<T>::internalSet(Signed val) {
    if (!val) {
        data[0] = '0';
        data[1] = 0;
        data[sizeof(data) - 1] = 1;
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
        data[sizeof(data) - 1] = (char) i;
        for (; i > 1; i -= 2) {
            swap(*ptr, ptr[i]);
            ptr += 1;
        }
    }
    return *this;
}


/****************************************************************************
*
*   String to integral
*
***/

int strToInt(const char src[], char ** eptr = nullptr, int base = 10);
unsigned strToUint(const char src[], char ** eptr = nullptr, int base = 10);
int64_t strToInt64(const char src[], char ** eptr = nullptr, int base = 10);
uint64_t strToUint64(const char src[], char ** eptr = nullptr, int base = 10);

int strToInt(std::string_view src, char ** eptr = nullptr, int base = 10);
unsigned strToUint(
    std::string_view src, 
    char ** eptr = nullptr, 
    int base = 10
);
int64_t strToInt64(
    std::string_view src, 
    char ** eptr = nullptr, 
    int base = 10
);
uint64_t strToUint64(
    std::string_view src, 
    char ** eptr = nullptr, 
    int base = 10
);

} // namespace
