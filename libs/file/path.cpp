// Copyright Glen Knowles 2017 - 2024.
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
    string_view m_path;
    unsigned m_rootLen;     // includes trailing ':'
    unsigned m_dirLen;      // includes leading and/or trailing '/'
    unsigned m_stemLen;
    unsigned m_extLen;      // includes leading '.'

    explicit Count(string_view path);

    string_view drive() const;      // Through first ':' in first segment.
    string_view dir() const;        // Path without drive or filename.
    string_view parentPath() const; // Drive and dir.
    string_view filename() const;   // Last segment of path.
    string_view stem() const;       // Filename without extension.
    string_view extension() const;  // Portion of filename from last dot.

    bool hasDrive() const;
    bool hasRootDir() const;    // If dir not empty and starts with '/'.
    bool hasDir() const;
    bool hasFilename() const;
    bool hasStem() const;
    bool hasExt() const;

    string resolve(const Count & base) const;
    string relative(const Count & base) const;
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
// Returns true if the slash should be kept, otherwise the output path has
// been modified and the slash should be skipped.
static bool normalizeAtDotSlash(
    string * out,
    const Count & cnt
) {
    auto dirpos = out->size() - cnt.m_rootLen;
    assert(dirpos > 0); // Must have leading dot to call this function.
    auto eptr = out->data() + out->size();

    // Handle current directory self reference (single dot).
    if (dirpos == 1 || eptr[-2] == '/') {
        // Either: "." + "/" => ""
        //     Or: "<prev>/." + "/" => "<prev>/"
        out->pop_back();
        return false;
    }

    // Handle parent directory reference (double dot).
    if (dirpos >= 3 && eptr[-2] == '.' && eptr[-3] == '/') {
        // "<prev>/.." + "/"

        if (dirpos == 3) {
            // "/.."
            out->resize(out->size() - 2);
            return false;
        }

        // Find slash at start of segment before the current ".." one.
        auto * obase =  // First place slash could be.
            out->data() + cnt.m_rootLen;

        // Last possible place for slash. The segment closing slash is at -3,
        // so start just before that.
        auto * slash = out->data() + out->size() - 4;
        for (; slash >= obase; --slash) {
            if (*slash == '/')
                break;
        }

        // Keep new ".." segment if the previous segment is also "..".
        if (slash + 3 == eptr - 3
            && slash[1] == '.' && slash[2] == '.'
        ) {
            return true;
        }

        // Remove previous path segment and current ".." segment.
        out->resize(slash - out->data() + 1);
        return false;
    }

    return true;
}

//===========================================================================
static void normalizeAtSlash(
    string * out,
    PrevCharType * prevChar,
    const Count & cnt,
    bool addSlash = true
) {
    auto pchar = *prevChar;
    *prevChar = kSlash;
    switch (pchar) {
    case kSlash:
        // Consecutive slashes after the first are discarded.
        return;
    case kDot:
        addSlash = normalizeAtDotSlash(out, cnt) && addSlash;
        break;
    case kNone:
    case kNormal:
        break;
    }
    if (addSlash)
        *out += '/';
}

//===========================================================================
static void normalize(string * path) {
    Count cnt(*path);
    string out;
    out.reserve(path->size());
    out.append(*path, 0, cnt.m_rootLen);

    // Only normalize the portion following the root name.
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

    // Check if last segment is "." or "..", and if it is process as if there's
    // a following slash so it's interpreted as the path modifying pseudo
    // directory that it is.
    if (prevChar == kDot && !cnt.m_extLen) {
        if (cnt.m_stemLen == 1
            || cnt.m_stemLen == 2 && path->data()[path->size() - 2] == '.'
        ) {
            normalizeAtSlash(
                &out,
                &prevChar,
                cnt,
                false    // add slash?
            );
        }
    }

    // remove trailing slash if it's not also the leading slash
    if (out.size() > cnt.m_rootLen + 1 && out.back() == '/')
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
    m_path = path;
    auto * base = path.data();
    auto * ptr = base;
    auto * eptr = ptr + path.size();
    const char * colon = nullptr;    // first ':'
    const char * slash = nullptr;    // last '/' or '\'
    const char * dot = nullptr;      // last '.'
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

//===========================================================================
string_view Count::drive() const {
    return {m_path.data(), m_rootLen};
}

//===========================================================================
string_view Count::dir() const {
    return {m_path.data() + m_rootLen, m_dirLen};
}

//===========================================================================
string_view Count::parentPath() const {
    return {m_path.data(), m_rootLen + m_dirLen - (m_dirLen > 1)};
}

//===========================================================================
string_view Count::filename() const {
   return {
        m_path.data() + m_rootLen + m_dirLen,
        m_stemLen + m_extLen
    };
}

//===========================================================================
string_view Count::stem() const {
    return {m_path.data() + m_rootLen + m_dirLen, m_stemLen};
}

//===========================================================================
string_view Count::extension() const {
    return {
        m_path.data() + m_rootLen + m_dirLen + m_stemLen,
        m_extLen
    };
}

//===========================================================================
bool Count::hasDrive() const {
    return m_rootLen;
}

//===========================================================================
bool Count::hasRootDir() const {
    return m_dirLen && m_path[m_rootLen] == '/';
}

//===========================================================================
bool Count::hasDir() const {
    return m_dirLen;
}

//===========================================================================
bool Count::hasFilename() const {
    return m_stemLen + m_extLen;
}

//===========================================================================
bool Count::hasStem() const {
    return m_stemLen;
}

//===========================================================================
bool Count::hasExt() const {
    return m_extLen;
}

//===========================================================================
string Count::resolve(const Count & base) const {
    string out;

    if (hasRootDir()) {
        if (!hasDrive()) {
            addRoot(&out, base.drive());
        }
        out += m_path;
    } else {
        if (drive() != base.drive() && hasDrive()) {
            out += m_path;
        } else {
            out += base.m_path;
            if (hasDir() || hasFilename()) {
                if (base.hasDir() || base.hasFilename())
                    out += '/';
                out += m_path.data() + drive().size();
            }
        }
    }
    return out;
}

//===========================================================================
string Count::relative(const Count & base) const {
    string out;

    if (drive() != base.drive()) {
        out += m_path;
        return out;
    }
    if (hasRootDir() != base.hasRootDir()) {
        if (hasRootDir()) {
            out += m_path.data() + drive().size();
        } else {
            out = m_path;
        }
    } else {
        // Find first segment difference (skip leading common segments).
        int segDiff = m_rootLen - 1;
        int i = m_rootLen;
        auto last = min(m_path.size(), base.m_path.size());
        for (;; ++i) {
            if (i == last) {
                if (m_path.size() > last && m_path[last] == '/'
                    || base.m_path.size() > last && base.m_path[last] == '/'
                ) {
                    // The end of the shorter path is also the end of a segment
                    // in the longer, therefore it is a common segment.
                    segDiff = i;
                }
                break;
            }
            if (m_path[i] != base.m_path[i])
                break;
            if (m_path[i] == '/')
                segDiff = i;
        }
        // Add ".." segment for each remaining base segments.
        if (i < base.m_path.size()) {
            out += "../";
            for (auto ch : base.m_path.substr(i)) {
                if (ch == '/')
                    out += "../";
            }
        }
        // Add each remaining segments from this.
        out += m_path.substr(segDiff + 1);
    }
    // No need to normalize, the caller will do that via assign.
    return out;
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
Path::Path(string && from)
    : m_data(move(from))
{
    normalize(&m_data);
}

//===========================================================================
Path::Path(const fs::path & from) {
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
    auto tmp = path.generic_u8string();
    return assign(string_view{(char *) tmp.data(), tmp.size()});
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
Path & Path::resolve(const Path & baseRaw) {
    Count cnt(m_data);
    Count base(baseRaw.m_data);
    auto out = cnt.resolve(base);
    return assign(out);
}

//===========================================================================
Path & Path::resolve(string_view base) {
    return resolve(Path(base));
}

//===========================================================================
Path & Path::resolve(const Path & base, string_view fallback) {
    resolve(base);
    if (m_data.size() >= base.size()) {
        auto ch = m_data[base.size()];
        if (m_data.starts_with(base.view()) && (ch == '\0' || ch == '/'))
            return *this;
    }
    assign(fallback);
    resolve(base);
    return *this;
}

//===========================================================================
Path & Path::resolve(string_view base, string_view fallback) {
    return resolve(Path{base}, fallback);
}

//===========================================================================
Path & Path::relative(const Path & baseRaw) {
    Count cnt(m_data);
    Count base(baseRaw.m_data);
    auto out = cnt.relative(base);
    return assign(out);
}

//===========================================================================
Path & Path::relative(std::string_view base) {
    return relative(Path(base));
}

//===========================================================================
fs::path Path::fsPath() const {
    return fs::path(
        (char8_t *) m_data.data(),
        (char8_t *) m_data.data() + m_data.size()
    );
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
string Path::preferredStr() const {
    string out;
    for (auto&& ch : m_data) {
        out += (ch == '/') ? '\\' : ch;
    }
    return out;
}

//===========================================================================
string_view Path::drive() const {
    return Count(m_data).drive();
}

//===========================================================================
string_view Path::dir() const {
    return Count(m_data).dir();
}

//===========================================================================
string_view Path::parentPath() const {
    return Count(m_data).parentPath();
}

//===========================================================================
string_view Path::filename() const {
    return Count(m_data).filename();
}

//===========================================================================
string_view Path::stem() const {
    return Count(m_data).stem();
}

//===========================================================================
string_view Path::extension() const {
    return Count(m_data).extension();
}

//===========================================================================
bool Path::empty() const {
    return m_data.empty();
}

//===========================================================================
bool Path::hasDrive() const {
    return Count(m_data).hasDrive();
}

//===========================================================================
bool Path::hasRootDir() const {
    return Count(m_data).hasRootDir();
}

//===========================================================================
bool Path::hasDir() const {
    return Count(m_data).hasDir();
}

//===========================================================================
bool Path::hasFilename() const {
    return Count(m_data).hasFilename();
}

//===========================================================================
bool Path::hasStem() const {
    return Count(m_data).hasStem();
}

//===========================================================================
bool Path::hasExt() const {
    return Count(m_data).hasExt();
}


/****************************************************************************
*
*   Free functions
*
***/

namespace Dim {
ostream& operator<< (ostream& os, const Path& val);
Path operator/ (const Path& a, string_view b);
Path operator+ (const Path& a, string_view b);
} // namespace

//===========================================================================
ostream & Dim::operator<< (ostream & os, const Path & val) {
    os << val.view();
    return os;
}

//===========================================================================
Path Dim::operator/ (const Path & a, string_view b) {
    return Path{a} /= b;
}

//===========================================================================
Path Dim::operator+ (const Path & a, string_view b) {
    return Path{a} += b;
}
