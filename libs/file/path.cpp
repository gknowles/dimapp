// Copyright Glen Knowles 2017 - 2019.
// Distributed under the Boost Software License, Version 1.0.
//
// path.cpp - dim file
#include "pch.h"
#pragma hdrstop

using namespace std;
using namespace Dim;
namespace fs = std::filesystem;


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

enum PrevCharType {
    kNone,
    kNormal,
    kSlash,
    kDot,
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
static bool normalizeAtDotSlash(
    string * out,
    Count const & cnt
) {
    auto pos = out->size();
    auto eptr = out->data() + pos;
    if (!pos)
        return true;
    if (pos == 1) {
        out->pop_back();
        return false;
    }
    if (eptr[-2] == '/') {
        // "<prev>/." + "/" => "<prev>/"
        out->pop_back();
        return false;
    }
    if (eptr[-2] == '.' && pos >= 3 && eptr[-3] == '/') {
        // "<prev>/.." + "/"
        auto * obase = out->data() + cnt.m_rootLen;
        auto * slash = out->data() + out->size() - 4;
        if (slash < obase) {
            out->pop_back();
            out->pop_back();
            return false;
        }
        for (;; --slash) {
            if (slash <= obase) {
                slash = obase - 1;
            } else if (*slash != '/') {
                continue;
            }
            break;
        }
        // pop unless previous segment is also ..
        if (slash + 3 != eptr - 3
            || slash[1] != '.' || slash[2] != '.'
        ) {
            out->resize(slash - out->data() + 1);
            return false;
        }
    }
    return true;
}

//===========================================================================
static void normalizeAtSlash(
    string * out,
    PrevCharType * prevChar,
    Count const & cnt,
    bool pseudoSlash = false
) {
    auto pchar = *prevChar;
    *prevChar = kSlash;
    switch (pchar) {
    case kSlash:
        return;
    case kDot:
        pseudoSlash = !normalizeAtDotSlash(out, cnt) || pseudoSlash;
        break;
    case kNone:
    case kNormal:
        break;
    }
    if (!pseudoSlash)
        *out += '/';
}

//===========================================================================
static void normalize(string * path) {
    Count cnt(*path);
    string out;
    out.reserve(path->size());
    out.append(*path, 0, cnt.m_rootLen);
    auto * ptr = path->data() + cnt.m_rootLen;
    auto * eptr = ptr + cnt.m_dirLen + cnt.m_stemLen + cnt.m_extLen;
    PrevCharType prevChar{kNone};
    for (; ptr != eptr; ++ptr) {
        switch(*ptr) {
        case '/':
        case '\\':
            normalizeAtSlash(&out, &prevChar, cnt);
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
    if (prevChar == kDot && !cnt.m_extLen) {
        if (cnt.m_stemLen == 1
            || cnt.m_stemLen == 2 && path->data()[path->size() - 2] == '.'
        ) {
            normalizeAtSlash(&out, &prevChar, cnt, true);
        }
    }
    // remove trailing slash if it's not also the leading slash
    if (prevChar == kSlash && cnt.m_dirLen > 1)
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
    char const * colon = nullptr;    // first ':'
    char const * slash = nullptr;    // last '/' or '\'
    char const * dot = nullptr;      // last '.'
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
        if (m_extLen == 1 && m_stemLen == 1 && dot[-1] == '.') {
            m_stemLen = 2;
            m_extLen = 0;
        } else if (!m_stemLen) {
            m_stemLen = m_extLen;
            m_extLen = 0;
        }
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
Path::Path(fs::path const & from) {
    assign(from);
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
Path & Path::assign(Path const & path) {
    m_data = path.m_data;
    return *this;
}

//===========================================================================
Path & Path::assign(Path const & path, string_view defExt) {
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
Path & Path::assign(fs::path const & path) {
    auto tmp = path.generic_u8string();
    return assign(string_view{(char *) tmp.data(), tmp.size()});
}

//===========================================================================
Path & Path::assign(fs::path const & path, string_view defExt) {
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
    Count scnt(filename);
    filename.remove_prefix(scnt.m_rootLen + scnt.m_dirLen);
    if (filename.empty()) {
        m_data.resize(cnt.m_rootLen + cnt.m_dirLen - (cnt.m_dirLen > 1));
    } else {
        m_data.resize(cnt.m_rootLen + cnt.m_dirLen);
        addStem(&m_data, filename.substr(0, scnt.m_stemLen));
        addExt(&m_data, filename.substr(scnt.m_stemLen));
    }
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
Path & Path::resolve(Path const & base) {
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
        if (hasDir() || hasFilename()) {
            if (base.hasDir() || base.hasFilename())
                out += '/';
            out += c_str() + drive().size();
        }
    }
    return assign(out);
}

//===========================================================================
fs::path Path::fsPath() const {
    return fs::path(
        (char8_t *) m_data.data(),
        (char8_t *) m_data.data() + m_data.size()
    );
}

//===========================================================================
char const * Path::c_str() const {
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
    return {m_data.data(), cnt.m_rootLen + cnt.m_dirLen - (cnt.m_dirLen > 1)};
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
bool Dim::operator==(Path const & left, string_view right) {
    return left.view() == right;
}

//===========================================================================
bool Dim::operator==(string_view left, Path const & right) {
    return left == right.view();
}

//===========================================================================
bool Dim::operator==(Path const & left, Path const & right) {
    return left.view() == right.view();
}

//===========================================================================
ostream & Dim::operator<<(ostream & os, Path const & val) {
    os << val.view();
    return os;
}

//===========================================================================
Path Dim::operator/ (Path const & a, std::string_view b) {
    return Path{a} /= b;
}

//===========================================================================
Path Dim::operator+ (Path const & a, std::string_view b) {
    return Path{a} += b;
}
