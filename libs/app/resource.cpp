// Copyright Glen Knowles 2018.
// Distributed under the Boost Software License, Version 1.0.
//
// resource.cpp - dim app
#include "pch.h"
#pragma hdrstop

using namespace std;
using namespace Dim;


/****************************************************************************
*
*   Variables
*
***/

static ResFileMap s_files;


/****************************************************************************
*
*   ResFileMap
*
***/

//===========================================================================
void ResFileMap::clear() {
    m_files.clear();
    m_data.clear();
}

//===========================================================================
bool ResFileMap::insert(
    string_view name,
    TimePoint mtime,
    string && content
) {
    bool inserted = !m_files.count(name);

    name = m_data.emplace_back(name);
    auto & ent = m_files[name];
    ent.mtime = mtime;
    ent.content = m_data.emplace_back(move(content));
    return inserted;
}

//===========================================================================
bool ResFileMap::erase(string_view name) {
    return m_files.erase(name) > 0;
}

//===========================================================================
static void appendNum(CharBuf * out, uint64_t val) {
    char tmp[8];
    out->append(hton64(tmp, val), size(tmp));
}

//===========================================================================
void ResFileMap::copy(CharBuf * out) const {
    out->clear();
    for (auto && [name, ent] : m_files) {
        appendNum(out, name.size());
        out->append(name);
        appendNum(out, ent.mtime.time_since_epoch().count());
        appendNum(out, ent.content.size());
        out->append(ent.content);
    }
}

//===========================================================================
static bool parseNum(uint64_t * out, string_view * data) {
    if (data->size() < sizeof(*out))
        return false;
    *out = ntoh64(data->data());
    data->remove_prefix(sizeof(*out));
    return true;
}

//===========================================================================
static bool parseView(string_view * out, string_view * data) {
    size_t len;
    if (!parseNum(&len, data))
        return false;
    if (data->size() < len)
        return false;
    *out = {data->data(), len};
    data->remove_prefix(len);
    return true;
}

//===========================================================================
bool ResFileMap::parse(string_view data) {
    string_view name;
    Entry ent;
    uint64_t num;
    while (!data.empty()) {
        if (!parseView(&name, &data))
            return false;
        if (!parseNum(&num, &data))
            return false;
        ent.mtime = TimePoint{Duration{num}};
        if (!parseView(&ent.content, &data))
            return false;
        m_files[name] = ent;
    }
    return true;
}

//===========================================================================
const ResFileMap::Entry * ResFileMap::find(string_view name) const {
    if (auto i = m_files.find(name); i != m_files.end())
        return &i->second;
    return nullptr;
}


/****************************************************************************
*
*   ShutdownNotify
*
***/

namespace {
class ShutdownNotify : public IShutdownNotify {
    void onShutdownConsole (bool firstTry) override;
};
} // namespace
static ShutdownNotify s_cleanup;

//===========================================================================
void ShutdownNotify::onShutdownConsole (bool firstTry) {
    s_files.clear();
}


/****************************************************************************
*
*   Public API
*
***/

//===========================================================================
void Dim::resLoadWebSite(string_view moduleName) {
    shutdownMonitor(&s_cleanup);

    auto h = resOpen({});
    auto src = resLoadHtml(h, kResWebSite);
    if (!s_files.parse(src)) {
        s_files.clear();
        return;
    }

    for (auto && [name, ent] : s_files)
        httpRouteAddFileRef(name, ent.mtime, ent.content);
}
