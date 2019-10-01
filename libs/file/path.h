// Copyright Glen Knowles 2017 - 2019.
// Distributed under the Boost Software License, Version 1.0.
//
// path.h - dim file
#pragma once

#include "cppconf/cppconf.h"

#include <cassert>
#include <cstdint>
#include <filesystem>
#include <ostream>
#include <string>
#include <string_view>

namespace Dim {


/****************************************************************************
*
*   Path
*
*   Paths are always normalized, which means that:
*   - '.' directories are removed
*   - non-leading '..' directories are removed along with the immediately
*     preceding directory.
*   - backslashes are replaced with forward slashes
*   - consecutive slashes are replaced with a single forward slash
*   - the trailing slash is removed, unless it is also the first character
*     after the root
*
***/

class Path {
public:
    Path() {}
    Path(Path const & from) = default;
    Path(Path && from) noexcept = default;
    explicit Path(char const from[]) : Path{std::string_view{from}} {}
    explicit Path(std::string_view from);
    explicit Path(std::string const & from) : Path{std::string_view{from}} {}
    Path(std::experimental::filesystem::path const & from);

    Path & clear();
    void swap(Path & from);

    Path & operator=(Path const & from) = default;
    Path & operator=(Path && from) noexcept = default;
    Path & operator=(char const from[]) {
        return assign(std::string_view(from));
    }
    Path & operator=(std::string_view from) { return assign(from); }
    Path & operator=(std::string const & from) {
        return assign(std::string_view(from));
    }
    Path & operator=(std::experimental::filesystem::path const & from) {
        return assign(from);
    }

    Path & assign(Path const & path);
    Path & assign(Path const & path, std::string_view defExt);
    Path & assign(char const path[]) {
        return assign(std::string_view{path});
    }
    Path & assign(char const path[], std::string_view defExt) {
        return assign(std::string_view{path}, defExt);
    }
    Path & assign(std::string_view path);
    Path & assign(std::string_view path, std::string_view defExt);
    Path & assign(std::string const & path) {
        return assign(std::string_view{path});
    }
    Path & assign(std::string const & path, std::string_view defExt) {
        return assign(std::string_view{path}, defExt);
    }
    Path & assign(std::experimental::filesystem::path const & path);
    Path & assign(
        std::experimental::filesystem::path const & path,
        std::string_view defExt
    );

    Path & setDrive(char drive);
    Path & setDrive(std::string_view root);
    Path & setDir(std::string_view dir);
    Path & setParentPath(std::string_view path);
    Path & setFilename(std::string_view filename);
    Path & setFilename(std::string_view filename, std::string_view defExt);
    Path & setStem(std::string_view stem);
    Path & setStem(std::string_view stem, std::string_view ext);
    Path & setExt(std::string_view ext);
    Path & defaultExt(std::string_view defExt);

    Path & removeFilename() { return setFilename({}); }

    Path & concat(std::string_view path);
    Path & operator+=(std::string_view path) { return concat(path); }

    Path & append(std::string_view path);
    Path & operator/=(std::string_view path) { return append(path); }

    // resolve path relative to base
    Path & resolve(Path const & base);
    Path & resolve(std::string_view base);

    explicit operator bool() const { return !empty(); }
    operator std::string_view() const { return m_data; }

    std::experimental::filesystem::path fsPath() const;
    std::string const & str() const { return m_data; }
    std::string_view view() const { return m_data; }
    char const * c_str() const;
    size_t size() const;

    std::string_view drive() const;
    std::string_view dir() const;
    std::string_view parentPath() const;
    std::string_view filename() const;
    std::string_view stem() const;
    std::string_view extension() const;

    bool empty() const;
    bool hasDrive() const;
    bool hasRootDir() const;
    bool hasDir() const;
    bool hasFilename() const;
    bool hasStem() const;
    bool hasExt() const;

private:
    friend bool operator==(Path const & a, std::string_view b);
    friend bool operator==(std::string_view a, Path const & b);
    friend bool operator==(Path const & a, Path const & b);
    friend std::ostream & operator<<(std::ostream & os, Path const & val);
    friend Path operator/ (Path const & a, std::string_view b);
    friend Path operator+ (Path const & a, std::string_view b);

    std::string m_data;
};


/****************************************************************************
*
*   Dim::Cli
*
***/

#ifdef DIMCLI_LIB_DECL

//===========================================================================
template <>
inline std::string Cli::OptBase::toValueDesc<Path>() const {
    return "FILE";
}

#endif

} // namespace


/****************************************************************************
*
*   std::hash
*
***/

template<> struct std::hash<Dim::Path> {
    size_t operator()(Dim::Path const & val) const {
        return std::hash<std::string>()(val.str());
    }
};


/****************************************************************************
*
*   filesystem::path
*
***/

//===========================================================================
template <>
inline std::experimental::filesystem::path::path(std::string const & from) {
    *this = u8path(from);
}

//===========================================================================
template <>
inline std::experimental::filesystem::path::path(
    char const * first,
    char const * last
) {
    *this = u8path(first, last);
}
