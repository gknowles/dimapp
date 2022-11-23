// Copyright Glen Knowles 2022.
// Distributed under the Boost Software License, Version 1.0.
//
// globast.cpp - dim file
#include "pch.h"
#pragma hdrstop

using namespace std;
using namespace Dim;
using namespace Dim::Glob;


/****************************************************************************
*
*   Declarations
*
***/

namespace Dim::Glob {

struct PartEmpty : Node {
    PartEmpty() { type = kPartEmpty; }
};
struct PartLiteral : Node {
    string_view val;
    PartLiteral() { type = kPartLiteral; }
};
struct PartBlot : Node {
    int count = 0;
    PartBlot() { type = kPartBlot; }
};
struct PartCharChoice : Node {
    bitset<256> vals;
    PartCharChoice() { type = kPartCharChoice; }
};
struct PartChoice : Node {
    List<Node> parts;
    PartChoice() { type = kPartChoice; }
};
struct PartList : Node {
    List<Node> parts;
    PartList() { type = kPartList; }
};

struct PathSeg : PartList {
    List<PathSeg> next;
    PathSegment pathSeg;
    PathSeg() { type = kPathSeg; }
};

} // namespace


/****************************************************************************
*
*   Glob::Info
*
***/

//===========================================================================
Glob::Info::~Info() {
    roots.unlinkAll();
}


/****************************************************************************
*
*   Querying abstract syntax tree
*
***/

//===========================================================================
vector<const PathSegment *> Dim::Glob::pathSegments(const Info & glob) {
    vector<const PathSegment *> out;
    auto node = glob.roots.front();
    while (node) {
        assert(node->type == kPathSeg);
        auto seg = static_cast<const PathSeg *>(node);
        out.push_back(&seg->pathSeg);
        node = seg->next.front();
        assert(node == seg->next.back());

    }
    return out;
}

//===========================================================================
NodeType Dim::Glob::getType(const Node & node) {
    return node.type;
}

static MatchResult matchNode(
    const List<Node> * parts,
    const Node * node,
    string_view val
);

//===========================================================================
static MatchResult matchNextNode(
    const List<Node> * parts,
    const Node * node,
    string_view val
) {
    node = parts ? parts->next(node) : nullptr;
    return matchNode(parts, node, val);
}

//===========================================================================
static MatchResult matchNode(
    const List<Node> * parts,
    const PartChoice * node,
    string_view val
) {
    if (!parts)
        return matchNode(parts, static_cast<const Node *>(node), val);

    auto & choices = node->parts;
    // TODO: stop at minimum required length of the following string
    for (int i = 0; i <= val.size(); ++i) {
        for (auto && sn : choices) {
            if (!matchNode(parts, &sn, val.substr(0, i)))
                continue;
            if (matchNextNode(parts, node, val.substr(i)))
                return kMatch;
        }
    }
    return kNoMatch;
}

//===========================================================================
static MatchResult matchNode(
    const List<Node> * parts,
    const Node * node,
    string_view val
) {
    auto type = node ? node->type : kPartEmpty;

    switch (type) {
    case kPartEmpty:
        return val.empty() ? kMatch : kNoMatch;

    case kPartLiteral:
    {
        auto & lit = static_cast<const PartLiteral *>(node)->val;
        auto len = lit.size();
        if (lit != val.substr(0, len))
            return kNoMatch;
        return matchNextNode(parts, node, val.substr(len));
    }

    case kPartBlot:
        // TODO: stop at minimum required length of the following string
        for (; !val.empty(); val.remove_prefix(1)) {
            if (matchNextNode(parts, node, val))
                return kMatch;
        }
        return matchNextNode(parts, node, val);

    case kPartDoubleBlot:
        return kMatchRest;

    case kPartCharChoice:
        if (val.empty()
            || !static_cast<const PartCharChoice *>(node)->vals.test(val[0])
        ) {
            return kNoMatch;
        }
        return matchNextNode(parts, node, val.substr(1));

    case kPartChoice:
        return matchNode(
            parts,
            static_cast<const PartChoice *>(node),
            val
        );

    case kPartList:
        //auto sn = static_cast<const
        //if (auto match = matchNode(
        return kNoMatch;

    default:
        assert(!"Not searchable path segment node type");
        return kNoMatch;
    }
}

//===========================================================================
MatchResult Dim::Glob::matchSegment(const PathSegment & seg, string_view val) {
    assert(seg.node->type == kPathSeg);
    auto sn = static_cast<const PathSeg *>(seg.node);
    return matchNode(&sn->parts, sn->parts.front(), val);
}

//===========================================================================
static bool IsLastSegment(const PathSegment & seg) {
    assert(seg.node->type == kPathSeg);
    auto sn = static_cast<const PathSeg *>(seg.node);
    return sn->next.empty();
}

//===========================================================================
Search Dim::Glob::newSearch(const Info & glob) {
    Search out;
    out.glob = &glob;
    out.current.push_back(nullptr);
    for (auto&& node : glob.roots) {
        auto seg = static_cast<const PathSeg *>(&node);
        out.current.push_back(&seg->pathSeg);
    }
    return out;
}

//===========================================================================
MatchResult Dim::Glob::matchSearch(
    Search * srch,
    string_view val,
    bool lastSeg
) {
    auto matches = 0;
    for (auto i = srch->current.size(); i > 0;) {
        auto seg = srch->current[--i];
        if (!seg)
            break;
        if (lastSeg != IsLastSegment(*seg))
            continue;
        if (auto match = matchSegment(*seg, val)) {
            if (lastSeg)
                return kMatch;
            if (match == kMatchRest) {
                // If a branch matches the rest, no other branches are needed.
                // All we care about is if it match, not how many different
                // ways it matches.
                if (matches) {
                    // Remove other branches as redundant.
                    srch->current.resize(srch->current.size() - matches);
                } else {
                    srch->current.push_back(nullptr);
                }
                srch->current.push_back(seg);
                return match;
            }
            if (!matches++)
                srch->current.push_back(nullptr);
            srch->current.push_back(seg);
        }
    }
    return matches ? kMatch : kNoMatch;
}

//===========================================================================
void Dim::Glob::popSearchSeg(Search * srch) {
    for (;;) {
        assert(!srch->current.empty());
        if (srch->current.back() == nullptr) {
            srch->current.pop_back();
            break;
        }
        srch->current.pop_back();
    }
}


/****************************************************************************
*
*   toString
*
***/

static void append(string * out, const Node & node, GlobType type, int indent);

//===========================================================================
static void append(
    string * out,
    const PartList & node,
    GlobType type,
    int indent
) {
    auto & sn = static_cast<const PartList &>(node);
    for (auto&& part : sn.parts)
        append(out, part, type, indent);
}

//===========================================================================
static void append(
    string * out,
    const Node & node,
    GlobType type,
    int indent
) {
    switch (node.type) {
    case NodeType::kPathSeg:
        {
            auto & sn = static_cast<const PathSeg &>(node);
            append(out, sn, type, indent);
            if (sn.next.empty())
                break;
            *out += '/';
            if (sn.next.front() == sn.next.back()) {
                append(
                    out,
                    static_cast<const Node &>(*sn.next.front()),
                    type,
                    indent + 2
                );
                break;
            }
            for (auto&& next : sn.next) {
                *out += '\n';
                out->append(indent + 2, ' ');
                append(
                    out,
                    static_cast<const Node &>(next),
                    type,
                    indent + 2
                );
            }
            break;
        }
    case NodeType::kPartEmpty:
        break;
    case NodeType::kPartLiteral:
        {
            auto & sn = static_cast<const PartLiteral &>(node);
            for (auto ch : sn.val) {
                if (ch == '?' || ch == '*'
                    || ch == '[' || ch == '/' || ch == '{'
                ) {
                    *out += '\\';
                }
                *out += ch;
            }
            break;
        }
    case NodeType::kPartBlot:
        {
            [[maybe_unused]] auto & sn = static_cast<const PartBlot &>(node);
            *out += "*";
            break;
        }
    case NodeType::kPartDoubleBlot:
        {
            [[maybe_unused]] auto & sn = static_cast<const PartBlot &>(node);
            assert(sn.count > 1);
            *out += "**";
            break;
        }
    case NodeType::kPartCharChoice:
        {
            auto & sn = static_cast<const PartCharChoice &>(node);
            auto cnt = sn.vals.count();
            if (cnt == 256) {
                *out += '?';
                break;
            }
            auto vals = sn.vals;
            *out += '[';
            if (cnt > 128) {
                vals.flip();
                *out += '^';
            }
            for (auto pos = 0; pos < vals.size(); ++pos) {
                if (!vals.test(pos))
                    continue;
                auto epos = pos + 1;
                for (; epos < vals.size(); ++epos) {
                    if (!vals.test(epos))
                        break;
                }
                if (pos == '\\' || pos == ']' || pos == '-'
                    || pos == '{' || pos == '}'
                ) {
                    *out += '\\';
                }
                *out += (char) pos;
                if (pos + 1 < epos) {
                    if (pos + 2 < epos)
                        *out += '-';
                    if (epos + 1 == '\\' || epos + 1 == ']' || epos + 1 == '-')
                        *out += '\\';
                    *out += (char) epos - 1;
                }
                pos = epos - 1;
            }
            *out += ']';
            break;
        }
    case NodeType::kPartChoice:
        {
            auto & sn = static_cast<const PartChoice &>(node);
            *out += '\n';
            out->append(indent + 2, ' ');
            for (auto&& part : sn.parts) {
                append(out, part, type, indent + 2);
            }
            break;
        }
    case NodeType::kPartList:
        {
            auto & sn = static_cast<const PartList &>(node);
            append(out, sn, type, indent);
            break;
        }
    }
}

//===========================================================================
string Dim::Glob::toString(const Node & node, GlobType type) {
    string out;
    append(&out, node, type, 0);
    return out;
}

//===========================================================================
string Dim::Glob::toString(const Info & glob) {
    string out;
    auto node = glob.roots.front();
    if (!node) {
        out = "<EMPTY>";
        return out;
    }
    for (;;) {
        out += toString(*node, glob.type);
        node = glob.roots.next(node);
        if (!node)
            break;
        out += '\n';
    }
    return out;
}


/****************************************************************************
*
*   Abstract syntax tree builders
*
***/

static Node * copyNode(Info * glob, Node * node);

//===========================================================================
inline static void removeRedundantSegments(PathSeg * root) {
    // Double blot segments are redundant unless separated by at least one
    // non-blot segment.
    auto node = static_cast<PathSeg *>(root->parts.front());
    while (auto next = static_cast<PathSeg *>(root->parts.next(node))) {
        if (node->parts.front()->type == kPartDoubleBlot) {
            while (next->parts.size() == 1
                && next->parts.front()->type == kPartBlot
            ) {
                next = static_cast<PathSeg *>(root->parts.next(next));
                if (!next)
                    return;
            }
            if (next->parts.front()->type == kPartDoubleBlot)
                root->parts.unlink(node);
        }
        node = next;
    }
}

//===========================================================================
static void makePathSegment(Info * glob, PathSeg * seg) {
    auto & ps = seg->pathSeg;
    ps.node = seg;
    auto & sn = static_cast<const PathSeg &>(*seg);
    auto len = sn.parts.size();
    if (len == 0) {
        ps.type = kEndMark;
    } else if (len > 1) {
        ps.type = kCondition;
    } else {
        assert(len == 1);
        auto node = sn.parts.front();
        switch (node->type) {
        case kPartBlot:
            ps.type = kAny;
            break;
        case kPartDoubleBlot:
            ps.type = kDynamicAny;
            break;
        case kPartLiteral:
            ps.type = kExact;
            ps.prefix = static_cast<const PartLiteral *>(node)->val;
            break;
        default:
            ps.type = kCondition;
            break;
        }
    }
}

//===========================================================================
Node * Dim::Glob::addRootSeg(Info * glob) {
    auto seg = glob->heap.emplace<PathSeg>();
    glob->roots.link(seg);
    return seg;
}

//===========================================================================
Node * Dim::Glob::addSeg(Info * glob, Node * node) {
    auto seg = glob->heap.emplace<PathSeg>();
    if (auto parent = static_cast<PathSeg *>(node)) {
        assert(node->type == kPathSeg);
        seg->parent = parent;
        parent->next.link(seg);
    } else {
        glob->roots.link(seg);
    }
    return seg;
}

//===========================================================================
void Dim::Glob::endSeg(Info * glob, Node * node) {
    assert(node->type == kPathSeg);
    auto seg = static_cast<PathSeg *>(node);
    if (seg->parts.size() == 1
        && seg->parts.front()->type == kPartBlot
    ) {
        auto sn = static_cast<PartBlot *>(seg->parts.front());
        if (sn->count == 2)
            sn->type = kPartDoubleBlot;
    }
    makePathSegment(glob, seg);
}

//===========================================================================
Node * Dim::Glob::addPartEmpty(Info * glob, Node * node) {
    assert(node->type == kPathSeg);
    auto seg = static_cast<PathSeg *>(node);
    auto sn = glob->heap.emplace<PartEmpty>();
    sn->parent = seg;
    seg->parts.link(sn);
    return sn;
}

//===========================================================================
Node * Dim::Glob::addPartLiteral(Info * glob, Node * node, string_view val) {
    assert(node->type == kPathSeg);
    auto seg = static_cast<PathSeg *>(node);
    auto sn = glob->heap.emplace<PartLiteral>();
    sn->parent = seg;
    seg->parts.link(sn);
    sn->val = glob->heap.viewDup(val);
    return sn;
}

//===========================================================================
Node * Dim::Glob::addPartBlot(Info * glob, Node * node) {
    assert(node->type == kPathSeg);
    auto seg = static_cast<PathSeg *>(node);
    if (seg->parts && seg->parts.back()->type == kPartBlot) {
        auto sn = static_cast<PartBlot *>(seg->parts.back());
        sn->count += 1;
        return nullptr;
    }
    glob->pathType = kCondition;
    auto sn = glob->heap.emplace<PartBlot>();
    sn->parent = seg;
    seg->parts.link(sn);
    sn->count = 1;
    return sn;
}

//===========================================================================
Node * Dim::Glob::addPartCharChoice(
    Info * glob,
    Node * node,
    bitset<256> && vals
) {
    assert(node->type == kPathSeg);
    if (auto cnt = vals.count(); cnt < 2) {
        if (!cnt)
            return nullptr;
        auto lit = glob->heap.alloc<char>(1);
        for (int i = 0; i < vals.size(); ++i) {
            if (vals.test(i)) {
                *lit = (unsigned char) i;
                return addPartLiteral(glob, node, string_view{lit, 1});
            }
        }
    }

    glob->pathType = kCondition;
    auto seg = static_cast<PathSeg *>(node);
    auto sn = glob->heap.emplace<PartCharChoice>();
    sn->parent = seg;
    seg->parts.link(sn);
    sn->vals = move(vals);
    return sn;
}

//===========================================================================
Node * Dim::Glob::addPartChoice(Info * glob, Node * node) {
    assert(node->type == kPathSeg);
    glob->pathType = kCondition;
    auto seg = static_cast<PathSeg *>(node);
    auto sn = glob->heap.emplace<PartChoice>();
    sn->parent = seg;
    seg->parts.link(sn);
    return sn;
}

//===========================================================================
template<typename T>
static T * copyNode(Info * glob, Node * raw) {
    auto src = static_cast<T *>(raw);
    auto sn = glob->heap.emplace<T>();
    if constexpr (is_same_v<T, PathSeg>
        || is_same_v<T, PartList>
        || is_same_v<T, PartChoice>
    ) {
        for (auto&& node : src->parts) {
            auto cp = copyNode(glob, &node);
            cp->parent = sn;
            sn->parts.link(cp);
        }
    } else if constexpr (is_same_v<T, PartCharChoice>) {
        sn->vals = src->vals;
    } else if constexpr (is_same_v<T, PartBlot>) {
        sn->count = src->count;
    } else if constexpr (is_same_v<T, PartLiteral>) {
        sn->val = src->val;
    } else if constexpr (is_same_v<T, PartEmpty>) {
        // nothing to do, it's empty after all
    } else {
        unreachable();
    }
    return sn;
}

//===========================================================================
static Node * copyNode(Info * glob, Node * node) {
    switch (node->type) {
    case kPathSeg: return copyNode<PathSeg>(glob, node);
    case kPartEmpty: return copyNode<PartEmpty>(glob, node);
    case kPartLiteral: return copyNode<PartLiteral>(glob, node);
    case kPartBlot: return copyNode<PartBlot>(glob, node);
    case kPartCharChoice: return copyNode<PartCharChoice>(glob, node);
    case kPartChoice: return copyNode<PartChoice>(glob, node);
    case kPartList: return copyNode<PartList>(glob, node);
    default:
    case kPartDoubleBlot:
        unreachable();
    }
}


/****************************************************************************
*
*   Parsing
*
***/

//===========================================================================
// POSIX
//
// 2.13 Pattern Matching Notation
//  https://pubs.opengroup.org/onlinepubs/009695399/utilities/xcu_chap02.html#tag_02_13
// 9.3.5 RE Bracket Expression
//  https://pubs.opengroup.org/onlinepubs/009695399/basedefs/xbd_chap09.html#tag_09_03_05
//
// ? (not between brackets) - matches any single character, except '/', and
//      except '.' at the start of a path segment.
// * (not between brackets) - matches series of zero or more characters, except
//      '/', and except '.' at the start of a path segment.
// [ (not between brackets) - starts a bracket expression, but only if there is
//      a ']' within the path segment to close the expression. Otherwise it is
//      a regular character that matches a literal '['.
//
// The path separator '/', is matched only by one or more explicit slashes, it
// is not matched by ?, *, or bracket expressions. It also takes precedence
// over bracket expressions.


/****************************************************************************
*
*   Parsing Ruby pattern
*
*
*   Dir.glob - https://ruby-doc.org/core-2.6.3/Dir.html#method-c-glob
*   File::Constants (for FNM_* constants)
*    https://ruby-doc.org/core-2.6.3/File/Constants.html
*    FNM_CASEFOLD - ignored for Dir.glob
*    FNM_DOTMATCH - causes '*' to match filenames starting with '.'
*    FNM_EXTGLOB - enabled for Dir.glob
*    FNM_NOESCAPE - disables '\' as an escape character
*   Regexp character classes
*    https://ruby-doc.org/core-2.6.3/Regexp.html#class-Regexp-label-Character+Classes
*
*   Metacharacters:
*    * - Matches zero or more characters of a single file or directory name.
*        But, does not match '/' character or leading '.' character.
*    ** - Like *, but also matches '/' (i.e. directories recursively).
*         TBD: Must be followed by '/'? Or only thing in path seg?
*    ? - Matches one character, except '/' and leading '.'.
*    [set] - Matches any one character in set, except '/'.
*    {p,q} - Matches literal p or q. May have any number of literals.
*        Literals may be of any length, be nested, and contain '/'. Applied
*        before other constructs (except the escape '\' character).
*    \ - Escapes the next character.
*
*   A [set] is:
*   - Delimited by square brackets
*   - Starting with a caret '^' or bang '!' inverts the class; it matches any
*     character except those in the set.
*   - Hyphen that is not the first or last character denotes an inclusive
*     range ([abcd] is equivalent to [a-d]).
*
***/

namespace {

enum class ParseMode { kInvalid, kLiteral, kSet };
struct ParsePos {
    size_t pos = 0;
    const char * ptr = {};
    const char * eptr = {};

    explicit operator bool () const { return ptr < eptr; }
};
struct SetDetail {
    ParsePos pos;
    bitset<256> vals;
    bool exclude = {};
    char prev = 0;
    char dash = 0;
};
struct ChoiceDetail {
    vector<string_view> strs;
    Glob::PathSeg * seg = {};
};
struct ParseDetail {
    ParseMode mode = {};
    Glob::PathSeg * parentSeg = {};
    Glob::PathSeg * seg = {};
    ParsePos pos;
    string lit;
    SetDetail set;
};
struct ChoiceList {
    ParseDetail choice;
    deque<ChoiceDetail> details;
    vector<string_view> tail;
};
struct ParseState : ParseDetail {
    Info * glob = {};
    vector<string_view> strs;
    vector<ChoiceList> choices;
};

} // namespace

// Forward declarations
static bool addChoiceList(ParseState * st);

//===========================================================================
static void flushLiteral(ParseState * st) {
    if (!st->lit.empty()) {
        addPartLiteral(st->glob, st->seg, st->lit);
        st->lit.clear();
    }
}

//===========================================================================
static char nextRawChar(ParseState * st) {
    assert(st->pos.ptr < st->pos.eptr);
    char ch = *st->pos.ptr++;
    if (st->pos.ptr == st->pos.eptr) {
        if (st->pos.pos + 1 < st->strs.size()) {
            auto str = st->strs[++st->pos.pos];
            st->pos.ptr = str.data();
            st->pos.eptr = st->pos.ptr + str.size();
        }
    }
    return ch;
}

//===========================================================================
static void addChoice(ParseState * st, ChoiceList * cl, ParsePos * detailPos) {
    auto & cd = cl->details.emplace_back();
    if (st->pos.pos == detailPos->pos) {
        cd.strs.push_back({detailPos->ptr, st->pos.ptr - 1});
    } else {
        cd.strs.push_back({detailPos->ptr, detailPos->eptr});
        auto ipos = detailPos->pos + 1;
        for (; ipos < st->pos.pos; ++ipos)
            cd.strs.push_back(st->strs[ipos]);
        cd.strs.push_back({st->strs[st->pos.pos].data(), st->pos.ptr - 1});
    }
    if (detailPos->ptr == cl->choice.pos.ptr) {
        cd.seg = st->seg;
    } else {
        cd.seg = static_cast<PathSeg *>(copyNode(st->glob, st->seg));
        if (st->parentSeg) {
            st->parentSeg->next.link(cd.seg);
        } else {
            st->glob->roots.link(cd.seg);
        }
    }
    detailPos->ptr = st->pos.ptr;
}

//===========================================================================
static void endChoiceList(
    ParseState * st,
    ChoiceList * cl,
    ParsePos * detailPos
) {
    if (st->pos) {
        cl->tail.push_back({st->pos.ptr, st->pos.eptr});
        for (auto ipos = st->pos.pos + 1; ipos < st->strs.size(); ++ipos)
            cl->tail.push_back(st->strs[ipos]);
    }
}

//===========================================================================
// True if choice was started.
static bool addChoiceList(ParseState * st) {
    auto & cl = st->choices.emplace_back();
    cl.choice = *st;
    auto detailPos = st->pos;
    auto braces = 1;
    while (st->pos) {
        auto ch = nextRawChar(st);
        if (ch == '\\' && st->pos) {
            ch = nextRawChar(st);
            continue;
        }
        switch (ch) {
        default:
            break;
        case ',':
            if (braces == 1)
                addChoice(st, &cl, &detailPos);
            break;
        case '{':
            ++braces;
            break;
        case '}':
            if (--braces)
                break;
            addChoice(st, &cl, &detailPos);
            endChoiceList(st, &cl, &detailPos);
            return true;
        }
    }

    st->strs.resize(cl.choice.pos.pos + 1);
    st->pos = cl.choice.pos;
    st->choices.pop_back();
    return false;
}

//===========================================================================
static bool applyChoice(ParseState * st) {
    if (st->choices.empty())
        return false;
    auto & cl = st->choices.back();
    auto & cd = cl.details.front();
    static_cast<ParseDetail &>(*st) = cl.choice;
    st->strs.resize(st->pos.pos);
    st->strs.insert(st->strs.end(), cd.strs.begin(), cd.strs.end());
    st->strs.insert(st->strs.end(), cl.tail.begin(), cl.tail.end());
    st->pos.ptr = st->strs[st->pos.pos].data();
    st->pos.eptr = st->pos.ptr + st->strs[st->pos.pos].size();
    st->seg = cd.seg;
    cl.details.pop_front();
    if (cl.details.empty())
        st->choices.pop_back();
    return true;
}

//===========================================================================
// Returns true if character is escaped literal.
static bool nextChoiceChar(char * out, ParseState * st) {
    for (;;) {
        *out = nextRawChar(st);
        if (*out == '\\' && st->pos) {
            *out = nextRawChar(st);
            return true;
        }
        if (*out != '{')
            return false;
        if (addChoiceList(st)) {
            applyChoice(st);
        } else {
            // An unmatched '{' is treated as if it was an escaped literal
            // open brace.
            return true;
        }
    }
}

//===========================================================================
bool Dim::Glob::parse(Info * glob, string_view src, GlobType type) {
    assert(type != kInvalid);
    if (!glob->type) {
        glob->type = type;
    } else {
        assert(type == glob->type);
    }

    ParseState st;
    st.glob = glob;
    st.strs.push_back(src);
    st.pos.ptr = src.data();
    st.pos.eptr = st.pos.ptr + src.size();

    st.seg = static_cast<PathSeg *>(addRootSeg(glob));
    char ch = 0;

IN_LITERAL:
    st.mode = ParseMode::kLiteral;
    while (st.pos) {
        if (nextChoiceChar(&ch, &st)) {
            st.lit += ch;
            continue;
        }
        switch (ch) {
        default:
            st.lit += ch;
            break;
        case '[':
            st.set = {};
            st.set.pos = st.pos;
            goto IN_SET;
        case '?':
            {
                flushLiteral(&st);
                bitset<256> bits;
                bits.set();
                addPartCharChoice(glob, st.seg, move(bits));
            }
            break;
        case '*':
            flushLiteral(&st);
            addPartBlot(glob, st.seg);
            break;
        case '/':
            {
                flushLiteral(&st);
                endSeg(glob, st.seg);
                auto seg = addSeg(glob, st.seg);
                st.parentSeg = st.seg;
                st.seg = static_cast<PathSeg *>(seg);
            }
            break;
        }
    }
    goto END;

IN_SET:
    st.mode = ParseMode::kSet;
    ch = 0;
    while (st.pos) {
        st.set.prev = ch;
        if (!nextChoiceChar(&ch, &st)) {
            switch (ch) {
            case '-':
                if (!st.set.prev)
                    break;
                st.set.dash = st.set.prev;
                continue;
            default:
                break;
            case '^':
            case '!':
                if (st.set.prev)
                    break;
                st.set.exclude = true;
                st.set.vals.set();

                // Set ch to 0 so that prev will still be on the "first"
                // character for the next iteration.
                ch = 0;
                continue;
            case ']':
                if (st.set.dash)
                    st.set.vals.set('-', !st.set.exclude);
                flushLiteral(&st);
                addPartCharChoice(st.glob, st.seg, move(st.set.vals));
                goto IN_LITERAL;
            }
        }
        if (st.set.dash) {
            auto last = ch;
            if (last < st.set.dash) {
                auto tmp = last;
                last = st.set.dash - 1;
                st.set.dash = tmp - 1;
            }
            for (; st.set.dash + 1 <= last; ++st.set.dash)
                st.set.vals.set(st.set.dash + 1, !st.set.exclude);
            st.set.dash = 0;
        } else {
            st.set.vals.set(ch, !st.set.exclude);
        }
    }

    // Wasn't a valid set (no matching close braket ']').
    st.pos = st.set.pos;
    st.lit += '[';
    goto IN_LITERAL;

END:
    flushLiteral(&st);
    endSeg(glob, st.seg);

    if (applyChoice(&st)) {
        if (st.mode == ParseMode::kLiteral) {
            goto IN_LITERAL;
        } else {
            assert(st.mode == ParseMode::kSet);
            goto IN_SET;
        }
    }
    return true;
}

//===========================================================================
bool Dim::Glob::parse(
    Info * glob,
    const std::vector<std::string> & srcs,
    GlobType type
) {
    for (auto&& src : srcs) {
        if (!parse(glob, src, type))
            return false;
    }
    return true;
}
