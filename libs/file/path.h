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
    Path(const Path & from) = default;
    Path(Path && from) noexcept = default;
    explicit Path(const char from[]) : Path{std::string_view{from}} {}
    explicit Path(std::string_view from);
    explicit Path(const std::string & from) : Path{std::string_view{from}} {}
    Path(const std::filesystem::path & from);

    Path & clear();
    void swap(Path & from);

    Path & operator=(const Path & from) = default;
    Path & operator=(Path && from) noexcept = default;
    Path & operator=(const char from[]) {
        return assign(std::string_view(from));
    }
    Path & operator=(std::string_view from) { return assign(from); }
    Path & operator=(const std::string & from) {
        return assign(std::string_view(from));
    }
    Path & operator=(const std::filesystem::path & from) {
        return assign(from);
    }

    Path & assign(const Path & path);
    Path & assign(const Path & path, std::string_view defExt);
    Path & assign(const char path[]) {
        return assign(std::string_view{path});
    }
    Path & assign(const char path[], std::string_view defExt) {
        return assign(std::string_view{path}, defExt);
    }
    Path & assign(std::string_view path);
    Path & assign(std::string_view path, std::string_view defExt);
    Path & assign(const std::string & path) {
        return assign(std::string_view{path});
    }
    Path & assign(const std::string & path, std::string_view defExt) {
        return assign(std::string_view{path}, defExt);
    }
    Path & assign(const std::filesystem::path & path);
    Path & assign(
        const std::filesystem::path & path,
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
    Path & resolve(const Path & base);
    Path & resolve(std::string_view base);

    explicit operator bool() const { return !empty(); }
    operator std::string_view() const { return m_data; }

    std::filesystem::path fsPath() const;
    const std::string & str() const { return m_data; }
    std::string_view view() const { return m_data; }
    const char * c_str() const;
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
    friend bool operator==(const Path & a, std::string_view b);
    friend bool operator==(std::string_view a, const Path & b);
    friend bool operator==(const Path & a, const Path & b);
    friend std::ostream & operator<<(std::ostream & os, const Path & val);
    friend Path operator/ (const Path & a, std::string_view b);
    friend Path operator+ (const Path & a, std::string_view b);

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

template<>
struct std::hash<Dim::Path> {
    size_t operator()(const Dim::Path & val) const {
        return std::hash<std::string>()(val.str());
    }
};


/****************************************************************************
*
*   filesystem::path
*
***/

#pragma warning(push)
#pragma warning(disable: 4996) // disable deprecated warning for u8path

//===========================================================================
template <>
inline std::filesystem::path::path(
    const std::string & from,
    std::filesystem::path::format
) {
    *this = u8path(from);
}

//===========================================================================
template <>
inline std::filesystem::path::path(
    const char * first,
    const char * last,
    std::filesystem::path::format
) {
    *this = u8path(first, last);
}

#pragma warning(pop)
