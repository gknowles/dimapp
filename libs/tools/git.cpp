// Copyright Glen Knowles 2021.
// Distributed under the Boost Software License, Version 1.0.
//
// git.cpp - dim tools
#include "pch.h"
#pragma hdrstop

using namespace std;
using namespace Dim;


/****************************************************************************
*
*   Git helper functions
*
***/

//===========================================================================
// Failure logs error and returns empty string.
string Dim::gitFindRoot(string_view path) {
    // Query metadata of repo containing config file
    auto cmdline = Cli::toCmdlineL(
        "git",
        "-C",
        path,
        "rev-parse",
        "--show-toplevel"
    );
    auto content = execToolWait(cmdline, path);
    return Path(trim(content)).str();
}

//===========================================================================
// True if successful, otherwise logs error.
bool Dim::gitLoadConfig(
    string * content,
    string * configFile,
    string * gitRoot,
    string_view pathFromCurrentDir,
    string_view fallbackPathFromGitRoot
) {
    if (!pathFromCurrentDir.empty()) {
        *configFile = pathFromCurrentDir;
    } else {
        *gitRoot = gitFindRoot(fileGetCurrentDir());
        *configFile = Path(fallbackPathFromGitRoot).resolve(*gitRoot);
    }
    *gitRoot = gitFindRoot(Path(*configFile).parentPath());
    if (!fileExists(*configFile)) {
        auto path = Path(envExecPath()).removeFilename()
            / Path(*configFile).filename();
        *configFile = move(path);
    }
    if (!fileLoadBinaryWait(content, *configFile)) {
        appSignalShutdown(EX_DATAERR);
        configFile->clear();
        gitRoot->clear();
        return false;
    }
    return true;
}
