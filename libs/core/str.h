// Copyright Glen Knowles 2017 - 2018.
// Distributed under the Boost Software License, Version 1.0.
//
// str.h - dim core
#pragma once

#include "cppconf/cppconf.h"

#include "math.h"

#include <cassert>
#include <charconv>
#include <cstdint>
#include <cstdio>
#include <limits>
#include <memory>
#include <string>
#include <sstream>
#include <string_view>
#include <utility>
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
        // optimize for 0 and 1... and 2 through 9 since it's no more cost
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
        std::swap(*ptr, ptr[i]);
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

#if 1
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

#else
//===========================================================================
template <typename T>
std::string_view StrFrom<T, std::enable_if_t<std::is_floating_point_v<T>>>
::set(T val) {
    auto r = std::to_chars(
        data,
        data + sizeof(data),
        val,
        std::chars_format::general
    );
    auto used = r.ptr - data;
    assert(used < sizeof(data));
    data[used] = 0;
    data[sizeof(data) - 1] = (char) (sizeof(data) - used - 1);
    return *this;
}
#endif

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
*   String to float
*
***/

// Uses strtod, but also supports k/K, ki/Ki, M/Mi, suffixes
[[nodiscard]] bool parse(double * out, std::string_view src);


/****************************************************************************
*
*   Parse string into arbitrary type
*
***/

//===========================================================================
// parse - converts from string to T
//===========================================================================
template <typename T>
[[nodiscard]] bool parse(T * out, std::string_view src) {
    if constexpr (std::is_assignable_v<T, std::string_view>) {
        *out = src;
    } else if constexpr (std::is_assignable_v<T, std::string>) {
        *out = std::string{src};
    } else {
        std::stringstream interpreter(std::string{src});
        if (!(interpreter >> *out) || !(interpreter >> std::ws).eof()) {
            *out = {};
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

void split(
    std::vector<std::string_view> * out,
    std::string_view src,
    char sep = ' '
);
void split(
    std::vector<std::string_view> * out,
    std::string_view src,
    std::string_view seps // = "\r\n"
);

// Removes leading and/or trailing whitespace characters (as defined by
// isspace).
std::string_view trim(std::string_view src);
std::string_view ltrim(std::string_view src);
std::string_view rtrim(std::string_view src);

// Splits source into lines, trims trailing spaces, leading and trailing
// blank lines, and removes a number of leading whitespace characters from
// each line equal to the smallest number of leading whitespace characters of
// any line that still has characters.
std::string trimBlock(std::string_view src);

std::unique_ptr<char[]> strDup(std::string_view src);

//===========================================================================
// vector to string
//===========================================================================
template<typename T>
class VectorProxy {
public:
    VectorProxy(const std::vector<T> & src) : m_src{src} {}
    const std::vector<T> & m_src;
};

//===========================================================================
template<typename T>
std::string toString(VectorProxy<T> proxy, char sep = ' ') {
    auto & src = proxy.m_src;
    if (src.empty())
        return {};
    std::ostringstream os;
    os << src[0];
    for (auto i = src.begin() + 1; i < src.end(); ++i) {
        os << sep;
        os << *i;
    }
    return os.str();
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
    return type == kUtf8
        ? 3
        : (type == kUtf16BE || type == kUtf16LE)
            ? 2
            : (type == kUtf32BE || type == kUtf32LE) ? 4 : 0;
}


//---------------------------------------------------------------------------
// UTF-8
//---------------------------------------------------------------------------

// returns 0 if src doesn't start with a valid UTF-8 encoded code point,
// which includes being empty.
char32_t popFrontUnicode(std::string_view * src);

size_t unicodeLen(std::string_view src);
std::string toString(std::wstring_view src);

struct ostream_utf8_return { std::wstring_view src; };
constexpr ostream_utf8_return utf8(std::wstring_view src) {
    return {src};
}
constexpr ostream_utf8_return utf8(const wchar_t src[]) {
    return {src};
}
constexpr ostream_utf8_return utf8(const wchar_t src[], size_t srclen) {
    return {std::wstring_view(src, srclen)};
}
std::ostream & operator<<(std::ostream & os, const ostream_utf8_return & out);

//===========================================================================
template<typename OutIt>
constexpr OutIt copy_char(char32_t ch, OutIt out) {
    if (ch < 0x80) {
        *out++ = (unsigned char) ch;
    } else if (ch < 0x800) {
        *out++ = (unsigned char) ((ch >> 6) + 0xc0);
        *out++ = (unsigned char) ((ch & 0x3f) + 0x80);
    } else  if (ch < 0xd800 || ch > 0xdfff && ch < 0x10'000) {
        *out++ = (unsigned char) ((ch >> 12) + 0xe0);
        *out++ = (unsigned char) (((ch >> 6) & 0x3f) + 0x80);
        *out++ = (unsigned char) ((ch & 0x3f) + 0x80);
    } else if (ch >= 0x10'000 && ch < 0x11'0000) {
        *out++ = (unsigned char) ((ch >> 18) + 0xf0);
        *out++ = (unsigned char) (((ch >> 12) & 0x3f) + 0x80);
        *out++ = (unsigned char) (((ch >> 6) & 0x3f) + 0x80);
        *out++ = (unsigned char) ((ch & 0x3f) + 0x80);
    } else {
        assert(ch >= 0xd800 && ch <= 0xdfff || ch >= 0x11'000);
        assert(!"invalid Unicode char");
    }
    return out;
}

//===========================================================================
template<typename OutIt>
constexpr OutIt copy_char(std::wstring_view in, OutIt out) {
    char32_t popFrontUnicode(std::wstring_view * src);
    while (auto ch = popFrontUnicode(&in))
        out = copy_char(ch, out);
    return out;
}


//---------------------------------------------------------------------------
// UTF-16
//---------------------------------------------------------------------------

// returns 0 if src doesn't start with a valid UTF-16 encoded code point,
// which includes being empty.
char32_t popFrontUnicode(std::wstring_view * src);

size_t unicodeLen(std::wstring_view src);
std::wstring toWstring(std::string_view src);

//===========================================================================
template<typename OutIt>
constexpr OutIt copy_wchar(char32_t ch, OutIt out) {
    if (ch <= 0xffff) {
        *out++ = (wchar_t) ch;
    } else if (ch <= 0x10ffff) {
        *out++ = (wchar_t) ((ch >> 10) + 0xd800);
        *out++ = (wchar_t) ((ch & 0x3ff) + 0xdc00);
    } else {
        assert(!"invalid Unicode char");
    }
    return out;
}

//===========================================================================
template<typename OutIt>
constexpr OutIt copy_wchar(std::string_view in, OutIt out) {
    while (auto ch = popFrontUnicode(&in))
        out = copy_wchar(ch, out);
    return out;
}

} // namespace
