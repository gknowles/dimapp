// Copyright Glen Knowles 2017.
// Distributed under the Boost Software License, Version 1.0.
//
// threadint.h - dim core
#pragma once

#include <string_view>

namespace Dim {

// Thread
void iThreadInitialize();

void iThreadSetName(std::string_view name);
std::string iThreadGetName();

} // namespace
