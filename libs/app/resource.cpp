// Copyright Glen Knowles 2018 - 2020.
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
    name = strDup(string(name));
    auto & ent = m_files[name];
    ent.mtime = mtime;
    ent.content = strDup(move(content));
    return inserted;
}

//===========================================================================
string_view ResFileMap::strDup(string && data) {
    return m_data.emplace_back(move(data));
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
    if (data->size() < sizeof *out)
        return false;
    *out = ntoh64(data->data());
    data->remove_prefix(sizeof *out);
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
void Dim::resLoadWebSite(
    string_view urlPrefix,
    string_view moduleName,
    string_view fallbackResFileMap
) {
    shutdownMonitor(&s_cleanup);

    auto h = resOpen(moduleName);
    auto src = resLoadData(h, kResWebSite);
    resClose(h);
    if (src.empty())
        src = fallbackResFileMap;
    if (!s_files.parse(src)) {
        s_files.clear();
        return;
    }

    auto prefix = string(urlPrefix);
    if (!prefix.empty() && prefix.back() == '/')
        prefix.pop_back();
    for (auto && [name, ent] : s_files) {
        auto rname = name;
        auto rcontent = ent.content;
        Path route(prefix);
        route += rname;
        bool dir = false;
        bool html = false;
        if (route.extension() == ".html") {
            html = true;
            if (route.filename() == "index.html") {
                dir = true;
                route.removeFilename();
            }
        }
        if (route != rname) {
            auto tmp = string(route);
            if (dir)
                tmp += '/';
            if (html) {
                string base = "<base href=\"";
                base += rname;
                base += "\">";
                if (auto pos = rcontent.find(base); pos != string::npos) {
                    string content;
                    content += rcontent.substr(0, pos);
                    content += "<base href=\"";
                    content += tmp;
                    content += "\">";
                    content += rcontent.substr(pos + base.size());
                    rcontent = s_files.strDup(move(content));
                }
            }
            rname = s_files.strDup(move(tmp));
        }
        auto mime = html ? "text/html"sv : ""sv;
        httpRouteAddFileRef(rname, ent.mtime, rcontent, mime);
        if (dir) {
            rname.remove_suffix(1);
            httpRouteAddFileRef(rname, ent.mtime, rcontent, mime);
        }
    }
}
