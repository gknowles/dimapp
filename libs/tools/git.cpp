// Copyright Glen Knowles 2021 - 2022.
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
    auto res = execToolWait(cmdline, path);
    return Path(trim(res.output)).str();
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
    Finally fin([=]() {
        appSignalShutdown(EX_DATAERR);
        content->clear();
        configFile->clear();
        gitRoot->clear();
    });

    if (!pathFromCurrentDir.empty()) {
        *configFile = pathFromCurrentDir;
    } else {
        Path tmp;
        if (auto ec = fileGetCurrentDir(&tmp); ec)
            return false;
        *gitRoot = gitFindRoot(tmp.view());
        if (gitRoot->empty())
            return false;;
        *configFile = Path(fallbackPathFromGitRoot).resolve(*gitRoot);
    }
    *gitRoot = gitFindRoot(Path(*configFile).parentPath());
    if (gitRoot->empty())
        return false;
    bool found = false;
    if (auto ec = fileExists(&found, *configFile))
        return false;
    if (!found) {
        auto path = Path(envExecPath()).removeFilename()
            / Path(*configFile).filename();
        *configFile = move(path);
    }
    if (auto ec = fileLoadBinaryWait(content, *configFile); ec)
        return false;

    fin.release();
    return true;
}
