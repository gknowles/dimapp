// Copyright Glen Knowles 2018.
// Distributed under the Boost Software License, Version 1.0.
//
// resource.h - dim app
#pragma once

#include "cppconf/cppconf.h"

#include "core/core.h"
#include "file/path.h"

#include <deque>
#include <string>
#include <string_view>
#include <unordered_set>
#include <vector>

namespace Dim {

class ResFileMap {
public:
    struct Entry {
        TimePoint mtime;
        std::string_view content;
    };

public:
    void clear();
    bool insert(
        std::string_view name,
        TimePoint mtime,
        std::string && content
    );
    bool erase(std::string_view name);

    void copy(CharBuf * out) const;
    [[nodiscard]] bool parse(std::string_view data);

    const Entry * find(std::string_view name) const;
    size_t size() const { return m_files.size(); }

    auto begin() const { return m_files.begin(); }
    auto end() const { return m_files.end(); }

private:
    std::unordered_map<std::string_view, Entry> m_files;
    std::deque<std::string> m_data;
};


/****************************************************************************
*
*   Public API
*
***/

// Loads file map from resource module and registers the files as http
// routes. Module defaults to this executable.
void resLoadWebSite(std::string_view moduleName = {});

const char kResWebSite[] = "WEBSITE";

struct ResHandle : HandleBase {};

ResHandle resOpen(std::string_view path);
ResHandle resOpenForUpdate(std::string_view path);

// Commit can only be true if the handle was opened for update. On such a
// handle any changes made via resUpdate are not written unless and until
// resClose is called with commit set to true.
bool resClose(ResHandle h, bool commit = false);

bool resUpdate(ResHandle h, std::string_view name, std::string_view data);

bool resLoadNames(ResHandle h, std::vector<std::string> * names);
std::string_view resLoadHtml(ResHandle h, std::string_view name);

} // namespace
