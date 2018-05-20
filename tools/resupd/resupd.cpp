// Copyright Glen Knowles 2018.
// Distributed under the Boost Software License, Version 1.0.
//
// resupd.cpp - resupd
#include "pch.h"
#pragma hdrstop

using namespace std;
using namespace Dim;


/****************************************************************************
*
*   Declarations
*
***/

const char kVersion[] = "1.0";


/****************************************************************************
*
*   Variables
*
***/


/****************************************************************************
*
*   Application
*
***/

//===========================================================================
static void app(int argc, char *argv[]) {
    Cli cli;
    cli.header("resupd v"s + kVersion + " (" __DATE__ ")")
        .helpNoArgs()
        .versionOpt(kVersion, "resupd");
    auto & target = cli.opt<Path>("<target file>")
        .desc("Executable file (generally .exe or .dll) to update.");
    cli.opt<string>("<type>")
        .choice("webpack", "webpack", "Webpack bundle files.")
        .desc("Type of resource to update");
    auto & src = cli.opt<Path>("<resource directory>")
        .desc("Webpack dist directory containing bundle files.");
    if (!cli.parse(argc, argv))
        return appSignalUsageError();

    unsigned added = 0;
    unsigned updated = 0;

    target->defaultExt("exe");
    ResHandle h = resOpenForUpdate(*target);
    if (!h)
        return appSignalShutdown(EX_DATAERR);
    Finally rclose{ [=]() { resClose(h); } };

    ResFileMap prev;
    if (!prev.parse(resLoadHtml(h, kResWebSite))) {
        prev.clear();
        logMsgWarn() << "Invalid website resource: " << *target;
    }

    if (!fileExists(*src / "manifest.json"))
        return appSignalUsageError(EX_DATAERR, "File not found: manifest.json");

    cout << "Updating '" << *target << "' from '" << *src << endl;

    ResFileMap files;
    for (auto & fn : FileIter{*src}) {
        string content;
        if (!fileLoadBinaryWait(&content, fn.path))
            return appSignalShutdown(EX_DATAERR);
        auto path = fn.path.str();
        path.erase(0, src->size() + 1);
        auto mtime = fileLastWriteTime(fn.path);
        if (auto ent = prev.find(path)) {
            if (ent->mtime != mtime || ent->content != content)
                updated += 1;
            prev.erase(path);
        } else {
            added += 1;
        }
        files.insert(path, mtime, move(content));
    }
    auto removed = (unsigned) prev.size();

    if (added + updated + removed == 0) {
        cout << "Resources: " << files.size() << " (0 changed)" << endl;
        return appSignalShutdown(EX_OK);
    }

    CharBuf out;
    files.copy(&out);
    if (!resUpdate(h, kResWebSite, out.view()))
        return appSignalShutdown(EX_DATAERR);

    rclose = {};
    if (!resClose(h, true))
        return appSignalShutdown(EX_DATAERR);

    ConsoleScopedAttr attr{kConsoleHighlight};
    cout << "Resources: " << files.size() << " ("
        << added << " added, "
        << updated << " updated, "
        << removed << " removed"
        << ")" << endl;
    return appSignalShutdown(EX_OK);
}


/****************************************************************************
*
*   Externals
*
***/

//===========================================================================
int main(int argc, char * argv[]) {
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
    _set_error_mode(_OUT_TO_MSGBOX);

    return appRun(app, argc, argv);
}
