// Copyright Glen Knowles 2017.
// Distributed under the Boost Software License, Version 1.0.
//
// path.h - dim file
#pragma once

#include "cppconf/cppconf.h"

#include <cassert>
#include <cstdint>
#include <experimental/filesystem>

namespace Dim {


/****************************************************************************
*
*   Path
*
***/

class Path {
public:
    Path(std::string_view from);
    Path(const std::experimental::filesystem::path & from);

    std::experimental::filesystem::path fsPath() const;
    std::string_view view() const;
private:
    // intentionally not implemented
    operator std::experimental::filesystem::path();

    std::string m_data;
};

} // namespace


/****************************************************************************
*
*   filesystem::path
*
***/

//===========================================================================
template <>
inline std::experimental::filesystem::path::path(const std::string & from) {
    *this = u8path(from);
}

//===========================================================================
template <>
inline std::experimental::filesystem::path::path(
    const char * first, 
    const char * last
) {
    *this = u8path(first, last);
}
