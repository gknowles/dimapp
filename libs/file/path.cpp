// Copyright Glen Knowles 2017.
// Distributed under the Boost Software License, Version 1.0.
//
// path.cpp - dim file
#include "pch.h"
#pragma hdrstop

using namespace std;
using namespace Dim;
namespace fs = std::experimental::filesystem;


/****************************************************************************
*
*   Declarations
*
***/

namespace {

struct Count {
    unsigned m_rootLen;
    unsigned m_dirLen;
    unsigned m_stemLen;
    unsigned m_extLen;

    Count(string_view path);
};

} // namespace


/****************************************************************************
*
*   Path
*
***/

//===========================================================================
Path::Path(std::string_view from) 
    : m_data(from)
{}

//===========================================================================
Path::Path(const fs::path & from) 
    : m_data(from.generic_u8string())
{}

//===========================================================================
Path & Path::clear() {
    m_data.clear();
    return *this;
}

//===========================================================================
void Path::swap(Path & from) {
    ::swap(m_data, from.m_data);
}

//===========================================================================
Path & Path::assign(std::string_view path) {
    m_data = path;
    //normalize();
    return *this;
}

//===========================================================================
Path & Path::assign(std::string_view path, std::string_view defExt) {
    return assign(path).defaultExt(defExt);
}

//===========================================================================
Path & Path::assign(const fs::path & path) {
    return assign(string_view{path.generic_u8string()});
}

//===========================================================================
Path & Path::assign(const fs::path & path, std::string_view defExt) {
    return assign(path).defaultExt(defExt);
}

//===========================================================================
Path & Path::setRootName(char drive) {
    Count cnt(m_data);
    return *this;
}

//===========================================================================
Path & Path::setRootName(std::string_view root) {
    Count cnt(m_data);
    return *this;
}

//===========================================================================
Path & Path::setDir(std::string_view dir) {
    Count cnt(m_data);
    return *this;
}

//===========================================================================
Path & Path::setParentPath(std::string_view path) {
    Count cnt(m_data);
    return *this;
}

//===========================================================================
Path & Path::setFilename(std::string_view filename) {
    Count cnt(m_data);
    return *this;
}

//===========================================================================
Path & Path::setFilename(std::string_view filename, std::string_view defExt) {
    Count cnt(m_data);
    return *this;
}

//===========================================================================
Path & Path::setStem(std::string_view stem) {
    Count cnt(m_data);
    return *this;
}

//===========================================================================
Path & Path::setStem(std::string_view stem, std::string_view ext) {
    Count cnt(m_data);
    return *this;
}

//===========================================================================
Path & Path::setExt(std::string_view ext) {
    Count cnt(m_data);
    return *this;
}

//===========================================================================
Path & Path::defaultExt(std::string_view defExt) {
    Count cnt(m_data);
    return *this;
}

//===========================================================================
Path & Path::concat(std::string_view path) {
    Count cnt(m_data);
    return *this;
}

//===========================================================================
Path & Path::resolve(const Path & base) {
    return resolve(base.m_data);
}

//===========================================================================
Path & Path::resolve(std::string_view base) {
    return *this;
}

//===========================================================================
fs::path Path::fsPath() const {
    return fs::u8path(m_data);
}

//===========================================================================
std::string_view Path::view() const {
    return m_data;
}

//===========================================================================
const char * Path::c_str() const {
    return m_data.c_str();
}

//===========================================================================
size_t Path::size() const {
    return m_data.size();
}

//===========================================================================
std::string_view Path::rootName() const {
    Count cnt(m_data);
    return {m_data.data(), cnt.m_rootLen};
}

//===========================================================================
std::string_view Path::dir() const {
    Count cnt(m_data);
    return {m_data.data() + cnt.m_rootLen, cnt.m_dirLen};
}

//===========================================================================
std::string_view Path::parentPath() const {
    Count cnt(m_data);
    return {m_data.data(), cnt.m_rootLen + cnt.m_dirLen};
}

//===========================================================================
std::string_view Path::filename() const {
    Count cnt(m_data);
    return {
        m_data.data() + cnt.m_rootLen + cnt.m_dirLen, 
        cnt.m_stemLen + cnt.m_extLen
    };
}

//===========================================================================
std::string_view Path::stem() const {
    Count cnt(m_data);
    return {m_data.data() + cnt.m_rootLen + cnt.m_dirLen, cnt.m_stemLen};
}

//===========================================================================
std::string_view Path::extension() const {
    Count cnt(m_data);
    return {
        m_data.data() + cnt.m_rootLen + cnt.m_dirLen + cnt.m_stemLen,
        cnt.m_extLen
    };
}

//===========================================================================
bool Path::empty() const {
    return m_data.empty();
}

//===========================================================================
bool Path::hasRootName() const {
    Count cnt(m_data);
    return cnt.m_rootLen;
}

//===========================================================================
bool Path::hasRootDir() const {
    Count cnt(m_data);
    return cnt.m_dirLen && m_data[cnt.m_rootLen] == '/';
}

//===========================================================================
bool Path::hasDir() const {
    Count cnt(m_data);
    return cnt.m_dirLen;
}

//===========================================================================
bool Path::hasFilename() const {
    Count cnt(m_data);
    return cnt.m_stemLen + cnt.m_extLen;
}

//===========================================================================
bool Path::hasStem() const {
    Count cnt(m_data);
    return cnt.m_stemLen;
}

//===========================================================================
bool Path::hasExt() const {
    Count cnt(m_data);
    return cnt.m_extLen;
}
