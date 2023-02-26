// Copyright Glen Knowles 2017 - 2023.
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

//===========================================================================
inline unsigned char charToDigit (unsigned char ch) {
    static constexpr unsigned char kCharToDigit[] = {
    //    0   1   2   3    4   5   6   7    8   9   a   b    c   d   e    f
        255,255,255,255, 255,255,255,255, 255,255,255,255, 255,255,255,255,
        255,255,255,255, 255,255,255,255, 255,255,255,255, 255,255,255,255,
        255,255,255,255, 255,255,255,255, 255,255,255,255, 255,255,255,255,
          0,  1,  2,  3,   4,  5,  6,  7,   8,  9,255,255, 255,255,255,255,
        255, 10, 11, 12,  13, 14, 15, 16,  17, 18, 19, 20,  21, 22, 23, 24,
         25, 26, 27, 28,  29, 30, 31, 32,  33, 34, 35,255, 255,255,255,255,
        255, 10, 11, 12,  13, 14, 15, 16,  17, 18, 19, 20,  21, 22, 23, 24,
         25, 26, 27, 28,  29, 30, 31, 32,  33, 34, 35,255, 255,255,255,255,
        255,255,255,255, 255,255,255,255, 255,255,255,255, 255,255,255,255,
        255,255,255,255, 255,255,255,255, 255,255,255,255, 255,255,255,255,
        255,255,255,255, 255,255,255,255, 255,255,255,255, 255,255,255,255,
        255,255,255,255, 255,255,255,255, 255,255,255,255, 255,255,255,255,
        255,255,255,255, 255,255,255,255, 255,255,255,255, 255,255,255,255,
        255,255,255,255, 255,255,255,255, 255,255,255,255, 255,255,255,255,
        255,255,255,255, 255,255,255,255, 255,255,255,255, 255,255,255,255,
        255,255,255,255, 255,255,255,255, 255,255,255,255, 255,255,255,255,
    };
    static_assert(std::size(kCharToDigit) == 256);

    return kCharToDigit[ch];
}

//===========================================================================
// There is no wchar version because the internal wchar implementation:
//  - isn't quite as bad because it doesn't deal with the C locale settings
//  - recognizes multiple code points for the digits 0-9
//
//  chars > MAX_INT is treated as MAX_INT
template<typename T>
requires (std::is_integral_v<T> && !std::is_same_v<T, bool>)
static T strToIntegral (
    const char source[],
    char ** endPtr,
    unsigned radix,
    size_t chars = (size_t) -1
) {
    // the high order partial digit (and the trailing null :P) are not safe
    constexpr unsigned kMaxSafeCharsBase10 = std::numeric_limits<T>::digits10;
    constexpr unsigned kMaxCharsBase16 = 2 * sizeof(T);

    constexpr uint64_t kPositiveValueLimit = std::numeric_limits<T>::max();

    const char * ptr = source;
    const char * base = ptr;
    const char * eptr = (chars > std::numeric_limits<int>::max())
        ? base + std::numeric_limits<int>::max()
        : base + chars;
    [[maybe_unused]] bool negate;
    if constexpr (std::is_signed_v<T>)
        negate = false;

    unsigned num;
    uint64_t val = 0;

PARSE_NUMBER:
    if (radix == 10) {
        goto BASE_10;
    } else if (radix == 16) {
        if (chars > 1
            && ptr[0] == '0'
            && ptr[1] == 'x'
        ) {
            ptr += 2;
            base = ptr;
        }
        goto BASE_16;
    } else if (!radix) {
        if (chars < 2 || ptr[0] != '0') {
            radix = 10;
            goto BASE_10;
        } else if (ptr[1] == 'x') {
            radix = 16;
            ptr += 2;
            base = ptr;
            goto BASE_16;
        } else {
            radix = 8;
            ptr += 1;
            base = ptr;
            goto BASE_ANY_WITH_OVERFLOW;
        }
    } else {
        if (radix >= 2 && radix <= 36)
            goto BASE_ANY_WITH_OVERFLOW;
        // invalid base, reset pointer to source and return 0
        ptr = source;
        goto RETURN_VALUE;
    }

BASE_10:
    if (auto limitPtr = ptr + kMaxSafeCharsBase10; eptr > limitPtr) {
        for (;;) {
            num = (unsigned char) *ptr - '0';
            if (num > 9)
                goto CHECK_NUMBER;
            val = 10 * val + num;
            if (++ptr == limitPtr)
                break;
        }
        goto BASE_ANY_WITH_OVERFLOW;
    }
    for (; ptr != eptr; ++ptr) {
        num = (unsigned char) *ptr - '0';
        if (num > 9)
            goto CHECK_NUMBER;
        val = 10 * val + num;
    }
    goto CHECK_NUMBER;

BASE_16:
    if (auto limitPtr = ptr + kMaxCharsBase16; eptr > limitPtr) {
        for (;;) {
            num = charToDigit(*ptr);
            if (num > 15)
                goto CHECK_NUMBER;
            val = 16 * val + num;
            if (++ptr == limitPtr)
                break;
        }
        goto BASE_ANY_WITH_OVERFLOW;
    }
    for (; ptr != eptr; ++ptr) {
        num = charToDigit(*ptr);
        if (num > 15)
            goto CHECK_NUMBER;
        val = 16 * val + num;
    }
    goto CHECK_NUMBER;

BASE_ANY_WITH_OVERFLOW:
    for (; ptr != eptr; ++ptr) {
        num = charToDigit(*ptr);
        if (num >= radix)
            break;
        auto next = radix * val + num;
        if (next > val && next <= kPositiveValueLimit) {
            val = next;
            continue;
        }
        if (next == 0 && val == 0)
            continue;

        // overflow, skip remaining digits
        while (++ptr != eptr) {
            num = charToDigit(*ptr);
            if (num >= radix)
                break;
        }
        if constexpr (std::is_signed_v<T>) {
            val = negate
                ? -std::numeric_limits<T>::min()
                : kPositiveValueLimit;
        } else {
            val = kPositiveValueLimit;
        }
        errno = ERANGE;
        goto RETURN_VALUE;
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
        for (; ptr != eptr; ++ptr) {
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
                    if constexpr (std::is_signed_v<T>)
                        negate = true;
                    [[fallthrough]];
                case '+':
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
    if constexpr (std::is_signed_v<T>) {
        if (negate)
            return 0 - static_cast<T>(val);
    }
    return static_cast<T>(val);
}

//===========================================================================
int Dim::strToInt(const char src[], char ** eptr, int base) {
    return strToIntegral<int>(src, eptr, base);
}

//===========================================================================
unsigned Dim::strToUint(const char src[], char ** eptr, int base) {
    return strToIntegral<unsigned>(src, eptr, base);
}

//===========================================================================
int64_t Dim::strToInt64(const char src[], char ** eptr, int base) {
    return strToIntegral<int64_t>(src, eptr, base);
}

//===========================================================================
uint64_t Dim::strToUint64(const char src[], char ** eptr, int base) {
    return strToIntegral<uint64_t>(src, eptr, base);
}

//===========================================================================
int Dim::strToInt(string_view src, char ** eptr, int base) {
    return strToIntegral<int>(src.data(), eptr, base, src.size());
}

//===========================================================================
unsigned Dim::strToUint(string_view src, char ** eptr, int base) {
    return strToIntegral<unsigned>(src.data(), eptr, base, src.size());
}

//===========================================================================
int64_t Dim::strToInt64(string_view src, char ** eptr, int base) {
    return strToIntegral<int64_t>(src.data(), eptr, base, src.size());
}

//===========================================================================
uint64_t Dim::strToUint64(string_view src, char ** eptr, int base) {
    return strToIntegral<uint64_t>(src.data(), eptr, base, src.size());
}


/****************************************************************************
*
*   String to float
*
***/

//===========================================================================
static double siFactor(char ** ptr, double binary, double metric) {
    *ptr += 1;
    if (**ptr == 'i') {
        *ptr += 1;
        return binary;
    } else {
        return metric;
    }
}

//===========================================================================
bool Dim::parse(double * out, std::string_view src) {
    char buf[maxNumericChars<double>() + 1];
    buf[src.copy(buf, size(buf) - 1)] = 0;
    *out = 0;
    auto r = from_chars(src.data(), src.data() + src.size(), *out);
    char * ptr = const_cast<char *>(r.ptr);
    switch (*ptr) {
    case 'k': case 'K':
        *out *= siFactor(&ptr, 1024, 1000);
        break;
    case 'M':
        *out *= siFactor(&ptr, 1024 * 1024, 1'000'000);
        break;
    case 'G':
        *out *= siFactor(&ptr, 1024 * 1024 * 1024, 1'000'000'000);
        break;
    case 'T':
        *out *= siFactor(
            &ptr,
            INT64_C(1024) * 1024 * 1024 * 1024,
            1'000'000'000'000
        );
        break;
    case 'P':
        *out *= siFactor(
            &ptr,
            INT64_C(1024) * 1024 * 1024 * 1024 * 1024,
            1'000'000'000'000'000
        );
        break;
    }
    return !*ptr;
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
void Dim::split(
    vector<string_view> * out,
    string_view src,
    string_view seps
) {
    out->clear();
    size_t pos = 0;
    size_t epos = src.size();
    while (pos < src.size()) {
        epos = src.find_first_of(seps, pos);
        out->push_back(src.substr(pos, epos - pos));
        pos = src.find_first_not_of(seps, epos);
    }
    if (!pos || epos < src.size())
        out->push_back({});
}

//===========================================================================
void Dim::split(
    vector<string_view> * out,
    string_view src,
    string_view seps,
    size_t maxLen
) {
    out->clear();
    size_t pos = 0;
    size_t epos = src.size();
    while (pos < src.size()) {
        epos = src.find_first_of(seps, pos);
        if (epos == string::npos)
            epos = src.size();
        auto len = epos - pos;
        for (; len > maxLen; len -= maxLen, pos += maxLen) {
            out->push_back(src.substr(pos, maxLen));
        }
        out->push_back(src.substr(pos, len));
        pos = src.find_first_not_of(seps, epos);
    }
    if (!pos || epos < src.size())
        out->push_back({});
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
    for (auto && line : lines) {
        if (line.size())
            prefix = min(prefix, line.size() - ltrim(line).size());
    }
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
    auto ptr = make_unique<char[]>(src.size() + 1);
    memcpy(ptr.get(), src.data(), src.size());
    ptr[src.size()] = 0;
    return ptr;
}

//===========================================================================
std::unique_ptr<char[]> Dim::strDupGather(
    std::string_view * srcs[],
    size_t count
) {
    size_t len = accumulate(
        srcs,
        srcs + count,
        (size_t) 0,
        [](const auto & a, auto & b) { return a + b->size() + 1; }
    );
    auto out = make_unique<char[]>(len);
    auto ptr = out.get();
    for (auto v = srcs; count; ++v, --count) {
        auto vlen = (*v)->size();
        memcpy(ptr, (*v)->data(), vlen);
        **v = {ptr, vlen};
        ptr += vlen;
        *ptr++ = 0;
    }
    return out;
}


/****************************************************************************
*
*   strCopy
*
***/

//===========================================================================
template<typename T>
static size_t strCopy(T * out, size_t outLen, const T * src, size_t srcLen) {
    auto cnt = min(outLen, srcLen);
    memcpy(out, src, cnt * sizeof *out);
    out[cnt] = 0;
    return cnt;
}

//===========================================================================
size_t Dim::strCopy(char * out, size_t outLen, std::string_view src) {
    return ::strCopy(out, outLen, src.data(), src.size());
}

//===========================================================================
size_t Dim::strCopy(wchar_t * out, size_t outLen, std::wstring_view src) {
    return ::strCopy(out, outLen, src.data(), src.size());
}

namespace {

template<typename T>
class PtrOutIter {
public:
    using iterator_category = output_iterator_tag;
    using value_type = void;
    using pointer = void;
    using reference = void;
    using difference_type = ptrdiff_t;

public:
    constexpr PtrOutIter() = default;
    PtrOutIter(T * out, size_t outLen) noexcept
        : m_cur(out)
        , m_end(out + outLen)
    {}
    PtrOutIter & operator* () noexcept { return *this; }
    PtrOutIter & operator= (T val) {
        if (m_cur < m_end)
            *m_cur++ = val;
        return *this;
    }
    PtrOutIter & operator++ () noexcept { return *this; }
    PtrOutIter operator++ (int) noexcept { return *this; }

    T * ptr() const { return m_cur; }
    T * eptr() const { return m_end; }

private:
    T * m_cur = {};
    T * m_end = {};
};

} // namespace

//===========================================================================
size_t Dim::strCopy(wchar_t * rawOut, size_t outLen, std::string_view src) {
    assert(outLen >= 1); // Must have room for terminating null.
    auto base = (wchar_t *) rawOut;
    auto ptr = base;
    auto last = base + outLen - 1;
    while (last - ptr >= 2) {
        auto cp = popFrontUnicode(&src);
        if (!cp) {
            *ptr = 0;
            return ptr - base;
        }
        ptr = writeWchar(ptr, cp);
    }
    wchar_t tmpBuf[4];
    auto tptr = tmpBuf;
    auto tlast = tmpBuf + (last - ptr);
    while (auto cp = popFrontUnicode(&src)) {
        auto tnext = writeWchar(tptr, cp);
        if (tnext <= tlast)
            tptr = tnext;
        if (tnext >= tlast)
            break;
    }
    if (auto cnt = tptr - tmpBuf; cnt) {
        memcpy(ptr, tmpBuf, cnt * sizeof *ptr);
        ptr += cnt;
    }
    *ptr = 0;
    return ptr - base;
}

//===========================================================================
size_t Dim::strCopy(char * rawOut, size_t outLen, std::wstring_view src) {
    assert(outLen >= 1); // Must have room for terminating null.
    auto base = (unsigned char *) rawOut;
    auto ptr = base;
    auto last = base + outLen - 1;
    while (last - ptr >= 4) {
        auto cp = popFrontUnicode(&src);
        if (!cp) {
            *ptr = 0;
            return ptr - base;
        }
        ptr = writeUtf8(ptr, cp);
    }
    unsigned char tmpBuf[8];
    auto tptr = tmpBuf;
    auto tlast = tmpBuf + (last - ptr);
    while (auto cp = popFrontUnicode(&src)) {
        auto tnext = writeUtf8(tptr, cp);
        if (tnext <= tlast)
            tptr = tnext;
        if (tnext >= tlast)
            break;
    }
    if (auto cnt = tptr - tmpBuf; cnt) {
        memcpy(ptr, tmpBuf, cnt * sizeof *ptr);
        ptr += cnt;
    }
    *ptr = 0;
    return ptr - base;
}


/****************************************************************************
*
*   Lowercase and Uppercase Conversions
*
***/

//===========================================================================
char * Dim::toLower(char src[]) {
    auto & f = use_facet<ctype<char>>(locale());
    for (auto ptr = src; *ptr; ++ptr)
        *ptr = f.tolower(*ptr);
    return src;
}

//===========================================================================
char * Dim::toUpper(char src[]) {
    auto & f = use_facet<ctype<char>>(locale());
    for (auto ptr = src; *ptr; ++ptr)
        *ptr = f.toupper(*ptr);
    return src;
}

//===========================================================================
char * Dim::toLower(char src[], size_t srclen) {
    auto & f = use_facet<ctype<char>>(locale());
    f.tolower(src, src + srclen);
    return src;
}

//===========================================================================
char * Dim::toUpper(char src[], size_t srclen) {
    auto & f = use_facet<ctype<char>>(locale());
    f.toupper(src, src + srclen);
    return src;
}

//===========================================================================
string Dim::toLower(string_view src) {
    string out(src);
    toLower(out.data());
    return out;
}

//===========================================================================
string Dim::toUpper(string_view src) {
    string out(src);
    toUpper(out.data());
    return out;
}


/****************************************************************************
*
*   Unicode
*
***/

//===========================================================================
// Wikipedia article on byte order marks:
//  https://tinyurl.com/ep3zfrj6
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
    writeUtf8(back_inserter(out), src);
    return out;
}

//===========================================================================
namespace Dim {
ostream & operator<<(ostream & os, const ostream_utf8_return & out);
}
ostream & Dim::operator<<(ostream & os, const ostream_utf8_return & out) {
    writeUtf8(ostreambuf_iterator<char>(os), out.src);
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
    writeWchar(back_inserter(out), src);
    return out;
}
