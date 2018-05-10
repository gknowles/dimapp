// Copyright Glen Knowles 2017 - 2018.
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
    unsigned m_rootLen;     // includes trailing ':'
    unsigned m_dirLen;      // includes leading and/or trailing '/'
    unsigned m_stemLen;
    unsigned m_extLen;      // includes leading '.'

    explicit Count(string_view path);
};

} // namespace


/****************************************************************************
*
*   Helpers
*
***/

//===========================================================================
static void addRoot(string * out, string_view val) {
    if (val.empty())
        return;
    out->append(val);
    if (out->back() != ':')
        out->push_back(':');
}

//===========================================================================
static void addStem(string * out, string_view stem) {
    out->append(stem);
}

//===========================================================================
static void addExt(string * out, string_view ext) {
    if (ext.empty())
        return;

    if (ext[0] != '.')
        out->push_back('.');
    out->append(ext);
    while (!out->empty() && out->back() == '.')
        out->pop_back();
}

//===========================================================================
static void normalize(string * path) {
    string out;
    out.reserve(path->size());
    Count cnt(*path);
    out.append(*path, 0, cnt.m_rootLen);
    auto * base = path->data() + cnt.m_rootLen;
    auto * ptr = base;
    auto * eptr = ptr + cnt.m_dirLen;
    enum {
        kNone,
        kNormal,
        kSlash,
        kDot,
    } prevChar{kNone};
    for (; ptr != eptr; ++ptr) {
        switch(*ptr) {
        case '/':
        case '\\':
            switch (prevChar) {
            case kSlash:
                continue;
            case kDot:
                {
                    auto pos = ptr - base;
                    if (pos == 1) {
                        out.pop_back();
                        prevChar = kSlash;
                        continue;
                    } else if (pos > 1) {
                        if (ptr[-2] == '/' || ptr[-2] == '\\') {
                            out.pop_back();
                            prevChar = kSlash;
                            continue;
                        }
                        if (ptr[-2] == '.' && pos >= 3
                            && (ptr[-3] == '/' || ptr[-3] == '\\')
                        ) {
                            auto * obase = out.data() + cnt.m_rootLen;
                            auto * slash = out.data() + out.size() - 4;
                            for (;; --slash) {
                                if (slash <= obase) {
                                    out.resize(out.size() - 2);
                                    break;
                                }
                                if (*slash == '/') {
                                    out.resize(slash - out.data() + 1);
                                    break;
                                }
                            }
                            prevChar = kSlash;
                            continue;
                        }
                        if (*base == '/') {
                            while (!out.empty() && out.back() == '.')
                                out.pop_back();
                        }
                    }
                }
                break;
            case kNone:
            case kNormal:
                break;
            }
            out += '/';
            prevChar = kSlash;
            break;
        case '.':
            out += '.';
            prevChar = kDot;
            break;
        default:
            out += *ptr;
            prevChar = kNormal;
            break;
        }
    }
    out.append(ptr);
    Count ocnt(out);
    auto pos = ocnt.m_rootLen + ocnt.m_dirLen;
    while (out.size() > pos && out.back() == '.')
        out.pop_back();
    out.swap(*path);
}


/****************************************************************************
*
*   Count
*
***/

//===========================================================================
Count::Count(string_view path) {
    auto * base = path.data();
    auto * ptr = base;
    auto * eptr = ptr + path.size();
    const char * colon{nullptr};
    const char * slash{nullptr};
    const char * dot{nullptr};
    for (; ptr != eptr; ++ptr) {
        switch(*ptr) {
        case '.':
            dot = ptr;
            break;
        case '/':
        case '\\':
            slash = ptr;
            break;
        case ':':
            if (!colon)
                colon = ptr;
            break;
        }
    }

    if (!slash) {
        m_rootLen = colon ? unsigned(colon - base + 1) : 0;
        m_dirLen = 0;
    } else {
        m_rootLen = (colon && colon < slash) ? unsigned(colon - base + 1) : 0;
        m_dirLen = unsigned(slash - base + 1 - m_rootLen);
    }
    if (!dot || unsigned(dot - base) <= m_rootLen + m_dirLen) {
        m_stemLen = unsigned(path.size() - m_rootLen - m_dirLen);
        m_extLen = 0;
    } else {
        m_stemLen = unsigned(dot - base - m_rootLen - m_dirLen);
        m_extLen = unsigned(eptr - dot);
    }
    assert(m_extLen == path.size() - m_rootLen - m_dirLen - m_stemLen);
}


/****************************************************************************
*
*   Path
*
***/

//===========================================================================
Path::Path(string_view from)
    : m_data(from)
{
    normalize(&m_data);
}

//===========================================================================
Path::Path(const fs::path & from)
    : m_data(from.generic_u8string())
{
    normalize(&m_data);
}

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
Path & Path::assign(const Path & path) {
    m_data = path.m_data;
    return *this;
}

//===========================================================================
Path & Path::assign(const Path & path, string_view defExt) {
    return assign(path).defaultExt(defExt);
}

//===========================================================================
Path & Path::assign(string_view path) {
    m_data = path;
    normalize(&m_data);
    return *this;
}

//===========================================================================
Path & Path::assign(string_view path, string_view defExt) {
    return assign(path).defaultExt(defExt);
}

//===========================================================================
Path & Path::assign(const fs::path & path) {
    return assign(string_view{path.generic_u8string()});
}

//===========================================================================
Path & Path::assign(const fs::path & path, string_view defExt) {
    return assign(path).defaultExt(defExt);
}

//===========================================================================
Path & Path::setDrive(char drive) {
    return setDrive(string_view(&drive, 1));
}

//===========================================================================
Path & Path::setDrive(string_view root) {
    string out;
    addRoot(&out, root);
    Count cnt(m_data);
    out.append(m_data, cnt.m_rootLen);
    out.swap(m_data);
    return *this;
}

//===========================================================================
Path & Path::setDir(string_view dir) {
    Count cnt(m_data);
    string out;
    out.append(m_data, 0, cnt.m_rootLen);
    out.append(dir);
    if (cnt.m_stemLen || cnt.m_extLen) {
        auto old = string_view{m_data};
        out += '/';
        old.remove_prefix(cnt.m_rootLen + cnt.m_dirLen);
        addStem(&out, old.substr(0, cnt.m_stemLen));
        addExt(&out, old.substr(cnt.m_stemLen));
    }
    out.swap(m_data);
    return *this;
}

//===========================================================================
Path & Path::setParentPath(string_view path) {
    Count cnt(m_data);
    string out{path};
    if (cnt.m_stemLen || cnt.m_extLen) {
        auto old = string_view{m_data};
        out += '/';
        old.remove_prefix(cnt.m_rootLen + cnt.m_dirLen);
        addStem(&out, old.substr(0, cnt.m_stemLen));
        addExt(&out, old.substr(cnt.m_stemLen));
    }
    out.swap(m_data);
    return *this;
}

//===========================================================================
Path & Path::setFilename(string_view filename) {
    Count cnt(m_data);
    m_data.resize(cnt.m_rootLen + cnt.m_dirLen);
    Count scnt(filename);
    filename.remove_prefix(scnt.m_rootLen + scnt.m_dirLen);
    addStem(&m_data, filename.substr(0, scnt.m_stemLen));
    addExt(&m_data, filename.substr(scnt.m_stemLen));
    return *this;
}

//===========================================================================
Path & Path::setFilename(string_view filename, string_view defExt) {
    return setFilename(filename).defaultExt(defExt);
}

//===========================================================================
Path & Path::setStem(string_view stem) {
    Count cnt(m_data);
    string ext{m_data, cnt.m_rootLen + cnt.m_dirLen + cnt.m_stemLen};
    m_data.resize(cnt.m_rootLen + cnt.m_dirLen);
    Count scnt(stem);
    stem = stem.substr(scnt.m_rootLen + scnt.m_dirLen, scnt.m_stemLen);
    addStem(&m_data, stem);
    addExt(&m_data, ext);
    return *this;
}

//===========================================================================
Path & Path::setStem(string_view stem, string_view ext) {
    Count cnt(m_data);
    m_data.resize(cnt.m_rootLen + cnt.m_dirLen);
    addStem(&m_data, stem);
    addExt(&m_data, ext);
    return *this;
}

//===========================================================================
Path & Path::setExt(string_view ext) {
    Count cnt(m_data);
    if (cnt.m_extLen)
        m_data.resize(cnt.m_rootLen + cnt.m_dirLen + cnt.m_stemLen);
    addExt(&m_data, ext);
    return *this;
}

//===========================================================================
Path & Path::defaultExt(string_view defExt) {
    if (!hasExt())
        addExt(&m_data, defExt);
    return *this;
}

//===========================================================================
Path & Path::concat(string_view path) {
    m_data.append(path);
    normalize(&m_data);
    return *this;
}

//===========================================================================
Path & Path::append(string_view path) {
    auto p = Path{path}.resolve(*this);
    swap(p);
    return *this;
}

//===========================================================================
Path & Path::resolve(const Path & base) {
    return resolve(string_view{base.m_data});
}

//===========================================================================
Path & Path::resolve(string_view basePath) {
    string out;
    Path base;
    base.setParentPath(basePath);

    if (hasRootDir()) {
        if (!hasDrive()) {
            addRoot(&out, base.drive());
            out += m_data;
        } else {
            return *this;
        }
    } else {
        if (drive() != base.drive() && hasDrive())
            return *this;

        out += base;
        if (!out.empty())
            out += '/';
        out += dir();
        if (!out.empty())
            out += '/';
        out += filename();
    }
    return assign(out);
}

//===========================================================================
fs::path Path::fsPath() const {
    return fs::u8path(m_data);
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
string_view Path::drive() const {
    Count cnt(m_data);
    return {m_data.data(), cnt.m_rootLen};
}

//===========================================================================
string_view Path::dir() const {
    Count cnt(m_data);
    return {m_data.data() + cnt.m_rootLen, cnt.m_dirLen};
}

//===========================================================================
string_view Path::parentPath() const {
    Count cnt(m_data);
    return {m_data.data(), cnt.m_rootLen + cnt.m_dirLen};
}

//===========================================================================
string_view Path::filename() const {
    Count cnt(m_data);
    return {
        m_data.data() + cnt.m_rootLen + cnt.m_dirLen,
        cnt.m_stemLen + cnt.m_extLen
    };
}

//===========================================================================
string_view Path::stem() const {
    Count cnt(m_data);
    return {m_data.data() + cnt.m_rootLen + cnt.m_dirLen, cnt.m_stemLen};
}

//===========================================================================
string_view Path::extension() const {
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
bool Path::hasDrive() const {
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


/****************************************************************************
*
*   Free functions
*
***/

//===========================================================================
bool Dim::operator==(const Path & left, string_view right) {
    return left.view() == right;
}

//===========================================================================
bool Dim::operator==(string_view left, const Path & right) {
    return left == right.view();
}

//===========================================================================
bool Dim::operator==(const Path & left, const Path & right) {
    return left.view() == right.view();
}

//===========================================================================
ostream & Dim::operator<<(ostream & os, const Path & val) {
    os << val.view();
    return os;
}

//===========================================================================
Path Dim::operator/ (const Path & a, std::string_view b) {
    return Path{a} /= b;
}

//===========================================================================
Path Dim::operator+ (const Path & a, std::string_view b) {
    return Path{a} += b;
}
