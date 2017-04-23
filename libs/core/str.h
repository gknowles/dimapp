// Copyright Glen Knowles 2017.
// Distributed under the Boost Software License, Version 1.0.
//
// str.h - dim core
#pragma once

#include "cppconf/cppconf.h"

#include <cstdint>
#include <limits>

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


/****************************************************************************
*
*   String to integral
*
***/

int strToInt(const char src[], char ** eptr, int base);
unsigned strToUint(const char src[], char ** eptr, int base);
int64_t strToInt64(const char src[], char ** eptr, int base);
uint64_t strToUint64(const char src[], char ** eptr, int base);

} // namespace
