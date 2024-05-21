// Copyright Glen Knowles 2016 - 2024.
// Distributed under the Boost Software License, Version 1.0.
//
// hex.h - dim core
#pragma once

#include "cppconf/cppconf.h"

#include <string>
#include <string_view>

namespace Dim {


/****************************************************************************
*
*   Hex conversions
*
***/

//===========================================================================
constexpr bool isHex(unsigned char ch) {
    // return ch - '0' <= 9 || (ch | 0x20) - 'a' <= 5;

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
// Converts hex character (0-9, a-f, A-F) to unsigned (0-15), other characters
// produce random garbage.
constexpr unsigned hexToNibbleUnsafe(char ch) {
    return ((ch | 432) * 239'217'992 & 0xffff'ffff) >> 28;
}

//===========================================================================
constexpr char hexFromNibble(unsigned val) {
    assert(val < 16);
    const char s_chars[] = "0123456789abcdef";
    return s_chars[val];
}

// On invalid input these return 16 and 256 respectively
unsigned hexToNibble(unsigned char val);
unsigned hexToByte(unsigned char high, unsigned char low);

bool hexToBytes(std::string & out, std::string_view src, bool append);
void hexFromBytes(std::string & out, std::string_view src, bool append);

std::ostream & hexByte(std::ostream & os, char data);
std::ostream & hexDumpLine(std::ostream & os, std::string_view data, size_t pos);
std::ostream & hexDump(std::ostream & os, std::string_view data);

} // namespace
