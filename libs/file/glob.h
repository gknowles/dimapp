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


/****************************************************************************
*
*   Directory search
*
***/
 
enum class GlobMode : unsigned {
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

struct GlobEntry {
    Path path;
    bool isdir{false};
    bool isbefore{false};
    TimePoint mtime;
};

class GlobIter {
public:
    struct Info;

public:
    GlobIter() {}
    GlobIter(std::shared_ptr<Info> info);

    bool operator==(const GlobIter & right) const = default;
    const GlobEntry & operator*() const;
    const GlobEntry * operator->() const;
    GlobIter & operator++();

private:
    std::shared_ptr<Info> m_info;
};

inline GlobIter begin(GlobIter iter) { return iter; }
inline GlobIter end(const GlobIter & iter) { return {}; }

template<> struct is_enum_flags<GlobMode> : std::true_type {};


GlobIter fileGlob(
    std::string_view dir, 
    std::string_view name = {}, 
    EnumFlags<GlobMode> flags = {}
);

} // namespace
