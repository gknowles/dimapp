// Copyright Glen Knowles 2021.
// Distributed under the Boost Software License, Version 1.0.
//
// system.h - dim tools
#pragma once

#include "cppconf/cppconf.h"

#include "file/path.h"
#include "system/system.h"

#include <functional>
#include <string_view>


namespace Dim {


/****************************************************************************
*
*   Exec subprocess - used by command line utilities to launch children
*
*   Warnings are logged, and errors are both logged and trigger a call to
*   appSignalShutdown(EX_IOERR).
*
***/

void execTool(
    std::function<void(std::string&&)> fn,
    std::string_view cmdline,
    std::string_view errTitle,
    const ExecOptions & opts = {}
);
std::string execToolWait(
    std::string_view cmdline,
    std::string_view errTitle,
    const ExecOptions & opts = {}
);


/****************************************************************************
*
*   Git helper functions
*
***/

// Failure logs error and returns empty string.
std::string gitFindRoot(std::string_view path);

// True if successful, otherwise logs error.
bool gitLoadConfig(
    std::string * content,
    std::string * configPath,
    std::string * gitRoot,
    std::string_view pathFromCurrentDir,
    std::string_view fallbackPathFromGitRoot
);

} // namespace
