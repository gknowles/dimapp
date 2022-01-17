// Copyright Glen Knowles 2018 - 2019.
// Distributed under the Boost Software License, Version 1.0.
//
// search.cpp - dim file
#include "pch.h"
#pragma hdrstop

using namespace std;
using namespace Dim;
namespace fs = std::filesystem;


/****************************************************************************
*
*   FileIter::Info
*
***/

namespace {
    struct DirInfo {
        fs::directory_iterator iter;
        bool firstPass{true};
    };
} // namespace

struct FileIter::Info {
    Path path;
    Flags flags{};
    vector<DirInfo> pos;
    Entry entry;
};

//===========================================================================
static bool match(const FileIter::Info & info) {
    if (info.entry.isdir) {
        if ((info.flags & (FileIter::fDirsFirst | FileIter::fDirsLast)) == 0)
            return false;
    } else {
        if (info.flags & FileIter::fDirsOnly)
            return false;
    }

    // TODO: check pattern
    return true;
}

//===========================================================================
static void copy(
    FileIter::Entry * entry, 
    fs::directory_iterator p, 
    bool firstPass
) {
    entry->path = p->path();
    entry->isdir = fs::is_directory(p->status());
    entry->isbefore = firstPass;
    entry->mtime = fileLastWriteTime(entry->path);
}

//===========================================================================
static bool find(FileIter::Info * info, bool fromNext) {
    auto cur = &info->pos.back();
    auto p = cur->iter;

    if (!fromNext)
        goto CHECK_CURRENT;

    if (cur->firstPass
        && (info->flags & FileIter::fDirsFirst)
        && fs::is_directory(p->status())
    ) {
ENTER_DIR:
        // we were at the directory, now start it's contents
        error_code ec;
        auto q = fs::directory_iterator(*p, ec);
        if (q == end(q))
            goto EXITED_DIR;
        info->pos.push_back({q});
        cur = &info->pos.back();
        p = q;
        goto CHECK_CURRENT;
    }

TRY_NEXT:
    p = ++cur->iter;
    cur->firstPass = true;

CHECK_CURRENT:
    // make for valid entry
    if (p == end(p)) {
        info->pos.pop_back();
        if (info->pos.empty())
            return false;
        cur = &info->pos.back();
        p = cur->iter;

EXITED_DIR:
        if (info->flags & FileIter::fDirsLast) {
            cur->firstPass = false;
            copy(&info->entry, p, cur->firstPass);
            return true;
        }
        goto TRY_NEXT;
    }

    // check filter
    if (!(info->flags & FileIter::fDirsFirst)
        && fs::is_directory(p->status())
    ) {
        goto ENTER_DIR;
    }
    copy(&info->entry, p, cur->firstPass);
    if (!match(*info))
        goto TRY_NEXT;

    return true;
}


/****************************************************************************
*
*   FileIter
*
***/

//===========================================================================
FileIter::FileIter(
    string_view dir,
    string_view name,
    FileIter::Flags flags
) {
    m_info = make_shared<Info>();
    m_info->path = dir;
    m_info->path /= name.empty() ? "*" : name;
    m_info->flags = flags;
    error_code ec;
    auto path = Path{m_info->path.parentPath()};
    auto it = fs::directory_iterator{path.empty() ? "." : path.fsPath(), ec};
    if (it == end(it)) {
        m_info.reset();
    } else {
        m_info->pos.push_back({it});
        if (!find(m_info.get(), false))
            m_info.reset();
    }
}

//===========================================================================
const FileIter::Entry & FileIter::operator* () const {
    return m_info->entry;
}

//===========================================================================
const FileIter::Entry * FileIter::operator-> () const {
    return &m_info->entry;
}

//===========================================================================
FileIter & FileIter::operator++ () {
    assert(m_info);
    if (!find(m_info.get(), true))
        m_info.reset();
    return *this;
}
