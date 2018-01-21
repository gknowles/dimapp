// Copyright Glen Knowles 2017.
// Distributed under the Boost Software License, Version 1.0.
//
// env.h - dim app
#pragma once

#include "cppconf/cppconf.h"

#include <string>

namespace Dim {

// Returns path to this executable being run
const std::string & envExecPath();

struct EnvMemoryConfig {
    size_t pageSize;        // memory page size
    size_t allocAlign;      // virtual memory allocation alignment

    // Access to large pages requires SeLockMemoryPrivilege, if large pages
    // are not supported (or the process is unprivileged) minLargeAlloc is set
    // to 0.
    size_t minLargeAlloc;   // min large page allocation
};
const EnvMemoryConfig & envMemoryConfig();

unsigned envCpus();

} // namespace
