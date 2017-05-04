// Copyright Glen Knowles 2017.
// Distributed under the Boost Software License, Version 1.0.
//
// appint.h - dim app
//
// Initialize and destroy methods called by appRun()

#pragma once

#include <string_view>

namespace Dim {

// Config
void iConfigInitialize(std::string_view dir);

// LogFile
void iLogFileInitialize(std::string_view dir);

// WebAdmin
void iWebAdminInitialize();

} // namespace
