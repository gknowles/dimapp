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

Guid strToGuid(std::string_view val);
std::string toString(const Guid & val);
[[nodiscard]] bool parse(Guid * out, std::string_view val);

} // namespace
