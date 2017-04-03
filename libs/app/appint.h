// Copyright Glen Knowles 2017.
// Distributed under the Boost Software License, Version 1.0.
//
// appint.h - dim app
//
// Initialize and destroy methods called by appRun()

#pragma once

#include <string_view>

namespace Dim {

// AppConfig
void iAppConfigInitialize(std::string_view dir);

} // namespace
