// Copyright Glen Knowles 2026.
// Distributed under the Boost Software License, Version 1.0.
//
// varsubst.h - dim basic
#pragma once

#include "cppconf/cppconf.h"

#include <cassert>
#include <string>
#include <string_view>
#include <unordered_map>

namespace Dim {

/****************************************************************************
*
*   Variable Substitution
*
*   Replace references in string with variable values.
*
***/


std::string varSubst(
    const std::string & str,
    const std::unordered_map<std::string, std::string> & vars
    std::string_view prefix,
    std::string_view suffix,
    unsigned flags = 0
);

} // namespace
