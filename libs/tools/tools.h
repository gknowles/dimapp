// Copyright Glen Knowles 2021 - 2023.
// Distributed under the Boost Software License, Version 1.0.
//
// system.h - dim tools
#pragma once

#include "cppconf/cppconf.h"

#include "file/path.h"
#include "system/system.h"

#include <functional>
#include <string_view>
#include <vector>


namespace Dim {


/****************************************************************************
*
*   Exec subprocess - used by command line utilities to launch children
*
*   Warnings are logged, and errors are both logged and trigger a call to
*   appSignalShutdown(EX_IOERR).
*
*   An error is defined as a failure to launch or an exit code that is not
*   contained in the 'codes' argument.
*
***/

void execTool(
    std::function<void(std::string&&)> fn,
    std::string_view cmdline,
    std::string_view errTitle,
    const ExecOptions & opts = {},
    const std::vector<int> & codes = {0}
);
std::string execToolWait(
    std::string_view cmdline,
    std::string_view errTitle,
    const ExecOptions & opts = {},
    const std::vector<int> & codes = {0}
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


/****************************************************************************
*
*   Test helper functions
*
***/

// Monitors log messages and filters out those that match the messages once
// each and in the order given. Logs an error if called with some messages
// still unmatched, but still registers the new list.
void testLogMsgs(
    const std::vector<std::pair<LogType, std::string>> & msgs
);

// Uses logGetMsgCount() and appBaseName() to print final pass/fail report
// and signal shutdown with EX_OK or EX_SOFTWARE.
void testSignalShutdown();


} // namespace
