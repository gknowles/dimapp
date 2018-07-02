// Copyright Glen Knowles 2017 - 2018.
// Distributed under the Boost Software License, Version 1.0.
//
// exec.h - dim app
#pragma once

#include "cppconf/cppconf.h"

#include <string>

namespace Dim {

// Waits for program to complete and returns its exit code, returns -1
// if unable to execute program.
int execElevated(std::string_view prog, std::string_view args);

} // namespace
