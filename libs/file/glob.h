// Copyright Glen Knowles 2022.
// Distributed under the Boost Software License, Version 1.0.
//
// glob.h - dim file
#pragma once

#include "cppconf/cppconf.h"

#include "file/path.h"

#include "core/time.h"
#include "core/types.h"

#include <memory>
#include <string_view>

namespace Dim {
namespace Glob {


/****************************************************************************
*
*   Directory search
*
***/

enum GlobType {
    kInvalid,
    kRuby,
};

enum Mode : unsigned {
    // When to return directories in the iteration, either before the
    // contained files, after them, or both. Implies recursion into
    // subdirectories.
    fDirsFirst = 0x01,
    fDirsLast  = 0x02,

    // Only return directories, implies recursive search. Nothing will be
    // returned unless fDirsFirst and/or fDirsLast is also set.
    fDirsOnly  = 0x04,

    // Hidden files and directories are excluded by default, this flag
    // causes them to be included.
    fHidden    = 0x08,
};

struct Entry {
    Path path;
    bool isdir{false};
    bool isbefore{false};
    TimePoint mtime;
};

class Iter {
public:
    struct Info;

public:
    Iter() {}
    Iter(std::shared_ptr<Info> info);

    bool operator==(const Iter & right) const = default;
    const Entry & operator*() const;
    const Entry * operator->() const;
    Iter & operator++();

private:
    std::shared_ptr<Info> m_info;
};

inline Iter begin(Iter iter) { return iter; }
inline Iter end(const Iter & iter) { return {}; }

} // namespace


/****************************************************************************
*
*   Public API
*
***/

Glob::Iter fileGlob(
    std::string_view root,
    std::string_view pattern = {},
    EnumFlags<Glob::Mode> flags = {},
    Glob::GlobType type = Glob::kRuby
);

} // namespace
