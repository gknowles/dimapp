// Copyright Glen Knowles 2020 - 2022.
// Distributed under the Boost Software License, Version 1.0.
//
// guid.h - dim system
#pragma once

#include "cppconf/cppconf.h"

#include <cstdint>
#include <string>
#include <string_view>

namespace Dim {


/****************************************************************************
*
*   Guid
*
***/

struct Guid {
    std::uint32_t data1 = 0;
    std::uint16_t data2 = 0;
    std::uint16_t data3 = 0;
    std::uint8_t data4[8] = {};

    explicit operator bool() const { return *this != Guid{}; }
    bool operator==(const Guid & other) const {
        return memcmp(this, &other, sizeof(*this)) == 0;
    }
};

Guid newGuid();

std::string toString(const Guid & val);
[[nodiscard]] bool parse(Guid * out, std::string_view val);

constexpr Guid operator ""_Guid (const char str[], size_t len);

//===========================================================================
constexpr Guid strToGuid(std::string_view val) {
    Guid out;
    if (val.size() != 36
        || val[8] != '-'
        || val[13] != '-'
        || val[18] != '-'
        || val[23] != '-'
    ) {
        if (std::is_constant_evaluated()) {
            // Invalid Guid
            toString(out);
        }
        return {};
    }

    auto ptr = val.data();
    if ((bool) std::from_chars(ptr, ptr + 8, out.data1, 16).ec
        || (bool) std::from_chars(ptr + 9, ptr + 13, out.data2, 16).ec
        || (bool) std::from_chars(ptr + 14, ptr + 18, out.data3, 16).ec
        || (bool) std::from_chars(ptr + 19, ptr + 21, out.data4[0], 16).ec
        || (bool) std::from_chars(ptr + 21, ptr + 23, out.data4[1], 16).ec
        || (bool) std::from_chars(ptr + 24, ptr + 26, out.data4[2], 16).ec
        || (bool) std::from_chars(ptr + 26, ptr + 28, out.data4[3], 16).ec
        || (bool) std::from_chars(ptr + 28, ptr + 30, out.data4[4], 16).ec
        || (bool) std::from_chars(ptr + 30, ptr + 32, out.data4[5], 16).ec
        || (bool) std::from_chars(ptr + 32, ptr + 34, out.data4[6], 16).ec
        || (bool) std::from_chars(ptr + 34, ptr + 36, out.data4[7], 16).ec
    ) {
        if (std::is_constant_evaluated()) {
            // Invalid Guid
            toString(out);
        }
        return {};
    }
    out.data1 = ntoh32(out.data1);
    out.data2 = ntoh16(out.data2);
    out.data3 = ntoh16(out.data3);

    return out;
}

//===========================================================================
constexpr Guid operator ""_Guid (const char str[], size_t len) {
    return strToGuid(std::string_view(str, len));
}

} // namespace
