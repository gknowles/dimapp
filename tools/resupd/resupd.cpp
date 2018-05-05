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

namespace {

struct ResNameInfo {
    vector<string> names;
};

class ResourceModule {
public:
    ~ResourceModule();

    bool open(const Path & path);
    bool loadNames(vector<string> * names);
    bool update(string_view name, string_view data);
    bool commit();

private:
    HMODULE m_mod{nullptr};
    Path m_path;
};

} // namespace


/****************************************************************************
*
*   Variables
*
***/


/****************************************************************************
*
*   ResourceModule
*
***/

//===========================================================================
ResourceModule::~ResourceModule() {
    if (m_mod)
        EndUpdateResource(m_mod, true);
}

//===========================================================================
bool ResourceModule::open(const Path & path) {
    m_path = path;
    m_mod = (HMODULE) BeginUpdateResource(m_path.c_str(), FALSE);
    if (!m_mod) {
        logMsgError() << "BeginUpdateResource(" << m_path << "): "
            << WinError{};
        return false;
    }
    return true;
}

//===========================================================================
static BOOL CALLBACK enumNameCallback(
    HMODULE mod,
    LPCWSTR type,
    LPWSTR name,
    LONG_PTR param
) {
    auto rni = (ResNameInfo *) param;
    rni->names.push_back(toString(name));
    return true;
}

//===========================================================================
bool ResourceModule::loadNames(vector<string> * names) {
    ResNameInfo rni;
    if (!EnumResourceNamesW(
        m_mod,
        MAKEINTRESOURCEW(23),
        enumNameCallback,
        (LONG_PTR) &rni
    )) {
        WinError err;
        if (err == ERROR_RESOURCE_DATA_NOT_FOUND
            || err == ERROR_RESOURCE_TYPE_NOT_FOUND
        ) {
            // no pre-existing resources
        } else {
            logMsgError() << "EnumResourceNames(" << m_path << "): "
                << WinError{};
            return false;
        }
    }
    return true;
}

//===========================================================================
bool ResourceModule::update(string_view name, string_view data) {
    return false;
}

//===========================================================================
bool ResourceModule::commit() {
    assert(m_mod);
    WinError err{0};
    if (!EndUpdateResource(m_mod, false)) {
        err.set();
        logMsgError() << "EndUpdateResource(" << m_path << "): " << err;

    }
    m_mod = nullptr;
    return err;
}


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

    target->defaultExt("exe");
    ResourceModule rm;
    if (!rm.open(*target))
        return appSignalShutdown(EX_DATAERR);
    vector<string> names;
    if (!rm.loadNames(&names))
        return appSignalShutdown(EX_DATAERR);

    *src /= "manifest.json";

    src->defaultExt("");

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
