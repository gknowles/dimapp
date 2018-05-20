// Copyright Glen Knowles 2018.
// Distributed under the Boost Software License, Version 1.0.
//
// winres.cpp - dim winres
#include "pch.h"
#pragma hdrstop

using namespace std;
using namespace Dim;

using NtStatus = WinError::NtStatus;
using SecStatus = WinError::SecurityStatus;


/****************************************************************************
*
*   Private
*
***/

namespace {

struct ResNameInfo {
    vector<string> names;
};

class ResModule : public HandleContent {
public:
    ~ResModule();

    bool open(const Path & path);
    bool openForUpdate(const Path & path);

    bool updateHtml(std::string_view name, std::string_view data);
    bool commit();

    bool loadNames(std::vector<std::string> * names);
    string_view loadHtml(string_view name);

private:
    HANDLE m_whandle{};
    HMODULE m_rhandle{};
    Path m_path;
};

} // namespace


/****************************************************************************
*
*   Variables
*
***/

static mutex s_mut;
static HandleMap<ResHandle, ResModule> s_modules;


/****************************************************************************
*
*   ResModule
*
***/

//===========================================================================
ResModule::~ResModule() {
    if (m_rhandle)
        FreeLibrary(m_rhandle);
    if (m_whandle)
        EndUpdateResource(m_whandle, true);
}

//===========================================================================
bool ResModule::open(const Path & path) {
    m_path = path;
    m_rhandle = LoadLibrary(m_path.c_str());
    if (!m_rhandle) {
        logMsgError() << "LoadLibrary(" << m_path << "): " << WinError{};
        return false;
    }
    return true;
}

//===========================================================================
bool ResModule::openForUpdate(const Path & path) {
    if (!open(path))
        return false;
    m_whandle = BeginUpdateResource(m_path.c_str(), FALSE);
    if (!m_whandle) {
        logMsgError() << "BeginUpdateResource(" << m_path << "): "
            << WinError{};
        FreeLibrary(m_rhandle);
        m_rhandle = nullptr;
        return false;
    }
    return true;
}

//===========================================================================
bool ResModule::updateHtml(string_view namev, string_view data) {
    auto name = toWstring(namev);
    if (!UpdateResourceW(
        m_whandle,
        MAKEINTRESOURCEW(23),
        name.c_str(),
        MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL),
        data.size() ? (void *) data.data() : nullptr,
        (DWORD) data.size()
    )) {
        logMsgError() << "UpdateResource(" << namev << "): " << WinError{};
        return false;
    }
    return true;
}

//===========================================================================
bool ResModule::commit() {
    assert(m_whandle);
    FreeLibrary(m_rhandle);
    m_rhandle = nullptr;
    WinError err{0};
    if (!EndUpdateResource(m_whandle, false)) {
        err.set();
        logMsgError() << "EndUpdateResource(" << m_path << "): " << err;
    }
    m_whandle = nullptr;
    return !err;
}

//===========================================================================
static BOOL CALLBACK enumNameCallback(
    HMODULE mod,
    LPCWSTR type,
    LPWSTR name,
    LONG_PTR param
) {
    auto rni = (ResNameInfo *) param;
    auto str = IS_INTRESOURCE(name)
        ? "#"s + StrFrom<WORD>(LOWORD(name)).c_str()
        : toString(name);
    rni->names.push_back(str);
    return true;
}

//===========================================================================
bool ResModule::loadNames(vector<string> * names) {
    ResNameInfo rni;
    WinError err{0};
    if (!EnumResourceNamesW(
        m_rhandle,
        MAKEINTRESOURCEW(23),
        enumNameCallback,
        (LONG_PTR) &rni
    )) {
        err.set();
    }
    if (!err) {
        // no errors enumerating resources
        names->swap(rni.names);
    } else if (err == ERROR_RESOURCE_DATA_NOT_FOUND
        || err == ERROR_RESOURCE_TYPE_NOT_FOUND
    ) {
        // no pre-existing resources
    } else {
        logMsgError() << "EnumResourceNames(" << m_path << "): "
            << WinError{};
        return false;
    }
    return true;
}

//===========================================================================
string_view ResModule::loadHtml(string_view namev) {
    auto name = toWstring(namev);
    auto hinfo = FindResourceW(
        m_rhandle,
        name.c_str(),
        MAKEINTRESOURCEW(23)
    );
    if (!hinfo)
        return {};
    auto h = LoadResource(m_rhandle, hinfo);
    if (!h) {
        logMsgError() << "LoadResource(" << namev << "): " << WinError{};
        return {};
    }
    auto ptr = (const char *) LockResource(h);
    auto len = SizeofResource(m_rhandle, hinfo);
    return string_view{ptr, len};
}


/****************************************************************************
*
*   Public API
*
***/

//===========================================================================
ResHandle Dim::resOpen(string_view path) {
    auto res = make_unique<ResModule>();
    if (!res->open(Path{path}))
        return {};

    scoped_lock lk{s_mut};
    return s_modules.insert(res.release());
}

//===========================================================================
ResHandle Dim::resOpenForUpdate(string_view path) {
    auto res = make_unique<ResModule>();
    if (!res->openForUpdate(Path{path}))
        return {};

    scoped_lock lk{s_mut};
    return s_modules.insert(res.release());
}

//===========================================================================
bool Dim::resClose(ResHandle h, bool commit) {
    unique_lock lk{s_mut};
    auto res = s_modules.release(h);
    lk.unlock();

    if (!res)
        return false;
    auto success = !commit || res->commit();
    delete res;
    return success;
}

//===========================================================================
bool Dim::resUpdate(ResHandle h, string_view name, string_view data) {
    if (auto res = s_modules.find(h))
        return res->updateHtml(name, data);
    return false;
}

//===========================================================================
bool Dim::resLoadNames(ResHandle h, vector<string> * names) {
    if (auto res = s_modules.find(h))
        return res->loadNames(names);
    return false;
}

//===========================================================================
string_view Dim::resLoadHtml(ResHandle h, string_view name) {
    if (auto res = s_modules.find(h))
        return res->loadHtml(name);
    return {};
}
