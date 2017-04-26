// Copyright Glen Knowles 2017.
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
    kSigned   = 1,
    k64Bit    = 2,

    kUint     = 0,
    kUint64   = k64Bit,
    kInt      = kSigned,
    kInt64    = kSigned | k64Bit,
};
} // namespace

//===========================================================================
static bool isHex(unsigned char ch) {
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
#pragma warning(disable : 4127) // conditional expression is constant
template<int Flags>
static uint64_t iStrToAny (
    const char source[], 
    char ** endPtr, 
    unsigned radix,
    size_t chars = (size_t) -1
) {
    const char * ptr = source;
    const char * base;
    bool negate = false;
    bool overflow = false;

    unsigned char ch;
    unsigned num;
    uint64_t val = 0;
    uint64_t shiftLimit;
    uint64_t valLimit;
    size_t charLimit = 0;

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
    valLimit = (Flags & k64Bit) ? UINT64_MAX : UINT_MAX;
    shiftLimit = valLimit / radix;
    charLimit = 0;
    for (; chars; ++ptr, --chars) {
        num = unsigned char(*ptr) - '0';
        if (num >= radix)
            break;
        if (val > shiftLimit) 
            overflow = true;
        val = radix * val;
        if (val > valLimit - num)
            overflow = true;
        val += num;
    }
    goto CHECK_OVERFLOW;

BASE_ANY_WITH_OVERFLOW:
    valLimit = (Flags & k64Bit) ? UINT64_MAX : UINT_MAX;
    shiftLimit = valLimit / radix;
    charLimit = 0;
    for (; chars; ++ptr, --chars) {
        ch = *ptr;
        if (ch <= '9') {
            num = ch - '0';
        } else if (ch <= 'Z') {
            // ensure a too large positive number when ch - 'A' is negative.
            num = unsigned char(ch - 'A') + 10;
        } else if (ch <= 'z') {
            num = unsigned char(ch - 'a') + 10;
        } else {
            break;
        }
        if (num >= radix)
            break;

        if (val > shiftLimit) 
            overflow = true;
        val = radix * val;
        if (val > valLimit - num)
            overflow = true;
        val += num;
    }
    goto CHECK_OVERFLOW;

CHECK_OVERFLOW:
    if (!overflow) {
        if (Flags & kSigned) {
            if (Flags & k64Bit) {
                valLimit = negate ? (uint64_t)-INT64_MIN : INT64_MAX;
            } else {
                valLimit = negate ? (unsigned)-INT_MIN : INT_MAX;
            }
            if (val > valLimit) 
                goto OVERFLOW_VALUE;
        }
    } else {
    OVERFLOW_VALUE:
        val = valLimit;
        errno = ERANGE;
        goto RETURN_VALUE;
    }
    goto CHECK_NUMBER;

BASE_16:
    constexpr unsigned kMaxCharsBase16 = (Flags & k64Bit) ? 16 : 8;
    if (chars > kMaxCharsBase16) {
        charLimit = chars;
        chars = kMaxCharsBase16;
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
        if (!isHex(*ptr))
            goto RETURN_VALUE;
        chars = charLimit - kMaxCharsBase16;
        goto BASE_ANY_WITH_OVERFLOW;
    }
    goto CHECK_NUMBER;

BASE_10:
    // the high order partial digit (and the trailing null :P) are not safe
    constexpr unsigned kMaxSafeCharsBase10 = (Flags & k64Bit) 
        ? maxIntegralChars<uint64_t>() - 1 
        : maxIntegralChars<unsigned>() - 1;
    if (chars > kMaxSafeCharsBase10) {
        charLimit = chars;
        chars = kMaxSafeCharsBase10;
    }
    for (; chars; ++ptr, --chars) {
        num = unsigned char(*ptr) - '0';
        if (num > 9)
            goto CHECK_NUMBER;
        val = 10 * val + num;
    }
    if (charLimit) {
        chars = charLimit - kMaxSafeCharsBase10;
        goto BASE_LE10_WITH_OVERFLOW;
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
    if (negate) {
        return (Flags & k64Bit) 
            ? -(int64_t)val 
            : (uint64_t)-(int)val;
    }
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
