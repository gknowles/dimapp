// Copyright Glen Knowles 2017 - 2022.
// Distributed under the Boost Software License, Version 1.0.
//
// env.h - dim system
#pragma once

#include "cppconf/cppconf.h"

#include "core/types.h"
#include "json/json.h"

#include <map>
#include <string>

namespace Dim {


/****************************************************************************
*
*   Machine
*
***/

// Number of logical processors on this machine.
unsigned envProcessors();

struct EnvMemoryConfig {
    size_t pageSize;        // Memory page size
    size_t allocAlign;      // Virtual memory allocation alignment

    // Access to large pages requires SeLockMemoryPrivilege, if large pages
    // are not supported (or the process is unprivileged) minLargeAlloc is set
    // to 0.
    size_t minLargeAlloc;   // Minimum large page allocation
};
const EnvMemoryConfig & envMemoryConfig();

struct DiskAlignment {
    // All values measured in bytes
    unsigned cacheLine;
    unsigned cacheOffset;
    unsigned logicalSector;
    unsigned physicalSector;
    unsigned sectorOffset;
};
DiskAlignment envDiskAlignment(std::string_view path);

struct DiskSpace {
    uint64_t avail;
    uint64_t total;
};
DiskSpace envDiskSpace(std::string_view path);


/****************************************************************************
*
*   Current Executable File
*
***/

// Returns path to this executable being run.
const std::string & envExecPath();

// Gets version info for this executable.
VersionInfo envExecVersion();

// Date this executable was built.
TimePoint envExecBuildTime();


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

// Root of default system directory for application to write crash and log
// data.
std::string envProcessLogDataDir();


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
