// Copyright Glen Knowles 2018 - 2022.
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
    EnumFlags<Flags> flags{};
    vector<DirInfo> pos;
    Entry entry;
};

//===========================================================================
static bool match(const FileIter::Info & info) {
    if (info.entry.isdir) {
        if (info.flags.none(FileIter::fDirsFirst | FileIter::fDirsLast))
            return false;
    } else {
        if (info.flags.any(FileIter::fDirsOnly))
            return false;
    }

    if (info.flags.none(FileIter::fHidden)) {
        EnumFlags<File::Attrs> attrs;
        if (auto ec = fileAttrs(&attrs, info.entry.path.view()); ec)
            return false;
        if (attrs.any(File::Attrs::fHidden))
            return false;
    }

    // TODO: check if info.entry.path matches info.path
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
    entry->isbefore = entry->isdir && firstPass;
    fileLastWriteTime(&entry->mtime, entry->path);
}

//===========================================================================
static bool find(FileIter::Info * info, bool fromNext) {
    auto cur = &info->pos.back();
    auto p = cur->iter;
    error_code ec;

    if (!fromNext)
        goto CHECK_CURRENT;

    if (cur->firstPass
        && info->flags.any(FileIter::fDirsFirst)
        && info->entry.isdir
    ) {
ENTER_DIR:
        // We were at the directory, now start it's contents
        auto q = fs::directory_iterator(*p, ec);
        if (q == end(q)) {
            // No contents, skip over pushing and popping the dir info stack 
            // and go straight to processing the exit.
            goto DIR_EXITED;
        }
        info->pos.push_back({q});
        cur = &info->pos.back();
        p = q;
        goto CHECK_CURRENT;
    }

TRY_NEXT:
    p = cur->iter.increment(ec);
    if (ec)
        p = end(p);
    cur->firstPass = true;

CHECK_CURRENT:
    if (p == end(p)) {
        // No more files in directory, move up to parent.
        info->pos.pop_back();
        if (info->pos.empty()) {
            // Already at search root, return false (end of search).
            return false;
        }
        cur = &info->pos.back();
        p = cur->iter;

DIR_EXITED:
        if (info->flags.any(FileIter::fDirsLast)) {
            // Always return a directory when leaving it if fDirsLast is
            // defined. No validation is required, it was checked to be 
            // desired before it was entered.
            cur->firstPass = false;
            copy(&info->entry, p, cur->firstPass);
            return true;
        }
        goto TRY_NEXT;
    }

    // check filter
    copy(&info->entry, p, cur->firstPass);
    if (!match(*info)) 
        goto TRY_NEXT;
    if (info->entry.isdir && info->flags.none(FileIter::fDirsFirst)) {
        // Not reporting directories when entered, so rather than reporting 
        // it, start searching it's contents.
        goto ENTER_DIR;
    }
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
    EnumFlags<FileIter::Flags> flags
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
