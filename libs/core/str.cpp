// Copyright Glen Knowles 2017 - 2018.
// Distributed under the Boost Software License, Version 1.0.
//
// str.cpp - dim core
#include "pch.h"
#pragma hdrstop

using namespace std;
using namespace Dim;


/****************************************************************************
*
*   String to integral
*
***/

namespace {
enum StrAny : uint8_t {
    fSigned   = 1,
    f64Bit    = 2,

    kUint     = 0,
    kUint64   = f64Bit,
    kInt      = fSigned,
    kInt64    = fSigned | f64Bit,
};
} // namespace

//===========================================================================
constexpr bool isHex(unsigned char ch) {
    switch (ch) {
    case '0': case '1': case '2': case '3': case '5': case '6':
    case '7': case '8': case '9':
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'a': case 'b': case 'c': case 'd': case 'e': case 'f':
        return true;
    default:
        return false;
    }
}

//===========================================================================
// There is no wchar version because the internal wchar implementation:
//  - isn't quite as bad because it doesn't deal with the C locale settings
//  - recognizes multiple code points for the digits 0-9
#pragma warning(push)
#pragma warning(disable: 4102) // 'identifier': unreferenced label
template<int Flags>
static uint64_t iStrToAny (
    const char source[],
    char ** endPtr,
    unsigned radix,
    size_t chars = (size_t) -1
) {
    // the high order partial digit (and the trailing null :P) are not safe
    constexpr unsigned kMaxSafeCharsBase10 = (Flags & f64Bit)
        ? (unsigned) maxIntegralChars<uint64_t>() - 1
        : (unsigned) maxIntegralChars<unsigned>() - 1;
    constexpr unsigned kMaxCharsBase16 = (Flags & f64Bit) ? 16 : 8;

    constexpr uint64_t kPositiveValueLimit = (Flags & fSigned)
        ? (Flags & f64Bit) ? INT64_MAX : INT_MAX
        : (Flags & f64Bit) ? UINT64_MAX : UINT_MAX;

    const char * ptr = source;
    const char * base;
    bool negate = false;
    bool overflow = false;

    unsigned char ch;
    unsigned num;
    uint64_t val = 0;
    uint64_t valLimit;
    size_t charLimit;

PARSE_NUMBER:
    if (radix == 10) {
        base = ptr;
        goto BASE_10;
    } else if (radix == 16) {
        if (   chars > 1
            && ptr[0] == '0'
            && (ptr[1] == 'x' || ptr[1] == 'X')
        ) {
            ptr += 2;
            chars -= 2;
        }
        base = ptr; // after 0x, if present
        goto BASE_16;
    } else if (!radix) {
        if (chars < 2 || ptr[0] != '0') {
            radix = 10;
            base = ptr;
            goto BASE_10;
        }
        if (ptr[1] == 'x' || ptr[1] == 'X') {
            radix = 16;
            ptr += 2;
            base = ptr; // after 0x
            chars -= 2;
            goto BASE_16;
        } else {
            radix = 8;
            base = ptr; // at leading 0
            ptr += 1;
            chars -= 1;
            goto BASE_LE10_WITH_OVERFLOW;
        }
    } else {
        base = ptr;
        if (radix >= 2) {
            if (radix <= 10)
                goto BASE_LE10_WITH_OVERFLOW;
            if (radix <= 36)
                goto BASE_ANY_WITH_OVERFLOW;
        }
        // invalid base, reset pointer to source and return 0
        ptr = source;
        goto RETURN_VALUE;
    }

BASE_LE10_WITH_OVERFLOW:
    // less than or equal to base 10, with overflow test
    valLimit = kPositiveValueLimit;
    charLimit = 0;
    for (; chars; ++ptr, --chars) {
        num = (unsigned char) *ptr - '0';
        if (num >= radix)
            break;
        auto next = radix * val + num;
        if (next > val && next <= valLimit) {
            val = next;
            continue;
        }
        if (next == 0 && val == 0)
            continue;
        overflow = true;
    }
    goto CHECK_OVERFLOW;

BASE_ANY_WITH_OVERFLOW:
    valLimit = kPositiveValueLimit;
    charLimit = 0;
    for (; chars; ++ptr, --chars) {
        ch = *ptr;
        if (ch <= '9') {
            num = ch - '0';
        } else if (ch <= 'Z') {
            // ch must be unsigned so that it fails with a too large positive
            // number when ch - 'A' is negative.
            static_assert(is_unsigned_v<decltype(ch)>);
            num = ch - 'A' + 10;
        } else if (ch <= 'z') {
            num = ch - 'a' + 10;
        } else {
            break;
        }
        if (num >= radix)
            break;
        auto next = radix * val + num;
        if (next > val && next <= valLimit) {
            val = next;
            continue;
        }
        if (next == 0 && val == 0)
            continue;
        overflow = true;
    }
    goto CHECK_OVERFLOW;

CHECK_OVERFLOW:
    if constexpr ((Flags & fSigned) != 0) {
        if (negate) {
            if constexpr ((Flags & f64Bit) != 0) {
                valLimit = (uint64_t)-INT64_MIN;
            } else {
                valLimit = (unsigned)-INT_MIN;
            }
        }
    }
    if (overflow || val > valLimit) {
        val = valLimit;
        errno = ERANGE;
        goto RETURN_VALUE;
    }
    goto CHECK_NUMBER;

BASE_16:
    if (chars > kMaxCharsBase16) {
        charLimit = chars;
        chars = kMaxCharsBase16;
    } else {
        charLimit = 0;
    }
    for (; chars; ++ptr, --chars) {
        ch = *ptr;
        num = ch - '0';
        if (num > 9) {
            num = ch - 'A';
            if (num > 'f' - 'A')
                goto CHECK_NUMBER;
            num = num % ('a' - 'A') + 10;
            if (num > 15)
                goto CHECK_NUMBER;
        }
        val = (val << 4) + num;
    }
    if (charLimit) {
        if (!*ptr)
            goto RETURN_VALUE;
        chars = charLimit - kMaxCharsBase16;
        goto BASE_ANY_WITH_OVERFLOW;
    }
    goto CHECK_NUMBER;

BASE_10:
    if (chars > kMaxSafeCharsBase10) {
        charLimit = chars;
        chars = kMaxSafeCharsBase10;
        for (; chars; ++ptr, --chars) {
            num = (unsigned char) *ptr - '0';
            if (num > 9)
                goto CHECK_NUMBER;
            val = 10 * val + num;
        }
        chars = charLimit - kMaxSafeCharsBase10;
        goto BASE_LE10_WITH_OVERFLOW;
    }
    charLimit = 0;
    for (; chars; ++ptr, --chars) {
        num = (unsigned char) *ptr - '0';
        if (num > 9)
            goto CHECK_NUMBER;
        val = 10 * val + num;
    }
    goto CHECK_NUMBER;

CHECK_NUMBER:
    // no number found? check for leading whitespace and sign
    if (ptr == base) {
        if (ptr != source) {
            // advanced past leading space, prefix, etc, but no digits.
            // reset ptr to source and return 0
            ptr = source;
            goto RETURN_VALUE;
        }
        if (charLimit)
            chars = charLimit;
        for (; chars; --chars, ++ptr) {
            switch (*ptr) {
                case ' ':   // space
                case '\t':  // tab
                case '\n':  // newline
                case '\v':  // vertical tab
                case '\f':  // page feed
                case '\r':  // carriage return
                    // whitespace
                    continue;
                case '-':
                    negate = true;
                    [[fallthrough]];
                case '+':
                    chars -= 1;
                    ptr += 1;
            }
            break;
        }
        // found something to skip? try the number again
        if (ptr != base)
            goto PARSE_NUMBER;
    }

RETURN_VALUE:
    if (endPtr)
        *endPtr = const_cast<char *>(ptr);
    if (negate)
        return (Flags & f64Bit) ? -(int64_t)val : (uint64_t)-(int)val;
    return val;
}
#pragma warning(pop)

//===========================================================================
int Dim::strToInt(const char src[], char ** eptr, int base) {
    return (int) iStrToAny<kInt>(src, eptr, base);
}

//===========================================================================
unsigned Dim::strToUint(const char src[], char ** eptr, int base) {
    return (unsigned) iStrToAny<kUint>(src, eptr, base);
}

//===========================================================================
int64_t Dim::strToInt64(const char src[], char ** eptr, int base) {
    return (int64_t) iStrToAny<kInt64>(src, eptr, base);
}

//===========================================================================
uint64_t Dim::strToUint64(const char src[], char ** eptr, int base) {
    return iStrToAny<kUint64>(src, eptr, base);
}

//===========================================================================
int Dim::strToInt(string_view src, char ** eptr, int base) {
    return (int) iStrToAny<kInt>(src.data(), eptr, base, src.size());
}

//===========================================================================
unsigned Dim::strToUint(string_view src, char ** eptr, int base) {
    return (unsigned) iStrToAny<kUint>(src.data(), eptr, base, src.size());
}

//===========================================================================
int64_t Dim::strToInt64(string_view src, char ** eptr, int base) {
    return (int64_t) iStrToAny<kInt64>(src.data(), eptr, base, src.size());
}

//===========================================================================
uint64_t Dim::strToUint64(string_view src, char ** eptr, int base) {
    return iStrToAny<kUint64>(src.data(), eptr, base, src.size());
}


/****************************************************************************
*
*   Parse string into arbitrary type
*
***/

//===========================================================================
bool Dim::parse(double * out, std::string_view src) {
    char buf[maxFloatChars<double>()];
    buf[src.copy(buf, src.size())] = 0;
    char * ptr;
    *out = strtod(buf, &ptr);
    switch (*ptr) {
    case 'k': case 'K':
        *out *= (ptr[1] == 'i') ? 1024 : 1000;
        break;
    case 'M':
        *out *= (ptr[1] == 'i') ? 1024 * 1024 : 1'000'000;
        break;
    case 'G':
        *out *= (ptr[1] == 'i') ? 1024 * 1024 * 1024 : 1'000'000'000;
        break;
    case 'T':
        *out *= (ptr[1] == 'i')
            ? INT64_C(1024) * 1024 * 1024 * 1024
            : 1'000'000'000'000;
        break;
    case 'P':
        *out *= (ptr[1] == 'i')
            ? INT64_C(1024) * 1024 * 1024 * 1024 * 1024
            : 1'000'000'000'000;
        break;
    }
    return true;
}


/****************************************************************************
*
*   String utilities
*
***/

//===========================================================================
void Dim::split(
    vector<string_view> * out,
    string_view src,
    char sep
) {
    out->clear();
    auto ptr = src.data();
    auto count = src.size();
    for (;;) {
        if (auto eptr = (const char *) memchr(ptr, sep, count)) {
            auto segLen = size_t(eptr - ptr);
            out->push_back(string_view{ptr, segLen});
            ptr = eptr + 1;
            count -= segLen + 1;
        } else {
            out->push_back(string_view{ptr, count});
            break;
        }
    }
}

//===========================================================================
string_view Dim::trim(string_view src) {
    const char * first = src.data();
    const char * last = first + src.size();
    while (first < last && isspace(*first))
        ++first;
    for (; first < last; --last) {
        if (!isspace(last[-1]))
            break;
    }
    return {first, size_t(last - first)};
}

//===========================================================================
string_view Dim::ltrim(std::string_view src) {
    const char * first = src.data();
    const char * last = first + src.size();
    while (first < last && isspace(*first))
        ++first;
    return {first, size_t(last - first)};
}

//===========================================================================
string_view Dim::rtrim(std::string_view src) {
    const char * first = src.data();
    const char * last = first + src.size();
    for (; first < last; --last) {
        if (!isspace(last[-1]))
            break;
    }
    return {first, size_t(last - first)};
}

//===========================================================================
string Dim::trimBlock(string_view src) {
    vector<string_view> lines;
    split(&lines, src, '\n');
    // remove trailing whitespace
    for (auto && line : lines)
        line = rtrim(line);

    // remove leading and trailing blank lines
    while (!lines.empty() && lines.back().empty())
        lines.pop_back();
    while (!lines.empty() && lines.front().empty())
        lines.erase(lines.begin());
    if (lines.empty())
        return {};

    // remove common leading whitespace
    auto prefix = lines.front().size();
    for (auto && line : lines)
        prefix = min(prefix, line.size() - ltrim(line).size());
    if (prefix) {
        for (auto && line : lines)
            line.remove_prefix(prefix);
    }

    // combined lines into new string
    auto out = string(lines.front());
    for (auto i = lines.begin() + 1; i != lines.end(); ++i) {
        out += '\n';
        out += *i;
    }
    return out;
}

//===========================================================================
unique_ptr<char[]> Dim::strDup(string_view src) {
    auto ptr = unique_ptr<char[]>(new char[src.size() + 1]);
    memcpy(ptr.get(), src.data(), src.size());
    ptr[src.size()] = 0;
    return move(ptr);
}


/****************************************************************************
*
*   Unicode
*
***/

//===========================================================================
// Wikipedia article on byte order marks:
//   https://en.wikipedia.org/wiki/Byte_order_mark#Representations_of_byte_order_marks_by_encoding
UtfType Dim::utfBomType(const char bytes[], size_t count) {
    if (count >= 2) {
        switch (bytes[0]) {
        case 0:
            if (count >= 4 && bytes[1] == 0 && bytes[2] == '\xfe'
                && bytes[3] == '\xff') {
                return kUtf32BE;
            }
            break;
        case '\xef':
            if (count >= 3 && bytes[1] == '\xbb' && bytes[2] == '\xbf')
                return kUtf8;
            break;
        case '\xfe':
            if (bytes[1] == '\xff')
                return kUtf16BE;
            break;
        case '\xff':
            if (bytes[1] == '\xfe') {
                if (count >= 4 && bytes[2] == 0 && bytes[3] == 0)
                    return kUtf32LE;
                return kUtf16LE;
            }
            break;
        }
    }
    return kUtfUnknown;
}


/****************************************************************************
*
*   UTF-8
*
***/

//===========================================================================
char32_t Dim::popFrontUnicode(string_view * src) {
    if (src->empty())
        return 0;
    unsigned char ch = src->front();
    if (ch < 0x80) {
        src->remove_prefix(1);
        return ch;
    }
    uint32_t out;
    unsigned num;
    if (ch < 0xe0) {
        if (ch < 0xc2)
            return 0;
        num = 2;
        out = ch & 0x1f;
    } else if (ch < 0xf0) {
        num = 3;
        out = ch & 0x0f;
    } else if (ch < 0xf5) {
        num = 4;
        out = ch & 0x07;
    } else {
        return 0;
    }
    if (src->size() < num)
        return 0;
    for (unsigned i = 1; i < num; ++i) {
        ch = (*src)[i];
        if ((ch & 0xc0) != 0x80)
            return 0;
        out = (out << 6) + (unsigned char) (ch & 0x3f);
    }
    if (out >= 0xd800 && (out < 0xe000 || out > 0x10ffff))
        return 0;
    src->remove_prefix(num);
    return out;
}

//===========================================================================
size_t Dim::unicodeLen(string_view src) {
    size_t len = 0;
    for (unsigned char ch : src) {
        if (!ch)
            break;
        if ((ch & 0xc0) != 0x80)
            len += 1;
    }
    return len;
}

//===========================================================================
string Dim::toString(wstring_view src) {
    string out;
    copy_char(src, back_inserter(out));
    return out;
}

//===========================================================================
ostream & Dim::operator<<(ostream & os, const ostream_utf8_return & out) {
    copy_char(out.src, ostreambuf_iterator<char>(os));
    return os;
}


/****************************************************************************
*
*   UTF-16
*
***/

//===========================================================================
char32_t Dim::popFrontUnicode(wstring_view * src) {
    if (src->empty())
        return 0;
    char32_t out = src->front();
    if (out < 0xd800 || out > 0xdfff) {
        src->remove_prefix(1);
        return out;
    }
    // surrogate pair, first contains high 10 bits encoded as d800 - dbff,
    // second contains low 10 bits encoded as dc00 - dfff.
    if (src->size() < 2 || out >= 0xdc00
        || (*src)[1] < 0xdc00 || (*src)[1] > 0xdfff
    ) {
        return 0;
    }
    out = ((out - 0xd800) << 10) + ((*src)[1] - 0xdc00);
    src->remove_prefix(2);
    return out;
}

//===========================================================================
size_t Dim::unicodeLen(wstring_view src) {
    size_t len = 0;
    for (auto && ch : src) {
        if (!ch)
            break;
        if ((ch & 0xfc00) != 0xdc00)
            len += 1;
    }
    return len;
}

//===========================================================================
wstring Dim::toWstring(string_view src) {
    wstring out;
    copy_wchar(src, back_inserter(out));
    return out;
}
