// Copyright Glen Knowles 2017 - 2022.
// Distributed under the Boost Software License, Version 1.0.
//
// str.h - dim core
#pragma once

#include "cppconf/cppconf.h"

#include "math.h"   // digits10
#include "types.h"  // CharType

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
template <typename T>
requires std::is_integral_v<T>
constexpr int maxNumericChars() {
    return std::numeric_limits<T>::is_signed
        + std::numeric_limits<T>::digits10 + 1;
}

//===========================================================================
// Maximum number of characters needed to represent any value of the
// underlying type of T in base 10.
template <typename T>
requires std::is_enum_v<T>
constexpr int maxNumericChars() {
    return maxNumericChars<std::underlying_type_t<T>>();
}

//===========================================================================
// Maximum number of characters needed to represent any floating point value
// of type T in base 10.
template <typename T>
requires std::is_floating_point_v<T>
constexpr int maxNumericChars() {
    constexpr auto cnt = 1 // sign
        + std::numeric_limits<T>::max_digits10 + 1 // digits + decimal point
        + digits10(std::numeric_limits<T>::max_exponent10) + 2; // "e+"
    return (int) cnt;
}


/****************************************************************************
*
*   Arithmetic to string
*
*   Convert arithmetic type to char buffer, handles both floating point (float,
*   double, long double) and integral (char, long, uint16_t, etc) types.
*
***/

template <typename T>
requires (std::is_arithmetic_v<T> || std::is_enum_v<T>)
class ToCharsBuf;

//===========================================================================
template <typename T>
ToCharsBuf<T> toChars(T val) {
    return ToCharsBuf<T>(val);
}

//===========================================================================
template <typename T>
requires (std::is_arithmetic_v<T> || std::is_enum_v<T>)
class ToCharsBuf {
public:
    ToCharsBuf();
    explicit ToCharsBuf(T val) { set(val); }
    std::string_view set(T val);
    std::string_view view() const;
    const char * c_str() const & { return data(); }

    size_t size() const;
    bool empty() const { return *m_data == 0; }
    const char * data() const & { return m_data; }

private:
    friend std::ostream & operator<<(
        std::ostream & os,
        const ToCharsBuf & str
    ) {
        os << str.view();
        return os;
    }

private:
    char m_data[maxNumericChars<T>() + 1];
};

//===========================================================================
template <typename T>
requires (std::is_arithmetic_v<T> || std::is_enum_v<T>)
ToCharsBuf<T>::ToCharsBuf() {
    m_data[0] = 0;
    m_data[sizeof m_data - 1] = (char) (sizeof m_data - 1);
}

//===========================================================================
template <typename T>
requires (std::is_arithmetic_v<T> || std::is_enum_v<T>)
std::string_view ToCharsBuf<T>::set(T val) {
    char * ptr;
    if constexpr (std::is_enum_v<T>) {
        auto r = std::to_chars(
            m_data,
            m_data + sizeof m_data,
            static_cast<std::underlying_type_t<T>>(val)
        );
        ptr = r.ptr;
    } else {
        auto r = std::to_chars(m_data, m_data + sizeof m_data, val);
        ptr = r.ptr;
    }
    auto used = ptr - m_data;
    assert(used < sizeof m_data);
    m_data[used] = 0;
    m_data[sizeof m_data - 1] = (char) (sizeof m_data - used - 1);
    return view();
}

//===========================================================================
template <typename T>
requires (std::is_arithmetic_v<T> || std::is_enum_v<T>)
std::string_view ToCharsBuf<T>::view() const {
    return std::string_view(data(), size());
}

//===========================================================================
template <typename T>
requires (std::is_arithmetic_v<T> || std::is_enum_v<T>)
size_t ToCharsBuf<T>::size() const {
    return sizeof m_data - m_data[sizeof m_data - 1] - 1;
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
// parse - converts from string to T, works when at least one of the following
// is true for T:
//  - is assignable from string or string_view
//  - is constructible from string or string_view
//  - is arithmetic and works with from_chars
//  - is non-arithmetic and has an istream extraction operator
//===========================================================================
template <typename T>
[[nodiscard]] bool parse(T * out, std::string_view src) {
    if constexpr (std::is_assignable_v<T, std::string_view>) {
        *out = src;
    } else if constexpr (std::is_constructible_v<T, std::string_view>) {
        *out = T(src);
    } else if constexpr (std::is_assignable_v<T, std::string>) {
        *out = std::string{src};
    } else if constexpr (std::is_constructible_v<T, std::string>) {
        *out = T(std::string(src));
    } else if constexpr (std::is_arithmetic_v<T>) {
        auto r = std::from_chars(src.data(), src.data() + src.size(), *out);
        if (r.ec != std::errc{} || r.ptr != src.data() + src.size()) {
            *out = {};
            return false;
        }
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

// Splits on every appearance of 'sep' in the source.
void split(
    std::vector<std::string_view> * out,
    std::string_view src,
    char sep = ' '
);
// Splits whenever any of the listed 'seps' are found.
void split(
    std::vector<std::string_view> * out,
    std::string_view src,
    std::string_view seps // = "\r\n"
);
// Splits whenever any of the listed 'seps' are found, or after maxLen chars
// go by without encountering any of 'seps'.
void split(
    std::vector<std::string_view> * out,
    std::string_view src,
    std::string_view seps,
    size_t maxLen
);

// Removes leading and/or trailing whitespace characters (as defined by
// isspace).
std::string_view trim(std::string_view src);
std::string_view ltrim(std::string_view src);
std::string_view rtrim(std::string_view src);

// Splits source into lines, trims trailing spaces, leading and trailing blank
// lines. Finally, removes an amount of leading whitespace from each line equal
// to the smallest amount of leading whitespace of any line that still has
// characters.
std::string trimBlock(std::string_view src);

std::unique_ptr<char[]> strDup(std::string_view src);

// Copies contents of all srcs into a single buffer, with null terminators, and
// updates the srcs to point into that newly allocated buffer.
std::unique_ptr<char[]> strDupGather(std::string_view * srcs[], size_t count);

// Never copies more than outLen chars/wchar_ts (including null), returns
// number of chars copied (not including null). outLen must be >= 1, so the
// null can fit.
size_t strCopy(char * out, size_t outLen, std::string_view src);
size_t strCopy(wchar_t * out, size_t outLen, std::wstring_view src);
size_t strCopy(char * out, size_t outLen, std::wstring_view src);
size_t strCopy(wchar_t * out, size_t outLen, std::string_view src);

//===========================================================================
// vector to string
//===========================================================================
template<typename T>
requires (std::forward_iterator<typename T::const_iterator>
    && !CharType<typename T::value_type>)
std::string toString(const T & src, char sep = ' ') {
    if (src.empty())
        return {};
    std::ostringstream os;
    typename T::const_iterator i = src.cbegin();
    os << *i;
    while (++i < src.cend()) {
        os << sep;
        os << *i;
    }
    return os.str();
}

//===========================================================================
// upper & lower case
//===========================================================================
char * toLower(char src[]);
char * toUpper(char src[]);
char * toLower(char src[], size_t srclen);
char * toUpper(char src[], size_t srclen);

std::string toLower(std::string_view src);
std::string toUpper(std::string_view src);


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
    switch (type) {
    case kUtf8:
        return 3;
    case kUtf16BE: case kUtf16LE:
        return 2;
    case kUtf32BE: case kUtf32LE:
        return 4;
    default:
        return 0;
    }
}


//---------------------------------------------------------------------------
// UTF-8
//---------------------------------------------------------------------------

// returns 0 if src doesn't start with a valid UTF-8 encoded code point,
// which includes being empty.
char32_t popFrontUnicode(std::string_view * src);

size_t unicodeLen(std::string_view src);
std::string toString(std::wstring_view src);

struct ostream_utf8_return {
    std::wstring_view src;
private:
    friend std::ostream & operator<<(
        std::ostream & os,
        const ostream_utf8_return & out
    );
};
constexpr ostream_utf8_return utf8(wchar_t wch) {
    return {std::wstring_view(&wch, 1)};
}
constexpr ostream_utf8_return utf8(std::wstring_view src) {
    return {src};
}
constexpr ostream_utf8_return utf8(const wchar_t src[]) {
    return {src};
}
constexpr ostream_utf8_return utf8(const wchar_t src[], size_t srclen) {
    return {std::wstring_view(src, srclen)};
}

//===========================================================================
template<typename OutIt>
requires std::output_iterator<OutIt, unsigned char>
constexpr OutIt writeUtf8(OutIt out, char32_t ch) {
    if (ch < 0x80) {
        *out++ = (unsigned char) ch;
        return out;
    } else if (ch < 0x800) {
        *out++ = (unsigned char) ((ch >> 6) + 0xc0);
        *out++ = (unsigned char) ((ch & 0x3f) + 0x80);
        return out;
    } else if (ch < 0x1'0000) {
        if (ch < 0xd800 || ch > 0xdfff) {
            *out++ = (unsigned char) ((ch >> 12) + 0xe0);
            *out++ = (unsigned char) (((ch >> 6) & 0x3f) + 0x80);
            *out++ = (unsigned char) ((ch & 0x3f) + 0x80);
            return out;
        }
    } else if (ch < 0x11'0000) {
        *out++ = (unsigned char) ((ch >> 18) + 0xf0);
        *out++ = (unsigned char) (((ch >> 12) & 0x3f) + 0x80);
        *out++ = (unsigned char) (((ch >> 6) & 0x3f) + 0x80);
        *out++ = (unsigned char) ((ch & 0x3f) + 0x80);
        return out;
    }

    assert(ch >= 0xd800 && ch <= 0xdfff || ch > 0x10'ffff);
    assert(!"invalid Unicode char");
    return out;
}

//===========================================================================
template<typename OutIt>
requires std::output_iterator<OutIt, unsigned char>
constexpr OutIt writeUtf8(OutIt out, std::wstring_view in) {
    char32_t popFrontUnicode(std::wstring_view * src);
    while (!in.empty()) {
        if (auto ch = popFrontUnicode(&in)) {
            out = writeUtf8(out, ch);
        } else {
            break;
        }
    }
    return out;
}


//---------------------------------------------------------------------------
// UTF-16
//---------------------------------------------------------------------------

// Returns 0 if src is empty or starts with an invalid UTF-16 encoded code
// point.
char32_t popFrontUnicode(std::wstring_view * src);

size_t unicodeLen(std::wstring_view src);
std::wstring toWstring(std::string_view src);

//===========================================================================
template<typename OutIt>
requires std::output_iterator<OutIt, wchar_t>
constexpr OutIt writeWchar(OutIt out, char32_t ch) {
    if (ch <= 0xffff) {
        *out++ = (wchar_t) ch;
        return out;
    } else if (ch <= 0x10ffff) {
        *out++ = (wchar_t) ((ch >> 10) + 0xd800);
        *out++ = (wchar_t) ((ch & 0x3ff) + 0xdc00);
        return out;
    }

    assert(ch > 0x10'ffff);
    assert(!"invalid Unicode char");
    return out;
}

//===========================================================================
template<typename OutIt>
requires std::output_iterator<OutIt, wchar_t>
constexpr OutIt writeWchar(OutIt out, std::string_view in) {
    while (!in.empty()) {
        if (auto ch = popFrontUnicode(&in)) {
            out = writeWchar(out, ch);
        } else {
            break;
        }
    }
    return out;
}

} // namespace
