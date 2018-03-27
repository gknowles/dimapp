// Copyright Glen Knowles 2018.
// Distributed under the Boost Software License, Version 1.0.
//
// search.cpp - dim file
#include "pch.h"
#pragma hdrstop

using namespace std;
using namespace Dim;
namespace fs = std::experimental::filesystem;


/****************************************************************************
*
*   Private
*
***/

namespace {


} // namespace


/****************************************************************************
*
*   FileIterator::Info
*
***/

namespace {
    struct DirInfo {
        fs::directory_iterator iter;
        bool firstPass{true};
    };
} // namespace

struct FileIterator::Info {
    Path path;
    Flags flags;
    vector<DirInfo> pos;
    Entry entry;
};

//===========================================================================
static bool match(const FileIterator::Info & info) {
    if (info.entry.isdir) {
        if (info.flags & FileIterator::fDirsOnly)
            return false;
    } else {
        if (~info.flags & (FileIterator::fDirsFirst | FileIterator::fDirsLast))
            return false;
    }

    // TODO: check pattern
    return true;
}

//===========================================================================
static void copy(FileIterator::Entry * entry, fs::directory_iterator p) {
    entry->path = p->path();
    entry->isdir = fs::is_directory(p->status());
}


/****************************************************************************
*
*   FileIterator
*
***/

//===========================================================================
FileIterator::FileIterator(
    string_view dir,
    string_view name,
    FileIterator::Flags flags
) {
    m_info = make_shared<Info>();
    m_info->path = dir.empty() ? "." : dir;
    m_info->path /= name.empty() ? "*" : name;
    m_info->flags = flags;
    error_code ec;
    auto it = fs::directory_iterator{m_info->path.fsPath(), ec};
    if (it != end(it)) {
        m_info->pos.push_back({it});
        m_info->entry.path = *it;
    }
}

//===========================================================================
bool FileIterator::operator== (const FileIterator & right) const {
    return m_info == right.m_info;
}

//===========================================================================
bool FileIterator::operator!= (const FileIterator & right) const {
    return m_info != right.m_info;
}

//===========================================================================
const FileIterator::Entry & FileIterator::operator* () const {
    return m_info->entry;
}

//===========================================================================
const FileIterator::Entry * FileIterator::operator-> () const {
    return &m_info->entry;
}

//===========================================================================
FileIterator & FileIterator::operator++ () {
    auto pos = &m_info->pos.back();
    auto p = pos->iter;

    if (pos->firstPass
        && (m_info->flags & fDirsFirst)
        && fs::is_directory(p->status())
    ) {
STEP_INTO:
        // we were at the directory, now start it's contents
        error_code ec;
        auto q = fs::directory_iterator(*p, ec);
        if (q == end(q))
            goto TRY_NEXT;
        m_info->pos.push_back({q});
        pos = &m_info->pos.back();
        goto CHECK_CURRENT;
    }

TRY_NEXT:
    p = ++pos->iter;
    pos->firstPass = true;

CHECK_CURRENT:
    // make for valid entry
    if (p == end(p)) {
        m_info->pos.pop_back();
        if (m_info->pos.empty()) {
            m_info.reset();
            return *this;
        }
        pos = &m_info->pos.back();
        p = pos->iter;
        if (m_info->flags & fDirsLast) {
            pos->firstPass = false;
            copy(&m_info->entry, p);
            return *this;
        }
        goto TRY_NEXT;
    }

    // check filter
    if (!(m_info->flags & fDirsFirst)
        && fs::is_directory(p->status())
    ) {
        goto STEP_INTO;
    }
    copy(&m_info->entry, p);
    if (!match(*m_info))
        goto CHECK_CURRENT;

    return *this;
}


/****************************************************************************
*
*   Public API
*
***/

