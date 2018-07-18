// Copyright Glen Knowles 2017 - 2018.
// Distributed under the Boost Software License, Version 1.0.
//
// env.h - dim system
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
unsigned envProcessId();

struct VersionInfo {
    unsigned major;
    unsigned minor;
    unsigned patch;
    unsigned build;
};
// Gets version info for this executable
VersionInfo envExecVersion();

// Returns rights available to the current process
enum ProcessRights {
    kEnvUserAdmin,           // User in admin group, and process is elevated
    kEnvUserRestrictedAdmin, // User in admin group, but process not elevated
    kEnvUserStandard,        // User not in admin group
};
ProcessRights envProcessRights();

} // namespace
