// Copyright Glen Knowles 2018 - 2024.
// Distributed under the Boost Software License, Version 1.0.
//
// search.cpp - dim file
#include "pch.h"
#pragma hdrstop

using namespace std;
using namespace Dim;
using enum Glob::Mode;
namespace fs = std::filesystem;


/****************************************************************************
*
*   FileIter::Info
*
***/

namespace {
    struct DirInfo {
        fs::directory_iterator iter;
        bool firstPass = true;
    };
} // namespace

struct Glob::Iter::Info {
    Path path;
    EnumFlags<Glob::Mode> flags = {};
    vector<DirInfo> pos;
    Glob::Entry entry;
    shared_ptr<Glob::Info> glob;
    Search search;
};

//===========================================================================
static bool match(Glob::Iter::Info * info) {
    if (!info->entry.isdir && info->flags.any(fDirsOnly))
        return false;
    if (!info->flags.any(fHidden)) {
        // Not including hidden files.
        EnumFlags<File::Attrs> attrs;
        if (auto ec = fileAttrs(&attrs, info->entry.path.view()); ec) {
            // Can't determine if it's a hidden file, exclude it.
            return false;
        }
        if (attrs.any(File::Attrs::fHidden)) {
            // It is a hidden file, exclude it.
            return false;
        }
    }

    auto res = Glob::matchSearch(
        &info->search,
        info->entry.path.filename(),
        !info->entry.isdir
    );
    return res != Glob::kNoMatch;
}

//===========================================================================
static void copy(
    Glob::Entry * entry,
    fs::directory_iterator p,
    bool firstPass
) {
    entry->path = p->path();
    entry->isdir = fs::is_directory(p->status());
    entry->isbefore = entry->isdir && firstPass;
    fileLastWriteTime(&entry->mtime, entry->path);
}

//===========================================================================
static bool find(Glob::Iter::Info * info, bool fromNext) {
    auto cur = &info->pos.back();
    auto p = cur->iter;
    error_code ec;

    if (!fromNext)
        goto CHECK_CURRENT;

    if (cur->firstPass
        && info->flags.any(fDirsFirst)
        && info->entry.isdir
    ) {
ENTER_DIR:
        // We were at the directory, now start it's contents.
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
        Glob::popSearchSeg(&info->search);
        info->pos.pop_back();
        if (info->pos.empty()) {
            // Already at search root, return false (end of search).
            return false;
        }
        cur = &info->pos.back();
        p = cur->iter;

DIR_EXITED:
        if (info->flags.any(fDirsLast)) {
            // Always return the directory when leaving it if fDirsLast is
            // defined. No pattern match required, it was matched before it was
            // entered.
            cur->firstPass = false;
            copy(&info->entry, p, cur->firstPass);
            return true;
        }
        goto TRY_NEXT;
    }

    // Check filter.
    copy(&info->entry, p, cur->firstPass);
    if (!match(info))
        goto TRY_NEXT;
    if (info->entry.isdir && !info->flags.any(fDirsFirst)) {
        // Not reporting directories when entered, so rather than reporting
        // it, start searching it's contents.
        goto ENTER_DIR;
    }
    return true;
}


/****************************************************************************
*
*   Glob::Iter
*
***/

//===========================================================================
Glob::Iter::Iter(shared_ptr<Info> info)
    : m_info(info)
{}

//===========================================================================
const Glob::Entry & Glob::Iter::operator* () const {
    return m_info->entry;
}

//===========================================================================
const Glob::Entry * Glob::Iter::operator-> () const {
    return &m_info->entry;
}

//===========================================================================
Glob::Iter & Glob::Iter::operator++ () {
    assert(m_info);
    if (!find(m_info.get(), true))
        m_info.reset();
    return *this;
}


/****************************************************************************
*
*   Public API
*
***/

//===========================================================================
Glob::Iter Dim::fileGlob(
    string_view root,
    string_view pattern,
    EnumFlags<Glob::Mode> flags,
    Glob::GlobType type
) {
    auto info = make_shared<Glob::Iter::Info>();
    info->path = root;
    info->flags = flags;
    info->glob = make_shared<Glob::Info>();
    if (!Glob::parse(info->glob.get(), pattern.empty() ? "*" : pattern))
        return Glob::Iter{};
    error_code ec;
    auto p = fs::directory_iterator{
        info->path.empty() ? "." : info->path.fsPath(),
        ec
    };
    if (p == end(p))
        return Glob::Iter{};
    info->pos.push_back({p});
    info->search = newSearch(*info->glob);
    if (!find(info.get(), false))
        return Glob::Iter{};

    return Glob::Iter{info};
}
