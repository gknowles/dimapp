// Copyright Glen Knowles 2017 - 2021.
// Distributed under the Boost Software License, Version 1.0.
//
// env.h - dim system
#pragma once

#include "cppconf/cppconf.h"

#include "json/json.h"

#include <map>
#include <string>

namespace Dim {


/****************************************************************************
*
*   Machine
*
***/

struct EnvMemoryConfig {
    size_t pageSize;        // memory page size
    size_t allocAlign;      // virtual memory allocation alignment

    // Access to large pages requires SeLockMemoryPrivilege, if large pages
    // are not supported (or the process is unprivileged) minLargeAlloc is set
    // to 0.
    size_t minLargeAlloc;   // min large page allocation
};
const EnvMemoryConfig & envMemoryConfig();

struct DiskSpace {
    uint64_t avail;
    uint64_t total;
};
DiskSpace envDiskSpace(std::string_view path);

unsigned envProcessors();


/****************************************************************************
*
*   Executable file
*
***/

// Returns path to this executable being run
const std::string & envExecPath();

struct VersionInfo {
    unsigned major;
    unsigned minor;
    unsigned patch;
    unsigned build;
};
// Gets version info for this executable
VersionInfo envExecVersion();


/****************************************************************************
*
*   Process
*
***/

unsigned envProcessId();
TimePoint envProcessStartTime();

// Returns rights available to the current process
enum ProcessRights {
    kEnvUserAdmin,           // User in admin group, and process is elevated
    kEnvUserRestrictedAdmin, // User in admin group, but process not elevated
    kEnvUserStandard,        // User not in admin group
};
ProcessRights envProcessRights();

// Dump information about current account
void envProcessAccount(IJBuilder * out);


/****************************************************************************
*
*   Environment Variables
*
***/

std::map<std::string, std::string> envGetVars();
std::string envGetVar(std::string_view name);
// Returns true if successful
bool envSetVar(std::string_view name, std::string_view value);

} // namespace
