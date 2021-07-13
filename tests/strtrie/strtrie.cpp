// Copyright Glen Knowles 2019 - 2021.
// Distributed under the Boost Software License, Version 1.0.
//
// Uses the page height minimization algorithm for trees described in
// "Clustering Techniques for Minimizing External Path Length"
// https://tinyurl.com/y52mkc22
//
// strtrie.cpp - dim core
#include "pch.h"
#pragma hdrstop

using namespace std;
using namespace Dim;


/****************************************************************************
*
*   Declarations
*
***/

// PLAN A - fixed width 18 byte chunks
// Segment
//  data[0] & 0x7 = kNodeSeg
//  (data[0] >> 3) + 1 = length in nibbles [1, 32]
//  data[1] = index of next node (255 if end of key with no next node)
//  data[2 - 17] = contiguous nibbles of key
// Fork
//  data[0] & 0x7 = kNodeFork
//  data[1] = 255 if end of key, 0 if not
//  data[2 - 17] = index by next nibble to get next node for key (0 for none)

// PLAN B
// Middle Segment
//  data[0] & ? = kNodeMidSeg
//  data[0] & ? = has end of key
//  data[1 & 2] = index of next node --- if not eok
//  data[3] = keyLen (0 - 255)
//  data[4 - (4 + keyLen)] = bytes of key
// Sparse Fork
//  data[0] & ? = kNodeSparseFork
//  data[0] & ? = has end of key
//  data[1] = len (number of cases)
//  data(2, {len}) = case values
//  data(2 + len + 1, {2 * len}) = index of case nodes (16 bit)
// Array Fork
//  data[0] & ? = kNodeArrayFork
//  data[0] & ? = has end of key
//  data[1] = len (number of cases)
//  data[2] = value of first case
//  data(3, {2 * len}) = index of case nodes
// Remote Node
//  data[0] & ? = kNodeRemoteNode
//  data(1, {4}) = pgno
//  data(5, {2}) = index of node

namespace {

const unsigned kNodeLen = 18;
const unsigned kMaxSegLen = 32;
const unsigned kForkChildCount = 16;

const unsigned kRefLen = 4;

using pgno_t = unsigned;

struct PageRef {
    enum {
        kInvalid, // no reference
        kEndMark, // end of key, no linked reference
        kSource,  // source page
        kUpdate,  // shadow node in ss->updates
        kVirtual  // reference to virtual page in ss->vpages
    } type = kInvalid;
    pgno_t pgno = 0;

    auto operator<=>(const PageRef & other) const = default;
};

struct NodeRef {
    PageRef page;
    int pos = -1;

    auto operator<=>(const NodeRef & other) const = default;
};

struct Ref {
    pgno_t pgno;
};
static_assert(sizeof Ref == kRefLen);

enum NodeType : int8_t {
    kNodeInvalid,
    kNodeSeg,
    kNodeFork,
};

struct UpdateNode {
    NodeType type;
    pgno_t vpgno = (pgno_t) -1;
    union {
        struct {
            NodeRef next;   // next.pgno is the vpage number
            unsigned len;
            uint8_t data[kMaxSegLen];
        } seg;
        struct {
            bool endOfKey;
            NodeRef next[kForkChildCount];  // link.pgno is the vpage number
        } fork;
    };

    UpdateNode(NodeType type = kNodeInvalid)
        : type(type)
    {}
};

struct VirtualPage {
    pgno_t targetPgno = (pgno_t) -1;
    size_t targetNodes = 0;
    NodeRef root;
    map<PageRef, UnsignedSet> nodes;
    UnsignedSet spageRefs;
    UnsignedSet vpageRefs;
    UnsignedSet updateRefs;
};

struct SearchState {
    string_view key;
    int kpos = 0;
    size_t klen = 0;
    uint8_t kval = 0;

    // Current page
    pgno_t pgno = 0;
    int nNodes = 0;

    // Current node
    int inode = 0;
    StrTrieBase::Node * node = nullptr;

    // Fork to adjacent key
    int fpos = -1;
    int ifork = -1;

    // Was key found?
    bool found = false;

    map<NodeRef, size_t> updateByRef;
    vector<UpdateNode> updates;
    vector<VirtualPage> vpages;

    IPageHeap * heap = nullptr;

    SearchState(string_view key, IPageHeap * heap);
};

} // namespace


/****************************************************************************
*
*   StrTrieBase::Node
*
***/

struct StrTrieBase::Node {
    uint8_t data[kNodeLen];

    bool operator==(const Node&) const = default;
};


/****************************************************************************
*
*   Helpers
*
***/

//===========================================================================
// Key helpers
//===========================================================================
static uint8_t keyVal(string_view key, size_t pos) {
    assert(key.size() * 2 >= pos);
    if (key.size() * 2 == pos)
        return 0xff;
    return pos % 2 == 0
        ? (key[pos / 2] >> 4)
        : (key[pos / 2] & 0xf);
}

//===========================================================================
// Node helpers
//===========================================================================
constexpr NodeType nodeType(const StrTrieBase::Node * node) {
    return NodeType(*node->data & 0x7);
}

//===========================================================================
static uint8_t segLen(const StrTrieBase::Node * node) {
    assert(nodeType(node) == kNodeSeg);
    return (*node->data >> 3) + 1;
}

//===========================================================================
static const uint8_t & segChild(const StrTrieBase::Node * node) {
    assert(nodeType(node) == kNodeSeg);
    return node->data[1];
}

//===========================================================================
static uint8_t segVal(const StrTrieBase::Node * node, size_t pos) {
    assert(nodeType(node) == kNodeSeg);
    return pos % 2 == 0
        ? (node->data[2 + pos / 2] >> 4)
        : (node->data[2 + pos / 2] & 0xf);
}

//===========================================================================
constexpr uint8_t asSeg0(size_t len) {
    return (uint8_t) (kNodeSeg | ((len - 1) << 3));
}

//===========================================================================
inline static void setSegLen(StrTrieBase::Node * node, size_t len) {
    node->data[0] = asSeg0(len);
}

//===========================================================================
inline static uint8_t pushSegVal(
    StrTrieBase::Node * node,
    size_t pos,
    uint8_t val
) {
    assert(nodeType(node) == kNodeSeg);
    assert(pos < kMaxSegLen);
    if (pos % 2 == 0) {
        node->data[2 + pos / 2] = val << 4;
    } else {
        node->data[2 + pos / 2] |= val;
    }
    return val;
}

//===========================================================================
static uint8_t forkChild(
    const StrTrieBase::Node * node,
    size_t pos
) {
    assert(nodeType(node) == kNodeFork);
    assert(pos < kForkChildCount);
    return node->data[pos + 2];
}

//===========================================================================
static size_t setForkChild(
    StrTrieBase::Node * node,
    uint8_t child,
    size_t inode
) {
    assert(nodeType(node) == kNodeFork);
    assert(child == 255 || child < kForkChildCount);
    assert(inode < 256);

    auto inext = (uint8_t) inode;
    if (child == 255) {
        node->data[1] = inext;
    } else {
        node->data[child + 2] = inext;
    }
    return inext;
}

//===========================================================================
static span<const uint8_t> kids(const StrTrie::Node & node) {
    switch (nodeType(&node)) {
    case kNodeSeg:
        return { &segChild(&node), 1 };
    case kNodeFork:
        return { node.data + 2, kForkChildCount };
    default:
        assert(!"Invalid node type");
        return {};
    }
}

//===========================================================================
// UpdateNode helpers
//===========================================================================
static span<NodeRef> kids(UpdateNode & upd) {
    if (upd.type == kNodeSeg) {
        return { &upd.seg.next, 1 };
    } else {
        assert(upd.type == kNodeFork);
        return { upd.fork.next, size(upd.fork.next) };
    }
}

//===========================================================================
// SearchState helpers
//===========================================================================
static bool checkCount(SearchState * ss, size_t nodes, size_t refs) {
    return nodes + refs < 255
        && nodes * kNodeLen + refs * sizeof Ref + 2 <= ss->heap->pageSize();
}

//===========================================================================
static void setCount(SearchState * ss, size_t nodes, size_t refs) {
    assert(checkCount(ss, nodes, refs));
    auto ptr = ss->heap->ptr(ss->pgno);
    ptr[0] = (uint8_t) nodes;
    ptr[1] = (uint8_t) refs;
}

//===========================================================================
static size_t numNodes(const SearchState & ss) {
    auto ptr = ss.heap->ptr(ss.pgno);
    return ptr[0];
}

//===========================================================================
inline static size_t numRefs(const SearchState & ss) {
    auto ptr = ss.heap->ptr(ss.pgno);
    return ptr[1];
}

//===========================================================================
static void allocPage(SearchState * ss) {
    ss->pgno = (pgno_t) ss->heap->create();
    setCount(ss, 0, 0);
    ss->nNodes = 0;
}

//===========================================================================
static void seekPage(SearchState * ss, pgno_t pgno) {
    ss->pgno = pgno;
    ss->nNodes = (int) numNodes(*ss);
}

//===========================================================================
static void seekRootPage(SearchState * ss) {
    seekPage(ss, (pgno_t) ss->heap->root());
}

//===========================================================================
static StrTrieBase::Node * getNode(SearchState * ss, size_t inode) {
    assert(inode < 255);
    auto ptr = ss->heap->ptr(ss->pgno);
    assert(ptr);
    ptr += 2;
    return (StrTrieBase::Node *) ptr + inode;
}

//===========================================================================
static NodeRef remoteRef(SearchState * ss, size_t inode) {
    assert(inode < 255);
    auto iref = 255 - inode;
    assert(iref <= numRefs(*ss));
    auto ptr = ss->heap->ptr(ss->pgno);
    auto rawRef = ptr + ss->heap->pageSize()
        - iref * kRefLen;
    assert(rawRef > ptr);
    auto ref = (Ref *) rawRef;

    NodeRef out = {
        {
            .type = PageRef::kSource,
            .pgno = ref->pgno
        },
        0 // inode
    };
    return out;
}

//===========================================================================
static uint8_t inode(
    const VirtualPage & vpage,
    const NodeRef & ref
) {
    if (ref.page.type == PageRef::kInvalid) {
        return 0;
    } else if (ref.page.type == PageRef::kEndMark) {
        return 255;
    } else {
        assert(ref.page.type == PageRef::kUpdate);
        assert(vpage.vpageRefs.empty());
        if (ref.page.pgno == vpage.targetPgno) {
            return (uint8_t) ref.pos;
        } else {
            auto pos = vpage.spageRefs.count(0, ref.page.pgno);
            return (uint8_t) (255 - pos);
        }
    }
}

//===========================================================================
static void setRemoteRef(SearchState * ss, size_t iref, pgno_t pgno) {
    assert(iref < 255);
    iref = 255 - iref;
    auto ptr = ss->heap->ptr(ss->pgno);
    auto rawRef = ptr + ss->heap->pageSize()
        - iref * kRefLen;
    assert(rawRef > ptr);
    auto ref = (Ref *) rawRef;
    ref->pgno = pgno;
}

//===========================================================================
static void seekNode(SearchState * ss, size_t inode) {
    ss->inode = (int) inode;
    ss->node = getNode(ss, ss->inode);
}


/****************************************************************************
*
*   StrTrieBase::Iterator
*
***/

//===========================================================================
StrTrieBase::Iterator & StrTrieBase::Iterator::operator++() {
    return *this;
}


/****************************************************************************
*
*   SearchState
*
***/

//===========================================================================
SearchState::SearchState(string_view key, IPageHeap * heap)
    : key(key)
    , kpos(0)
    , klen(key.size() * 2)
    , kval(keyVal(key, kpos))
    , heap(heap)
{}


/****************************************************************************
*
*   Insert
*
***/

//===========================================================================
static UpdateNode & newUpdate(
    SearchState * ss,
    int idest
) {
    auto ref = NodeRef{{PageRef::kUpdate, ss->pgno}, idest};
    auto id = ss->updates.size();
    auto & upd = ss->updates.emplace_back();
    assert(!ss->updateByRef.contains(ref));
    ss->updateByRef[ref] = id;
    return upd;
}

//===========================================================================
static UpdateNode & newSeg(
    SearchState * ss,
    int idest
) {
    auto & upd = newUpdate(ss, idest);
    upd.type = kNodeSeg;
    upd.seg = {};
    return upd;
}

//===========================================================================
static UpdateNode & newFork(
    SearchState * ss,
    int idest,
    bool zero = true
) {
    auto & upd = newUpdate(ss, idest);
    upd.type = kNodeFork;
    if (zero)
        upd.fork = {};
    return upd;
}

//===========================================================================
static void mapFringeNode(
    SearchState * ss,
    VirtualPage * vpage,
    UnsignedSet * nodes,
    int inode
) {
    if (!inode || inode == 255) {
        // no node
        return;
    } else if (inode >= numNodes(*ss)) {
        // remote node
        auto ref = remoteRef(ss, inode);
        assert(ref.pos == 0);
        assert(ref.page.type == PageRef::kSource);
        vpage->spageRefs.insert(ref.page.pgno);
        return;
    }

    // local node
    nodes->insert(inode);

    auto node = getNode(ss, inode);
    switch (::nodeType(node)) {
    case kNodeFork:
        for (auto i = 0; i < kForkChildCount; ++i) {
            auto nval = node->data[i + 2];
            mapFringeNode(ss, vpage, nodes, nval);
        }
        break;
    case kNodeSeg:
        mapFringeNode(ss, vpage, nodes, segChild(node));
        break;
    default:
        assert(!"Invalid node type");
        break;
    }
}

//===========================================================================
static void mapFringeNode(SearchState * ss, int inode) {
    auto & vpage = ss->vpages.emplace_back();
    vpage.root = {
        .page = { .type = PageRef::kSource, .pgno = ss->pgno },
        .pos = inode
    };
    auto & nodes = vpage.nodes[vpage.root.page];
    mapFringeNode(ss, &vpage, &nodes, inode);
}

//===========================================================================
static NodeRef nodeRef(SearchState * ss, uint8_t nval) {
    NodeRef out;
    if (!nval) {
        // use default constructed ref
    } else if (nval < numNodes(*ss)) {
        out.pos = nval;
        out.page.type = PageRef::kSource;
        out.page.pgno = ss->pgno;
    } else if (nval < 255) {
        out = remoteRef(ss, nval);
    } else {
        assert(nval == 255);
        out.page.type = PageRef::kEndMark;
    }
    return out;
}

//===========================================================================
static UpdateNode & copySeg(
    SearchState * ss,
    int idest,
    size_t pos,
    size_t len
) {
    assert(nodeType(ss->node) == kNodeSeg);
    auto & upd = newSeg(ss, idest);
    upd.seg.len = (unsigned) len;
    auto nval = ss->node->data[1];
    upd.seg.next = nodeRef(ss, nval);
    assert(upd.seg.next.page.type != PageRef::kInvalid);
    for (unsigned i = 0; i < len; ++i) {
        auto sval = segVal(ss->node, i + pos);
        upd.seg.data[i] = sval;
    }
    return upd;
}

//===========================================================================
// Returns false to abort (exact match already exists). Sets ss->inode ==
// ss->nNodes if the search is over, otherwise ss->inode < ss->nNodes.
static bool insertAtSeg(SearchState * ss) {
    // Will either split on first, middle, last, after last, or will advance
    // to next node.

    PageRef page = { PageRef::kUpdate, ss->pgno };

    auto spos = (uint8_t) 0; // split point where key diverges from segment
    auto slen = segLen(ss->node);
    auto sval = segVal(ss->node, spos);
    assert(slen > 0);
    auto leadPos = ss->updates.size();
    copySeg(ss, ss->inode, 0, slen);

    for (;;) {
        if (ss->kval != sval)
            break;
        ss->kval = keyVal(ss->key, ++ss->kpos);
        if (++spos == slen) {
			// End of segment, still more key?
            ss->inode = ss->node->data[1];
            if (ss->inode == 255) {
                // End of both segment and key? Key was found, abort.
                if (ss->kval == 255)
                    return false;

                // Fork with the end mark that's after the segment.
                sval = 255;
                break;
            }
            // Continue search at next node
            auto & lead = ss->updates[leadPos];
            lead.seg.next.page = page;
            return true;
        }
        sval = segVal(ss->node, spos);
    }

    // Replace segment with [lead segment] fork [tail segment]
    assert(spos <= slen);
    auto itail = (unsigned) 0;
    if (spos + 1 >= slen) {
        itail = ss->node->data[1];
    } else {
        // Add tail segment
        itail = ss->nNodes++;
        copySeg(ss, itail, spos + 1, slen - spos - 1);
    }
    auto & lead = ss->updates[leadPos];
    UpdateNode * upd = nullptr;
    if (!spos) {
        // Only single element after detaching tail, so no lead segment needed,
        // convert to fork.
        assert(itail != 255);
        ss->inode = ss->nNodes;
        lead.type = kNodeFork;
        lead.fork = {};
        upd = &lead;
    } else {
        // Copy truncated node as the lead segment.
        // Point at fork that's about to be added.
        lead.seg.len = spos;
        lead.seg.next.pos = ss->nNodes;
        lead.seg.next.page = page;

        // Add fork.
        auto & mid = newFork(ss, ss->nNodes);
        ss->inode = ++ss->nNodes;
        upd = &mid;
    }
    if (itail == 255) {
        if (sval == 255) {
            upd->fork.endOfKey = true;
        } else {
            upd->fork.next[sval].page.type = PageRef::kEndMark;
        }
    } else {
        assert(sval != 255);
        upd->fork.next[sval].pos = itail;
        upd->fork.next[sval].page = page;
        if (spos + 1 >= slen) {
            // Continue to unmodified node, no new tail inserted.
            upd->fork.next[sval].page.type = PageRef::kSource;
        }
    }
    if (ss->kpos + 1 >= ss->klen) {
        if (ss->kpos == ss->klen) {
            upd->fork.endOfKey = true;
            return true;
        } else {
            upd->fork.next[ss->kval].page.type = PageRef::kEndMark;
        }
    } else {
        upd->fork.next[ss->kval].pos = ss->nNodes;
        upd->fork.next[ss->kval].page = page;
    }

    // Continue processing the key.
    ss->kval = keyVal(ss->key, ++ss->kpos);
    return true;
}

//===========================================================================
static UpdateNode & copyFork(SearchState * ss, int idest) {
    assert(nodeType(ss->node) == kNodeFork);
    auto & upd = newFork(ss, idest, false);
    upd.fork.endOfKey = ss->node->data[1];
    for (auto i = 0; i < size(upd.fork.next); ++i) {
        auto nval = ss->node->data[i + 2];
        upd.fork.next[i] = nodeRef(ss, nval);
    }
    return upd;
}

//===========================================================================
// Returns true if there is more to do
static bool insertAtFork (SearchState * ss) {
    // Will either add branch to fork, or will advance to next node.

    PageRef page = { PageRef::kUpdate, ss->pgno };
    auto & upd = copyFork(ss, ss->inode);

    if (ss->kpos == ss->klen) {
        assert(ss->kval == 255);
        if (!upd.fork.endOfKey) {
            upd.fork.endOfKey = true;
            ss->inode = ss->nNodes;
            return true;
        } else {
            ss->found = true;
            return false;
        }
    }

    auto & next = upd.fork.next[ss->kval];
    if (next.page.type == PageRef::kInvalid) {
        if (ss->kpos + 1 == ss->klen) {
            next.page.type = PageRef::kEndMark;
        } else {
            next.pos = ss->nNodes;
            next.page = page;
        }
        ss->kval = keyVal(ss->key, ++ss->kpos);
        ss->inode = ss->nNodes;
    } else if (next.page.type == PageRef::kEndMark) {
        next.pos = ss->nNodes;
        next.page = page;
        ss->kval = keyVal(ss->key, ++ss->kpos);
        if (ss->kval == 255) {
            ss->found = true;
            return false;
        }

        ss->inode = ++ss->nNodes;
        auto & sub = newFork(ss, ss->inode);
        sub.fork.endOfKey = true;
    } else {
        next.page = page;
        ss->kval = keyVal(ss->key, ++ss->kpos);
        ss->inode = next.pos;
    }

    return true;
}

//===========================================================================
static void addSegs(SearchState * ss) {
    while (ss->kpos < ss->klen) {
        auto & upd = newSeg(ss, ss->nNodes);
        ss->inode = ss->nNodes++;
        if (ss->klen - ss->kpos > kMaxSegLen) {
            upd.seg.len = kMaxSegLen;
            upd.seg.next.page.type = PageRef::kUpdate;
            upd.seg.next.page.pgno = ss->pgno;
            upd.seg.next.pos = ss->nNodes;
        } else {
            upd.seg.len = (unsigned) (ss->klen - ss->kpos);
            upd.seg.next.page.type = PageRef::kEndMark;
        }
        for (auto spos = 0u; spos < upd.seg.len; ++spos, ++ss->kpos)
            upd.seg.data[spos] = keyVal(ss->key, ss->kpos);
    }
}

//===========================================================================
static UpdateNode & copyAny(SearchState * ss, int idest) {
    if (auto type = nodeType(ss->node); type == kNodeFork) {
        return copyFork(ss, idest);
    } else {
        assert(type == kNodeSeg);
        return copySeg(ss, idest, 0, segLen(ss->node));
    }
}

//===========================================================================
static void copySeg(
    SearchState * ss,
    const VirtualPage & vpage,
    const UpdateNode & upd
) {
    setSegLen(ss->node, upd.seg.len);
    ss->node->data[1] = inode(vpage, upd.seg.next);
    unsigned i = 0;
    for (; i < upd.seg.len; ++i)
        pushSegVal(ss->node, i, upd.seg.data[i]);
    for (; i < kMaxSegLen; ++i)
        pushSegVal(ss->node, i, 0);
}

//===========================================================================
static void copyFork(
    SearchState * ss,
    const VirtualPage & vpage,
    const UpdateNode & upd
) {
    ss->node->data[0] = kNodeFork;
    ss->node->data[1] = upd.fork.endOfKey ? 255 : 0;
    for (auto i = 0; i < size(upd.fork.next); ++i) {
        auto & ref = upd.fork.next[i];
        setForkChild(ss->node, (uint8_t) i, inode(vpage, ref));
    }
}

//===========================================================================
static void copyAny(
    SearchState * ss,
    const VirtualPage & vpage,
    const UpdateNode & upd
) {
    if (upd.type == kNodeSeg) {
        copySeg(ss, vpage, upd);
    } else {
        assert(upd.type == kNodeFork);
        copyFork(ss, vpage, upd);
    }
}

//===========================================================================
static int applyNode(
    SearchState * ss,
    UnsignedSet * avail,
    VirtualPage * vpage,
    const NodeRef & ref,
    int ikid
) {
    auto newPos = ref.pos;
    auto spgno = vpage->targetPgno;
    NodeRef kid;
    if (ref.page.type == PageRef::kVirtual) {
        auto & vp = ss->vpages[ref.page.pgno];
        kid.page = { .type = PageRef::kUpdate, .pgno = vp.targetPgno };
        kid.pos = 0;
        return inode(vp, kid);
    }
    if (ref.page.type == PageRef::kSource) {
        seekPage(ss, ref.page.pgno);
        seekNode(ss, ref.pos);
        if (ref.page.pgno != spgno || ref.pos >= vpage->targetNodes) {
            auto inode = (int) avail->pop_front();
            [[maybe_unused]] auto & upd = copyAny(ss, inode);
            NodeRef dref = {
                .page = { .type = PageRef::kUpdate, .pgno = spgno },
                .pos = inode
            };
            return applyNode(ss, avail, vpage, dref, ikid);
        }
        auto subs = kids(*ss->node);
        for (auto i = ikid; i < subs.size(); ++i) {
            auto inode = subs[i];
            if (!inode || inode == 255)
                continue;
            if (inode >= ss->nNodes) {
                [[maybe_unused]] auto remote = remoteRef(ss, inode);
                assert(vpage->spageRefs.contains(remote.page.pgno));
                [[maybe_unused]] auto & upd = copyAny(ss, ss->inode);
                auto dref = ref;
                dref.page.type = PageRef::kUpdate;
                return applyNode(ss, avail, vpage, dref, i);
            }

            kid.page = ref.page;
            kid.pos = inode;
            auto pos = applyNode(ss, avail, vpage, kid, 0);
            if (kid.pos != pos) {
                seekNode(ss, inode);
                auto & upd = copyAny(ss, ss->inode);
                auto & sub = kids(upd)[i];
                sub.page.type = PageRef::kUpdate;
                sub.pos = pos;
                auto dref = ref;
                dref.page.type = PageRef::kUpdate;
                return applyNode(ss, avail, vpage, dref, i);
            }
        }
        return ref.pos;
    }

    assert(ref.page.type == PageRef::kUpdate);
    auto id = ss->updateByRef[ref];
    vpage->updateRefs.insert((unsigned) id);
    auto & upd = ss->updates[id];
    auto subs = kids(upd);
    for (auto i = ikid; i < subs.size(); ++i) {
        auto & kid = subs[i];
        if (kid.page.type == PageRef::kInvalid
            || kid.page.type == PageRef::kEndMark
        ) {
            assert(kid.pos == -1);
            continue;
        }
        auto pos = applyNode(ss, avail, vpage, kid, 0);
        if (kid.pos == pos && kid.page == ref.page)
            continue;
        kid = ref;
        kid.pos = pos;
    }

    return newPos;
}

//===========================================================================
static void applyUpdates(SearchState * ss) {
    // From updated nodes, generate:
    //  - child to parent graph
    //  - vector of leaf nodes
    //  - set of involved source pages
    struct UpdateInfo {
        NodeRef ref;
        size_t parent = (size_t) -1;

        // for leaf: 0
        // for branch: 1 + number of unprocessed kids
        size_t unprocessed = 0;
    };
    vector<UpdateInfo> infos(ss->updates.size());
    vector<size_t> unprocessed;
    UnsignedSet spages;
    for (auto&& kv : ss->updateByRef) {
        spages.insert(kv.first.page.pgno);
        auto & ui = infos[kv.second];
        ui.ref = kv.first;
        bool branch = false;
        auto pos = -1;
        for (auto&& kid : kids(ss->updates[kv.second])) {
            pos += 1;
            if (kid.page.type == PageRef::kInvalid
                || kid.page.type == PageRef::kEndMark
            ) {
                // skip empty child
                assert(kid.pos == -1);
            } else if (kid.page.type == PageRef::kUpdate) {
                ui.unprocessed += 1;
                assert(ss->updateByRef.contains(kid));
                auto id = ss->updateByRef[kid];
                infos[id].parent = kv.second;
                branch = true;
            } else {
                assert(kid.page.type == PageRef::kSource);
                branch = true;
            }
        }
        if (branch)
            ui.unprocessed += 1;
        if (ui.unprocessed < 2)
            unprocessed.push_back(kv.second);
    }

    // Process updated nodes and related fringe nodes into virtual pages. Start
    // with leaf nodes, with branches becoming eligible for processing when all
    // their leafs are done. Continues all the way up until the root node is
    // processed. Child virtual pages are merged up into their parents when
    // possible.
    while (!unprocessed.empty()) {
        auto id = unprocessed.back();
        unprocessed.pop_back();
        auto & ui = infos[id];
        if (!ui.unprocessed) {
            // leaf
            auto & vpage = ss->vpages.emplace_back();
            ss->updates[id].vpgno = (pgno_t) (ss->vpages.size() - 1);
            vpage.root = ui.ref;
            vpage.nodes[vpage.root.page].insert(ui.ref.pos);
        } else {
            // internal branch
            VirtualPage vmerge;
            vmerge.root = ui.ref;
            vmerge.nodes[vmerge.root.page].insert(ui.ref.pos);
            UnsignedSet nvRefs;
            for (auto&& kid : kids(ss->updates[id])) {
                if (kid.page.type == PageRef::kInvalid
                    || kid.page.type == PageRef::kEndMark
                ) {
                    assert(kid.pos == -1);
                    continue;
                }
                pgno_t vpgno;
                if (kid.page.type == PageRef::kSource) {
                    // map fringe page
                    seekPage(ss, kid.page.pgno);
                    mapFringeNode(ss, kid.pos);
                    vpgno = (pgno_t) (ss->vpages.size() - 1);
                } else {
                    assert(kid.page.type == PageRef::kUpdate);
                    auto uid = ss->updateByRef[kid];
                    auto & upd = ss->updates[uid];
                    vpgno = upd.vpgno;
                }
                // Speculatively update kids assuming they won't be combined
                // with branch. If we don't update them we would still have to
                // keep a copy of the vpages to reference somewhere, in case
                // they aren't all combined.
                kid.page.type = PageRef::kVirtual;
                kid.page.pgno = vpgno;
                kid.pos = 0;

                nvRefs.insert(vpgno);
                auto & vpage = ss->vpages[vpgno];
                vmerge.spageRefs.insert(vpage.spageRefs);
                vmerge.vpageRefs.insert(vpage.vpageRefs);
                for (auto&& [ref, nodes] : vpage.nodes)
                    vmerge.nodes[ref].insert(nodes);
            }
            auto nRefs = vmerge.spageRefs.size() + vmerge.vpageRefs.size();
            auto nNodes = 0;
            for (auto&& kv : vmerge.nodes)
                nNodes += (int) kv.second.size();
            if (!checkCount(ss, nNodes, nRefs)) {
                // new vpage, separate from kids
                auto & vpage = ss->vpages.emplace_back();
                vpage.root = ui.ref;
                vpage.nodes[vpage.root.page].insert(ui.ref.pos);
                vpage.vpageRefs.insert(nvRefs);
            } else {
                // new vpage, combined with kids
                ss->vpages.emplace_back(move(vmerge));
                for (auto&& kid : kids(ss->updates[id])) {
                    if (kid.page.type == PageRef::kVirtual) {
                        // Update child references to point at local page nodes
                        // instead of other virtual pages.
                        auto & vpage = ss->vpages[kid.page.pgno];
                        kid = vpage.root;
                        vpage.nodes.clear();
                        vpage.spageRefs.clear();
                        vpage.vpageRefs.clear();
                    }
                }
            }
            ss->updates[id].vpgno = (pgno_t) (ss->vpages.size() - 1);
        }

        // Update reference from parent
        if (ui.parent == -1) {
            // Adding root node as a leaf, must be adding first key to this
            // previously empty container.
            assert(unprocessed.empty());
        } else {
            auto & pi = infos[ui.parent];
            assert(pi.unprocessed > 1);
            if (--pi.unprocessed == 1) {
                // Parent has had all children processed, queue for
                // processing.
                unprocessed.push_back(ui.parent);
            }
        }
    }

    // Find best virtual to source page replacements
    UnsignedSet vpages;
    struct NodeCount {
        pgno_t vid;
        pgno_t sid;
        size_t count;
    };
    vector<NodeCount> counts;
    for (auto i = 0; i < ss->vpages.size(); ++i) {
        auto & vpage = ss->vpages[i];
        if (vpage.nodes.empty())
            continue;
        vpages.insert(i);
        unordered_map<pgno_t, NodeCount> vcnts;
        for (auto&& [ref, nodes] : vpage.nodes) {
            auto & cnt = vcnts[ref.pgno];
            cnt.vid = i;
            cnt.sid = ref.pgno;
            cnt.count = nodes.size();
        }
        for (auto&& cnt : vcnts)
            counts.emplace_back(cnt.second);
    }
    sort(counts.begin(), counts.end(), [](auto & a, auto & b) {
        return a.count > b.count;
    });
    for (auto&& cnt : counts) {
        if (vpages.contains(cnt.vid) && spages.contains(cnt.sid)) {
            ss->vpages[cnt.vid].targetPgno = cnt.sid;
            spages.erase(cnt.sid);
            vpages.erase(cnt.vid);
        }
    }
    // Left over source pages after assigning all virtual pages? erase 'em.
    for (auto&& id : spages)
        ss->heap->destroy(id);
    // Still more virtual pages after all sources pages have been reused?
    // Assign 'em new pages.
    for (auto&& id : vpages) {
        allocPage(ss);
        ss->vpages[id].targetPgno = ss->pgno;
    }

    // Map node changes to source pages
    for (pgno_t vpgno = 0; vpgno < ss->vpages.size(); ++vpgno) {
        auto & vpage = ss->vpages[vpgno];
        if (vpage.targetPgno == -1)
            continue;
        // Translate vpageRefs to spageRefs using the just calculated virtual
        // to source mapping.
        for (auto&& vid : vpage.vpageRefs) {
            auto sid = ss->vpages[vid].targetPgno;
            assert(sid != -1);
            vpage.spageRefs.insert(sid);
        }
        vpage.vpageRefs.clear();
        // Count total nodes, and track nodes already in source page.
        UnsignedSet used;
        vpage.targetNodes = 0;
        for (auto&& kv : vpage.nodes) {
            vpage.targetNodes += (int) kv.second.size();
            if (kv.first.pgno == vpage.root.page.pgno)
                used.insert(kv.second);
        }
        seekPage(ss, vpage.targetPgno);

        // All nodes less than the total that haven't already been used
        // are available.
        UnsignedSet avail(0, vpage.targetNodes);
        avail.erase(used);

        applyNode(
            ss,
            &avail,
            &vpage,
            vpage.root,
            0
        );
    }

    // Log updates
    infos.resize(ss->updates.size());
    for (auto&& [ref, id] : ss->updateByRef) {
        auto & info = infos[id];
        info.ref = ref;
    }
    for (pgno_t vpgno = 0; vpgno < ss->vpages.size(); ++vpgno) {
        auto & vpage = ss->vpages[vpgno];
        if (vpage.nodes.empty())
            continue;
        seekPage(ss, vpage.targetPgno);
        auto nRefs = vpage.spageRefs.size();
        auto nNodes = vpage.targetNodes;
        setCount(ss, nNodes, nRefs);
        auto pos = 1;
        for (auto&& i : vpage.spageRefs) {
            setRemoteRef(ss, pos, i);
            pos += 1;
        }
        for (auto&& id : vpage.updateRefs) {
            auto & upd = ss->updates[id];
            auto & ref = infos[id].ref;
            seekPage(ss, ref.page.pgno);
            seekNode(ss, ref.pos);
            copyAny(ss, vpage, upd);
        }
    }
}

//===========================================================================
// Returns true if key was inserted
bool StrTrieBase::insert(string_view key) {
    auto ss = SearchState(key, &m_heap);
    if (empty()) {
        allocPage(&ss);
    } else {
        seekRootPage(&ss);
        while (ss.inode < ss.nNodes) {
            seekNode(&ss, ss.inode);
            switch (auto ntype = ::nodeType(ss.node)) {
            case kNodeSeg:
                if (!insertAtSeg(&ss))
                    return false;
                break;
            case kNodeFork:
                if (!insertAtFork(&ss))
                    return false;
                break;
            default:
                logMsgFatal() << "Invalid StrTrieBase node type: " << ntype;
            }
        }
    }

    // Add any trailing key segments.
    addSegs(&ss);

    // Apply pending updates
    if (!ss.updates.empty())
        applyUpdates(&ss);

    // was an insert; return true
    return true;
}


/****************************************************************************
*
*   Erase
*
***/

//===========================================================================
// Returns true if there is more to do
static bool findLastForkAtFork(SearchState * ss) {
    ss->inode = forkChild(ss->node, ss->kval);
    if (!ss->inode)
        return false;
    if (ss->inode == 255) {
        if (ss->kpos == ss->klen)
            ss->found = true;
        return false;
    }
    if (ss->kpos == ss->klen)
        return false;

    ss->ifork = ss->inode;
    ss->fpos = ss->kpos;
    ss->kval = keyVal(ss->key, ++ss->kpos);
    return true;
}

//===========================================================================
// Returns true if there is more to do
static bool findLastForkAtSeg(SearchState * ss) {
    auto slen = segLen(ss->node);
    if (ss->klen - ss->kpos < slen)
        return false;
    for (auto spos = 0; spos < slen; ++spos) {
        auto sval = segVal(ss->node, spos);
        if (sval != ss->kval)
            return false;
        ss->kval = keyVal(ss->key, ++ss->kpos);
    }
    ss->inode = ss->node->data[1];
    if (ss->kpos != ss->klen)
        return ss->inode != 255;
    ss->found = true;
    return false;
}

//===========================================================================
static void findLastFork(SearchState * ss) {
    ss->ifork = ss->inode;
    ss->fpos = 0;

    for (;;) {
        seekNode(ss, ss->inode);
        switch (auto ntype = ::nodeType(ss->node)) {
        case kNodeSeg:
            if (!findLastForkAtSeg(ss))
                return;
            break;
        case kNodeFork:
            if (!findLastForkAtFork(ss))
                return;
            break;
        default:
            logMsgFatal() << "Invalid StrTrieBase node type: " << ntype;
        }
    }
}

constexpr static StrTrieBase::Node s_emptyFork = { kNodeFork };

//===========================================================================
bool StrTrieBase::erase(string_view key) {
    if (empty())
        return false;

    auto ss = SearchState(key, &m_heap);
    seekRootPage(&ss);

    findLastFork(&ss);
    if (!ss.found)
        return false;

    seekNode(&ss, ss.ifork);
    ss.kpos = ss.fpos;
    ss.inode = forkChild(ss.node, ss.kval);
    setForkChild(ss.node, ss.kval, 0);
    if (*ss.node == s_emptyFork) {
    }
    if (memcmp(ss.node, &s_emptyFork, sizeof *ss.node)) {
        //
    }

    return true;
}


/****************************************************************************
*
*   Find
*
***/

//===========================================================================
bool StrTrieBase::contains(string_view key) const {
    if (empty())
        return false;

    auto ss = SearchState(key, &m_heap);
    seekRootPage(&ss);

    for (;;) {
        seekNode(&ss, ss.inode);
        auto ntype = ::nodeType(ss.node);
        if (ntype == kNodeFork) {
            if (ss.kpos == ss.klen) {
                ss.inode = ss.node->data[1];
                return ss.inode == 255;
            }
            ss.inode = forkChild(ss.node, ss.kval);
            if (ss.inode == 255)
                return ss.kpos + 1 == ss.klen;
            ss.kval = keyVal(ss.key, ++ss.kpos);
            continue;
        }

        if (ntype == kNodeSeg) {
            auto slen = segLen(ss.node);
            if (ss.klen - ss.kpos < slen)
                return false;
            for (auto spos = 0; spos < slen; ++spos) {
                auto sval = segVal(ss.node, spos);
                if (sval != ss.kval)
                    return false;
                ss.kval = keyVal(ss.key, ++ss.kpos);
            }
            ss.inode = ss.node->data[1];
            if (ss.inode == 255)
                return ss.kpos == ss.klen;
        }
    }
}

//===========================================================================
bool StrTrieBase::lowerBound(string * out, string_view key) const {
    out->clear();
    if (empty())
        return false;

    auto ss = SearchState(key, &m_heap);
    seekRootPage(&ss);
    auto noEqual = false;

    while (ss.inode != 255) {
        seekNode(&ss, ss.inode);
        auto ntype = ::nodeType(ss.node);
        if (ntype == kNodeFork) {
            if (ss.kpos == ss.klen) {
                ss.inode = ss.node->data[1];
                break;
            }
            ss.inode = forkChild(ss.node, ss.kval);
            ss.kval = keyVal(key, ++ss.kpos);
            continue;
        }

        if (ntype == kNodeSeg) {
            auto slen = segLen(ss.node);
            if (ss.klen - ss.kpos < slen) {
                for (auto spos = 0; ss.kpos < ss.klen; ++spos) {
                    auto sval = segVal(ss.node, spos);
                    if (sval == ss.kval) {
                        ss.kval = keyVal(ss.key, ++ss.kpos);
                        continue;
                    }
                    if (sval < ss.kval) {
                        // TODO: go back to last lower branch
                        assert(0);
                    }
                    noEqual = true;
                    //goto follow to end of this greater key
                }
                return false;
            } else {
                for (auto spos = 0; spos < slen; ++spos) {
                    auto sval = segVal(ss.node, spos);
                    if (sval != ss.kval)
                        return false;
                    ss.kval = keyVal(key, ++ss.kpos);
                }
            }
            ss.inode = ss.node->data[1];
            if (ss.kpos == ss.klen)
                break;
        }
    }

    *out = key;
    return ss.inode == 255 && ss.kpos == ss.klen;
}


/****************************************************************************
*
*   Misc
*
***/

//===========================================================================
StrTrieBase::Iterator StrTrieBase::begin() const {
    return Iterator{};
}

//===========================================================================
StrTrieBase::Iterator StrTrieBase::end() const {
    return Iterator{};
}

//===========================================================================
ostream & StrTrieBase::dump(ostream & os) const {
    if (empty())
        return os;

    auto ss = SearchState({}, &m_heap);
    seekRootPage(&ss);
    ss.inode = 0;
    for (; ss.inode < ss.nNodes; ++ss.inode) {
        seekNode(&ss, ss.inode);
        os << setw(3) << ss.inode << ": ";
        for (auto i = 0; i < kNodeLen; ++i) {
            if (i % 4 == 2) os.put(' ');
            if (auto val = ss.node->data[i]) {
                hexByte(os, ss.node->data[i]);
            } else {
                os << "--";
            }
        }

        switch (auto ntype = ::nodeType(ss.node)) {
        case kNodeInvalid:
            os << "\n";
            break;
        case kNodeFork:
            os << "  Fork\n";
            break;
        case kNodeSeg:
            os << "  Segment[" << (int) segLen(ss.node) << "], next="
                << (int) ss.node->data[1] << "\n";
            break;
        default:
            os << "  UNKNOWN(" << ntype << ")\n";
            break;
        }
    }
    return os;
}
