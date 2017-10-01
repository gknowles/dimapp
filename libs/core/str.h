// Copyright Glen Knowles 2017.
// Distributed under the Boost Software License, Version 1.0.
//
// str.h - dim core
#pragma once

#include "cppconf/cppconf.h"

#include "math.h"

#include <cassert>
#include <cstdint>
#include <cstdio>
#include <limits>
#include <string>
#include <string_view>
#include <vector>

namespace Dim {

//===========================================================================
// Maximum number of characters needed to represent any integral value of 
// type T in base 10.
template <typename T> constexpr 
std::enable_if_t<std::is_integral_v<T>, int>
maxIntegralChars() {
    return std::numeric_limits<T>::is_signed
        + std::numeric_limits<T>::digits10 + 1;
}

//===========================================================================
// Maximum number of characters needed to represent any floating point value
// of type T in base 10.
template <typename T> constexpr 
std::enable_if_t<std::is_floating_point_v<T>, int>
maxFloatChars() {
    auto cnt = 1 // sign
        + std::numeric_limits<T>::max_digits10 + 1 // digits + decimal point
        + digits10(std::numeric_limits<T>::max_exponent10) + 2; // "e+"
    return (int) cnt;
}


/****************************************************************************
*
*   StrFrom<>
*
***/

template <typename T, typename Enable = void> class StrFrom {};

//===========================================================================
template <typename T>
std::ostream & operator<<(std::ostream & os, const StrFrom<T> & str) {
    os << (std::string_view) str;
    return os;
}

/****************************************************************************
*
*   StrFrom<is_integral_v<T>>
*
*   Convert integral type (char, long, uint16_t, etc) to string_view
*
***/

template <typename T> 
class StrFrom<T, std::enable_if_t<std::is_integral_v<T>>> {
public:
    StrFrom();
    StrFrom(T val) { set(val); }
    std::string_view set(T val);
    operator std::string_view() const;
    const char * c_str() const { return data; }

private:
    using Signed = typename std::make_signed<T>::type;
    using Unsigned = typename std::make_unsigned<T>::type;

    int internalSet(char * ptr, Unsigned val);

    char data[maxIntegralChars<T>() + 1];
};

//===========================================================================
template <typename T> 
StrFrom<T, std::enable_if_t<std::is_integral_v<T>>>::StrFrom() {
    data[0] = 0;
    data[sizeof(data) - 1] = (char) (sizeof(data) - 1);
}

//===========================================================================
template <typename T> 
std::string_view StrFrom<T, std::enable_if_t<std::is_integral_v<T>>>
::set(T val) {
    if constexpr (std::numeric_limits<T>::is_signed) {
        if (val < 0) {
            *data = '-';
            // "-val" is undefined for MIN_INT, so use safe equivalent
            // "~(Unsigned)val + 1" (assumes 2's complement)
            int used = internalSet(data + 1, ~(Unsigned)val + 1);
            data[sizeof(data) - 1] = (char) (sizeof(data) - used - 2);
            return *this;
        }
    }

    int used = internalSet(data, val);
    data[sizeof(data) - 1] = (char) (sizeof(data) - used - 1);
    return *this;
}

//===========================================================================
template <typename T> 
StrFrom<T, std::enable_if_t<std::is_integral_v<T>>>
::operator std::string_view() const {
    return std::string_view(data, sizeof(data) - data[sizeof(data) - 1] - 1);
}

//===========================================================================
template <typename T> 
int StrFrom<T, std::enable_if_t<std::is_integral_v<T>>>
::internalSet(char * ptr, Unsigned val) {
    if (val < 10) {
        // optimize for 0 and 1... and 2 thru 9 since it's no more cost
        ptr[0] = static_cast<char>(val) + '0';
        ptr[1] = 0;
        return 1;
    }

    int i = 0;
    for (;;) {
        ptr[i] = (val % 10) + '0';
        val /= 10;
        i += 1;
        if (!val)
            break;
    }
    ptr[i] = 0;
    auto num = i;
    for (i -= 1; i > 0; i -= 2) {
        swap(*ptr, ptr[i]);
        ptr += 1;
    }
    return num;
}


/****************************************************************************
*
*   StrFrom<is_float_point_v>
*
*   Convert floating point type (float, double, long double) to string_view
*
***/

template <typename T>
class StrFrom<T, std::enable_if_t<std::is_floating_point_v<T>>> {
public:
    StrFrom();
    StrFrom(T val) { set(val); }
    std::string_view set(T val);
    operator std::string_view() const;
    const char * c_str() const { return data; }

private:
    char data[maxFloatChars<T>() + 1];
};

//===========================================================================
template <typename T> 
StrFrom<T, std::enable_if_t<std::is_floating_point_v<T>>>::StrFrom() {
    data[0] = 0;
    data[sizeof(data) - 1] = (char) (sizeof(data) - 1);
}

//===========================================================================
template <typename T> 
std::string_view StrFrom<T, std::enable_if_t<std::is_floating_point_v<T>>>
::set(T val) {
    auto used = std::snprintf(
        data, 
        sizeof(data), 
        "%.*g", 
        numeric_limits<T>::max_digits10, 
        val
    );
    assert(used < sizeof(data));
    data[sizeof(data) - 1] = (char) (sizeof(data) - used - 1);
    return *this;
}

//===========================================================================
template <typename T> 
StrFrom<T, std::enable_if_t<std::is_floating_point_v<T>>>
::operator std::string_view() const {
    return std::string_view(data, sizeof(data) - data[sizeof(data) - 1] - 1);
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


/****************************************************************************
*
*   Convert string to arbitrary type
*
***/

//===========================================================================
// stringTo - converts from string to T
//===========================================================================
template <typename T> bool stringTo(T & out, const std::string & src) {
    if constexpr (std::is_assignable_v<T, const std::string&>) {
        out = src;
    } else {
        std::stringstream interpreter;
        if (!(interpreter << src) || !(interpreter >> out)
            || !(interpreter >> std::ws).eof()) {
            out = {};
            return false;
        }
    }
    return true;
}


/****************************************************************************
*
*   String utilities
*
***/

//===========================================================================
void strSplit(
    std::vector<std::string_view> & out, 
    std::string_view src, 
    char sep = ' '
);


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
    return type == kUtf8 
        ? 3
        : (type == kUtf16BE || type == kUtf16LE)
            ? 2
            : (type == kUtf32BE || type == kUtf32LE) ? 4 : 0;
}


//---------------------------------------------------------------------------
// utf-8
//---------------------------------------------------------------------------

// returns 0 if src doesn't start with a valid utf-8 encoded code point, 
// which includes being empty.
char32_t popFrontUnicode(std::string_view & src);

void appendUnicode(std::string & out, char32_t ch);
size_t unicodeLen(std::string_view src);
std::string toString(std::wstring_view src);

//---------------------------------------------------------------------------
// utf-16
//---------------------------------------------------------------------------

// returns 0 if src doesn't start with a valid utf-16 encoded code point, 
// which includes being empty.
char32_t popFrontUnicode(std::wstring_view & src);

void appendUnicode(std::wstring & out, char32_t ch);
size_t unicodeLen(std::wstring_view src);
std::wstring toWstring(std::string_view src);

} // namespace
