// Copyright Glen Knowles 2017.
// Distributed under the Boost Software License, Version 1.0.
//
// path.h - dim file
#pragma once

#include "cppconf/cppconf.h"

#include <cassert>
#include <cstdint>
#include <experimental/filesystem>
#include <ostream>
#include <string>
#include <string_view>

namespace Dim {


/****************************************************************************
*
*   Path
*
***/

class Path {
public:
    Path() {}
    Path(const char from[]) : Path{std::string_view{from}} {}
    Path(std::string_view from);
    Path(const std::experimental::filesystem::path & from);

    Path & clear();
    void swap(Path & from);

    Path & assign(const Path & path);
    Path & assign(const Path & path, std::string_view defExt);
    Path & assign(std::string_view path);
    Path & assign(std::string_view path, std::string_view defExt);
    Path & assign(const std::experimental::filesystem::path & path);
    Path & assign(
        const std::experimental::filesystem::path & path, 
        std::string_view defExt
    );
    Path & operator=(const char from[]) { 
        return assign(std::string_view(from)); 
    }
    Path & operator=(const Path & from) { return assign(from); }
    Path & operator=(std::string_view from) { return assign(from); }
    Path & operator=(const std::string & from) { 
        return assign(std::string_view(from)); 
    }
    Path & operator=(const std::experimental::filesystem::path & from) { 
        return assign(from); 
    }

    Path & setRootName(char drive);
    Path & setRootName(std::string_view root);
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

    // resolve path relative to base
    Path & resolve(const Path & base);
    Path & resolve(std::string_view base);
    Path & operator/=(std::string_view path) { return resolve(path); }

    explicit operator bool() const { return !empty(); }
    operator const std::string() const { return m_data; }

    std::experimental::filesystem::path fsPath() const;
    std::string_view view() const;
    const char * c_str() const;
    size_t size() const;

    std::string_view rootName() const;
    std::string_view dir() const;
    std::string_view parentPath() const;
    std::string_view filename() const;
    std::string_view stem() const;
    std::string_view extension() const;

    bool empty() const;
    bool hasRootName() const;
    bool hasRootDir() const;
    bool hasDir() const;
    bool hasFilename() const;
    bool hasStem() const;
    bool hasExt() const;

private:
    std::string m_data;
};


/****************************************************************************
*
*   Free functions
*
***/

bool operator==(const Path & left, std::string_view right);
bool operator==(std::string_view left, const Path & right);
bool operator==(const Path & left, const Path & right);

std::ostream & operator<<(std::ostream & os, const Path & val);

} // namespace


/****************************************************************************
*
*   filesystem::path
*
***/

//===========================================================================
template <>
inline std::experimental::filesystem::path::path(const std::string & from) {
    *this = u8path(from);
}

//===========================================================================
template <>
inline std::experimental::filesystem::path::path(
    const char * first, 
    const char * last
) {
    *this = u8path(first, last);
}
