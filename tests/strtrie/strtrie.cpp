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

using USet = IntegralSet<unsigned,
    std::pmr::polymorphic_allocator<unsigned>>;


/****************************************************************************
*
*   Declarations
*
***/

// PLAN C - implicitly referenced byte aligned nodes
// Segment
//  data[0] & 0x7 = kNodeSeg
//  data[0] & 0x8 = has end of key
//  data[0] & 0xf0 = keyLen (1 - 16) in full bytes (no odd nibble)
//  data[1, keyLen] = bytes of key
//  : followed by next node - only if not eok
// Half Segment
//  data[0] & 0x7 = kNodeHalfSeg
//  data[0] & 0x8 = has end of key
//  data[0] & 0xf0 = key (1 nibble in length)
//  : followed by next node - only if not eok
// Fork
//  data[0] & 0x7 = kNodeFork
//  data[0] & 0x8 = has end of key
//  data[1, 2] = bitmap of existing children, all 16 bits means all 16 kids
//  : followed by nodes[hammingWeight(bitmap)]
// Remote
//  data[0] & 0x7 = kNodeRemote
//  data[0] & 0x8 = false (has end of key)
//  data(1, 4) = pgno
//  data(5, 2) = index of node
// EndMark
//  data[0] & 0x7 = kNodeEndMark
//  data[0] & 0x8 = true (end of key - no content)

const size_t kMaxForkBit = 16;
const size_t kMaxSegLen = 32;

const size_t kRemoteNodeLen = 7;


namespace {

using pgno_t = unsigned;

struct PageRef {
    enum {
        kInvalid, // no reference
        kSource,  // source page
        kUpdate,  // shadow node in ss->updates
        kVirtual  // reference to virtual page in ss->vpages
    } type = kInvalid;
    pgno_t pgno = 0;

    auto operator<=>(const PageRef & other) const = default;
};
struct LocalRef {
    int pos = -1;
    int len = -1;

    auto operator<=>(const LocalRef & other) const = default;
};
struct NodeRef {
    PageRef page;
    LocalRef data;

    auto operator<=>(const NodeRef & other) const = default;
};

enum NodeType : int8_t {
    kNodeInvalid,
    kNodeSeg,
    kNodeHalfSeg,
    kNodeFork,
    kNodeRemote,
    kNodeEndMark,
};

struct UpdateBase : ListLink<> {
    int id = 0;
    NodeType type = kNodeInvalid;
    int len = -1;
    span<NodeRef> refs;
};
struct UpdateSeg : UpdateBase {
    constexpr static NodeType s_type = kNodeSeg;
    bool endOfKey = false;
    unsigned keyLen = 0;    // length in nibbles
    uint8_t * key = nullptr;
};
struct UpdateHalfSeg : UpdateBase {
    constexpr static NodeType s_type = kNodeHalfSeg;
    bool endOfKey = false;
    int nibble = 0;
};
struct UpdateFork : UpdateBase {
    constexpr static NodeType s_type = kNodeFork;
    bool endOfKey = false;
    uint16_t kidBits = 0;
};
struct UpdateRemote : UpdateBase {
    constexpr static NodeType s_type = kNodeRemote;
    PageRef page;
    int pos = -1;
};
struct UpdateEndMark : UpdateBase {
    constexpr static NodeType s_type = kNodeEndMark;
};

struct VirtualPage {
    pgno_t targetPgno = (pgno_t) -1;
    NodeRef root;

    explicit VirtualPage(TempHeap * heap);
};

struct SearchState {
    string_view key;
    int kpos = 0;
    int klen = 0;
    uint8_t kval = 0;

    // Current page
    pgno_t pgno = 0;
    int used = 0;

    // Current node
    int inode = 0;
    StrTrieBase::Node * node = nullptr;

    // Fork to adjacent key
    pgno_t fpno = (pgno_t) -1;
    int ifork = -1;
    int fpos = -1;

    // Was key found?
    bool found = false;

    int updateCount = 0;
    List<UpdateBase> updates;
    pmr::vector<VirtualPage> vpages;
    USet spages;

    TempHeap * heap = nullptr;
    IPageHeap * pages = nullptr;

    SearchState(
        string_view key,
        IPageHeap * pages,
        TempHeap * heap = nullptr
    );
};

} // namespace


/****************************************************************************
*
*   StrTrieBase::Node
*
***/

struct StrTrieBase::Node {
    uint8_t data[1];
};
static_assert(sizeof StrTrieBase::Node == 1);


/****************************************************************************
*
*   Helpers
*
***/

//===========================================================================
// Key helpers
//===========================================================================
static uint8_t keyVal(string_view key, size_t pos) {
    assert(pos < key.size() * 2);
    return pos % 2 == 0
        ? (uint8_t) key[pos / 2] >> 4
        : (uint8_t) key[pos / 2] & 0xf;
}

//===========================================================================
// Node helpers
//===========================================================================
static NodeType nodeType(const StrTrieBase::Node * node) {
    return NodeType(*node->data & 0x3);
}

//===========================================================================
static bool nodeEndMarkFlag(const StrTrieBase::Node * node) {
    return *node->data & 0x8;
}

//===========================================================================
static void setEndMarkFlag(StrTrieBase::Node * node, bool eok = true) {
    if (eok) {
        *node->data |= 0x8;
    } else {
        *node->data &= ~0x8;
    }
}

//===========================================================================
static int segLen(const StrTrieBase::Node * node) {
    assert(nodeType(node) == kNodeSeg);
    return 2 * (node->data[0] >> 4) + 2;
}

//===========================================================================
static const uint8_t * segData(const StrTrieBase::Node * node) {
    assert(nodeType(node) == kNodeSeg);
    return node->data + 1;
}

//===========================================================================
static uint8_t segVal(const StrTrieBase::Node * node, size_t pos) {
    assert(nodeType(node) == kNodeSeg);
    assert(pos < segLen(node));
    auto ptr = node->data + 1;
    return pos % 2 == 0
        ? (ptr[pos / 2] >> 4)
        : (ptr[pos / 2] & 0xf);
}

//===========================================================================
static void setSeg(StrTrieBase::Node * node, size_t len, bool eok) {
    assert(len && len <= 16);
    node->data[0] = kNodeSeg | (uint8_t(len + 1) << 4);
    setEndMarkFlag(node, eok);
}

//===========================================================================
static void pushSegVal(
    StrTrieBase::Node * node,
    size_t pos,
    uint8_t val
) {
    assert(nodeType(node) == kNodeSeg);
    assert(pos < segLen(node));
    auto ptr = node->data + 1;
    if (pos % 2 == 0) {
        ptr[pos / 2] = val << 4;
    } else {
        ptr[pos / 2] |= val;
    }
}

//===========================================================================
static uint8_t halfSegVal(const StrTrieBase::Node * node) {
    assert(nodeType(node) == kNodeHalfSeg);
    return node->data[0] >> 4;
}

//===========================================================================
static void setHalfSeg(
    StrTrieBase::Node * node,
    uint8_t val,
    bool eok
) {
    assert(val < 16);
    node->data[0] = kNodeSeg | (val << 4);
    setEndMarkFlag(node, eok);
}

//===========================================================================
static uint16_t forkBits(const StrTrieBase::Node * node) {
    assert(nodeType(node) == kNodeFork);
    return ntoh16(node->data + 1);
}

//===========================================================================
static void setForkBits(
    StrTrieBase::Node * node,
    uint16_t bits
) {
    assert(nodeType(node) == kNodeFork);
    hton16(node->data + 1, bits);
}

//===========================================================================
static size_t forkLen(const StrTrieBase::Node * node) {
    assert(nodeType(node) == kNodeFork);
    return hammingWeight(forkBits(node));
}

//===========================================================================
static pmr::vector<uint8_t> forkVals(
    SearchState * ss,
    const StrTrieBase::Node * node,
    int kval = -1
) {
    assert(nodeType(node) == kNodeFork);
    pmr::vector<uint8_t> out(ss->heap);
    auto val = forkBits(node);
    if (kval > -1)
        val |= (0x8000 >> kval);
    out.reserve(hammingWeight(val));
    for (uint8_t i = 0; val; val <<= 1) {
        if (val & 0x8000)
            out.push_back(i);
    }
    return out;
}

//===========================================================================
static bool forkBit(uint16_t bits, size_t val) {
    assert(val && val < kMaxForkBit);
    auto mask = 1u << (kMaxForkBit - val - 1);
    return bits & mask;
}

//===========================================================================
static bool forkBit(const StrTrieBase::Node * node, size_t val) {
    assert(nodeType(node) == kNodeFork);
    return forkBit(forkBits(node), val);
}

//===========================================================================
static uint16_t setForkBit(uint16_t bits, size_t val, bool enable) {
    assert(val && val < kMaxForkBit);
    auto mask = 1u << (kMaxForkBit - val - 1);
    if (enable) {
        bits |= mask;
    } else {
        bits &= ~mask;
    }
    return bits;
}

//===========================================================================
static void setForkBit(
    StrTrieBase::Node * node,
    size_t val,
    bool enable
) {
    assert(nodeType(node) == kNodeFork);
    setForkBits(node, setForkBit(forkBits(node), val, enable));
}

//===========================================================================
static int forkPos(uint16_t bits, size_t val) {
    return hammingWeight(bits >> (kMaxForkBit - val - 1));
}

//===========================================================================
static int forkPos(const StrTrieBase::Node * node, size_t val) {
    assert(nodeType(node) == kNodeFork);
    return forkPos(forkBits(node), val);
}

//===========================================================================
static void setRemoteRef(StrTrieBase::Node * node, pgno_t pgno, size_t inode) {
    node->data[0] = kNodeRemote;
    hton32(node->data + 1, pgno);
    hton16(node->data + 5, (uint16_t) inode);
}

//===========================================================================
static uint8_t endMarkKey(const StrTrieBase::Node * node) {
    assert(nodeType(node) == kNodeEndMark);
    assert(!nodeEndMarkFlag(node));
    return node->data[0] >> 4;
}

//===========================================================================
static void setEndMark(StrTrieBase::Node * node) {
    node->data[0] = kNodeEndMark;
    setEndMarkFlag(node);
}

//===========================================================================
static void setEndMark(StrTrieBase::Node * node, uint8_t lastKeyNibble) {
    node->data[0] = kNodeEndMark | (lastKeyNibble << 4);
}

//===========================================================================
static int nodeHeaderLen(const StrTrieBase::Node * node) {
    switch (nodeType(node)) {
    default:
        assert(!"Invalid node type");
        return 0;
    case kNodeSeg:
        return 1 + segLen(node) / 2;
    case kNodeHalfSeg:
        return 1;
    case kNodeFork:
        return 3;
    case kNodeRemote:
        return kRemoteNodeLen;
    case kNodeEndMark:
        return 1;
    }
}

//===========================================================================
static int nodeLen(const StrTrieBase::Node * node) {
    auto len = nodeHeaderLen(node);
    switch (nodeType(node)) {
    default:
        assert(!"Invalid node type");
        break;
    case kNodeSeg:
        if (!nodeEndMarkFlag(node))
            len += nodeLen(node + len);
        break;
    case kNodeHalfSeg:
        break;
    case kNodeFork:
        for (auto i = 0; i < forkLen(node); ++i)
            len += nodeLen(node + len);
        break;
    case kNodeRemote:
        break;
    case kNodeEndMark:
        break;
    }
    assert(len < INT_MAX);
    return len;
}

//===========================================================================
static pmr::vector<pair<int, size_t>> kids(
    SearchState * ss,
    int inode
) {
    pmr::vector<pair<int, size_t>> out(ss->heap);
    auto root = ss->node - ss->inode;
    inode += nodeHeaderLen(root);
    auto node = root + inode;
    switch (nodeType(root)) {
    case kNodeSeg:
        if (!nodeEndMarkFlag(root)) {
            auto len = nodeLen(node);
            out.emplace_back(inode, len);
            inode += len;
            node += len;
        }
        break;
    case kNodeFork:
        out.reserve(forkLen(root) + 1);
        for ([[maybe_unused]] auto&& val : forkVals(ss, root)) {
            auto len = nodeLen(node);
            out.emplace_back(inode, len);
            inode += len;
            node += len;
        }
        break;
    case kNodeRemote:
    case kNodeEndMark:
        break;
    default:
        assert(!"Invalid node type");
    }
    return out;
}

//===========================================================================
// NodeRef helpers
//===========================================================================
static void setUpdateRef(NodeRef * out, SearchState * ss) {
    out->page = { .type = PageRef::kUpdate };
    out->data = { .pos = ss->updateCount };
}

//===========================================================================
static void setSourceRef(
    NodeRef * out,
    SearchState * ss,
    size_t pos,
    size_t len
) {
    assert(pos && pos < INT_MAX);
    assert(len < INT_MAX);
    out->page = { .type = PageRef::kSource, .pgno = ss->pgno };
    out->data = { .pos = (int) pos, .len = (int) len };
}

//===========================================================================
static void setRemoteRef(NodeRef * out, SearchState * ss, pgno_t pgno) {
    out->page = { .type = PageRef::kVirtual, .pgno = pgno };
    out->data = { .pos = 0, .len = kRemoteNodeLen };
}

//===========================================================================
// UpdateNode helpers
//===========================================================================
static size_t nodeLen(const UpdateBase & upd) {
    size_t len = 0;
    switch(upd.type) {
    case kNodeSeg:
        len = 1 + static_cast<const UpdateSeg &>(upd).keyLen / 2;
        break;
    case kNodeHalfSeg:
        len = 1;
        break;
    case kNodeFork:
        len = 3;
        break;
    case kNodeRemote:
        len = kRemoteNodeLen;
        break;
    case kNodeEndMark:
        len = 1;
        break;
    default:
        assert(!"Invalid node type");
    }
    for (auto&& kid : upd.refs) {
        assert(kid.data.pos > -1 && kid.data.len > -1);
        len += kid.data.len;
    }
    return len;
}

//===========================================================================
// SearchState helpers
//===========================================================================
static void allocPage(SearchState * ss) {
    ss->pgno = (pgno_t) ss->pages->create();
}

//===========================================================================
static void seekPage(SearchState * ss, pgno_t pgno) {
    ss->spages.insert(pgno);
    ss->pgno = pgno;
}

//===========================================================================
static void seekRootPage(SearchState * ss) {
    seekPage(ss, (pgno_t) ss->pages->root());
}

//===========================================================================
static StrTrieBase::Node * getNode(SearchState * ss, size_t pos) {
    auto ptr = ss->pages->ptr(ss->pgno);
    assert(ptr);
    assert(pos < ss->pages->pageSize());
    return (StrTrieBase::Node *) (ptr + pos);
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
*   VirtualPage
*
***/

//===========================================================================
VirtualPage::VirtualPage(TempHeap * heap)
{}


/****************************************************************************
*
*   SearchState
*
***/

//===========================================================================
SearchState::SearchState(string_view key, IPageHeap * pages, TempHeap * heap)
    : key(key)
    , kpos(0)
    , klen((int) key.size() * 2)
    , kval(keyVal(key, kpos))
    , pages(pages)
    , heap(heap)
    , vpages(heap)
    , spages(heap)
{}


/****************************************************************************
*
*   Insert
*
***/

//===========================================================================
// Allocs NodeRef pointing at next update node.
static NodeRef * newRef(SearchState * ss) {
    auto ref = ss->heap->emplace<NodeRef>();
    setUpdateRef(ref, ss);
    return ref;
}

//===========================================================================
// Allocs NodeRef pointing at position on current source page.
static NodeRef * newRef(SearchState * ss, ptrdiff_t srcPos, size_t srcLen) {
    assert(srcPos >= 0);
    auto ref = ss->heap->emplace<NodeRef>();
    setSourceRef(ref, ss, srcPos, srcLen);
    return ref;
}

//===========================================================================
// Allocs NodeRef pointing at node on remote virtual page.
static NodeRef * newRef(SearchState * ss, pgno_t pgno) {
    assert(pgno >= 0);
    auto ref = ss->heap->emplace<NodeRef>();
    setRemoteRef(ref, ss, pgno);
    return ref;
}

//===========================================================================
template <typename T>
static T & newUpdate(SearchState * ss) {
    auto * upd = ss->heap->emplace<T>();
    ss->updates.link(upd);
    upd->id = ss->updateCount;
    upd->type = T::s_type;
    ss->updateCount += 1;
    return *upd;
}

//===========================================================================
static UpdateSeg & copySeg(
    SearchState * ss,
    size_t pos,
    size_t len,
    int halfSeg = -1
) {
    assert(nodeType(ss->node) == kNodeSeg);
    assert(pos + len <= segLen(ss->node));
    assert(len && (len + (halfSeg > -1)) % 2 == 0);
    assert(halfSeg >= -1 && halfSeg <= 15);
    auto & upd = newUpdate<UpdateSeg>(ss);
    upd.keyLen = (int) len + (halfSeg > -1);
    auto len8 = upd.keyLen / 2;
    upd.key = ss->heap->alloc<uint8_t>(len8);
    auto data = segData(ss->node);
    if (pos % 2 == 0) {
        memcpy(upd.key, data + pos / 2, len8);
    } else {
        auto epos = pos + len;
        auto out = upd.key;
        uint8_t val = data[pos / 2] & 0xf;
        for (pos += 1; pos < epos; pos += 2) {
            *out++ = (val << 4) + (data[pos / 2] >> 4);
            val = data[pos / 2] & 0xf;
        }
    }
    if (halfSeg > -1) {
        upd.key[len8 - 1] |= halfSeg;
    }
    if (nodeEndMarkFlag(ss->node)) {
        upd.endOfKey = true;
    } else {
        auto ref = newRef(ss);
        upd.refs = {ref, 1};
    }
    return upd;
}

//===========================================================================
// If the search should continue:
//      returns true
//      sets ss->node and ss->inode to the next node
// If the search is over:
//      returns false
//      sets ss->found to true if the key was found, otherwise false
static bool insertAtSeg(SearchState * ss) {
    // Will either split on first, middle, last, after last (possible if it's
    // the end of the key), or will advance to next node.

    auto spos = (uint8_t) 0; // split point where key diverges from segment
    auto slen = segLen(ss->node);
    auto sval = segVal(ss->node, spos);
    assert(slen > 0);

    for (;;) {
        if (ss->kpos == ss->klen) {
            // Fork key end with position in the segment
            break;
        }
        if (ss->kval != sval) {
            // Fork in the segment.
            break;
        }
        if (++ss->kpos != ss->klen)
            ss->kval = keyVal(ss->key, ss->kpos);
        if (++spos == slen) {
            if (nodeEndMarkFlag(ss->node)) {
                if (ss->kpos == ss->klen) {
                    // End of both segment and key? Key was found, abort.
                    ss->found = true;
                    return false;
                }
                // Fork with the end mark that's after the segment.
                break;
            }
            // Continue search at next node
            copySeg(ss, 0, slen);
            seekNode(ss, ss->inode + nodeHeaderLen(ss->node));
            return true;
        }
        sval = segVal(ss->node, spos);
    }

    // Replace segment with:
    //      [lead seg] [lead half seg]
    //      fork
    //      [tail seg] [tail half seg]
    assert(spos <= slen);
    if (spos > 1) {
        copySeg(ss, 0, spos / 2);
    }
    if (spos % 2 == 1) {
        auto & halfSeg = newUpdate<UpdateHalfSeg>(ss);
        halfSeg.nibble = segVal(ss->node, spos - 1);
        auto ref = newRef(ss);
        halfSeg.refs = {ref, 1};
    }
    auto & fork = newUpdate<UpdateFork>(ss);
    if (spos == slen) {
        assert(ss->kpos < ss->klen);
        fork.endOfKey = true;
        auto ref = newRef(ss);
        fork.refs = {ref, 1};
        fork.kidBits = 1u << (kMaxForkBit - ss->kval - 1);
    } else {
        auto tail = ss->node + nodeHeaderLen(ss->node);
        auto nrefs = 2;
        fork.kidBits = 1u << (kMaxForkBit - sval - 1);
        if (ss->kpos == ss->klen) {
            fork.endOfKey = true;
            nrefs = 1;
        } else {
            fork.kidBits |= 1u << (kMaxForkBit - ss->kval - 1);
        }
        fork.refs = ss->heap->allocSpan<NodeRef>(nrefs);
        fork.refs[0].page = { .type = PageRef::kUpdate };
        fork.refs[0].data.pos = ss->updateCount;
        if ((slen - spos) % 2 == 1 && nodeType(tail) == kNodeHalfSeg) {
            // Combined seg with following halfSeg.
            auto & seg = copySeg(
                ss,
                spos,
                (slen - spos) & ~1,
                halfSegVal(tail)
            );
            if (nodeEndMarkFlag(tail)) {
                seg.endOfKey = true;
            } else {
                // Add ref to node after following halfSeg.
                auto inext = ss->inode + tail - ss->node + nodeHeaderLen(tail);
                auto ref = newRef(
                    ss,
                    inext,
                    nodeLen(ss->node - ss->inode + inext)
                );
                seg.refs = {ref, 1};
            }
        } else if (slen - spos > 1) {
            // Add seg.
            copySeg(ss, spos, (slen - spos) & ~1);
            if ((slen - spos) % 2 == 1) {
                // Add following halfSeg.
                auto & halfSeg = newUpdate<UpdateHalfSeg>(ss);
                halfSeg.nibble = segVal(ss->node, slen - 1);
                auto ref = newRef(
                    ss,
                    ss->inode + tail - ss->node,
                    nodeLen(tail)
                );
                halfSeg.refs = {ref, 1};
            }
        }

        if (ss->kpos == ss->klen) {
            ss->found = false;
            return false;
        }

        setUpdateRef(&fork.refs[1], ss);
        if (ss->kval < sval)
            swap(fork.refs[0], fork.refs[1]);
    }
    // Continue processing the key.
    if (++ss->kpos == ss->klen) {
        [[maybe_unused]] auto & eok = newUpdate<UpdateEndMark>(ss);
    } else {
        ss->kval = keyVal(ss->key, ss->kpos);
    }
    ss->found = false;
    return false;
}

//===========================================================================
static UpdateFork & copyFork(SearchState * ss, int * inext) {
    assert(nodeType(ss->node) == kNodeFork);
    auto & upd = newUpdate<UpdateFork>(ss);
    if (ss->kpos == ss->klen) {
        upd.endOfKey = true;
        auto count = forkLen(ss->node);
        upd.refs = ss->heap->allocSpan<NodeRef>(count);
        auto len = nodeHeaderLen(ss->node);
        for (auto i = 0; i < count; ++i) {
            setSourceRef(
                &upd.refs[i],
                ss,
                ss->inode + len,
                nodeLen(ss->node + len)
            );
            len += upd.refs[i].data.len;
        }
        return upd;
    }
    upd.endOfKey = nodeEndMarkFlag(ss->node);
    upd.kidBits = setForkBit(forkBits(ss->node), ss->kval, true);
    auto vals = forkVals(ss, ss->node, ss->kval);
    upd.refs = ss->heap->allocSpan<NodeRef>(vals.size());
    auto i = 0;
    auto len = nodeHeaderLen(ss->node);
    for (; vals[i] < ss->kval; ++i) {
        setSourceRef(
            &upd.refs[i],
            ss,
            ss->inode + len,
            nodeLen(ss->node + len)
        );
        len += upd.refs[i].data.len;
    }
    *inext = ss->inode + len;
    setUpdateRef(&upd.refs[i], ss);
    if (upd.kidBits == forkBits(ss->node))
        len += nodeLen(ss->node + len);
    i += 1;
    for (; i < vals.size(); ++i) {
        setSourceRef(
            &upd.refs[i],
            ss,
            ss->inode + len,
            nodeLen(ss->node + len)
        );
        len += upd.refs[i].data.len;
    }
    return upd;
}

//===========================================================================
// Returns true if there is more to do; otherwise false and sets ss->found
static bool insertAtFork (SearchState * ss) {
    // Will either add branch to fork, or will advance to next node.

    int inext;
    if (ss->kpos == ss->klen) {
        if (!nodeEndMarkFlag(ss->node)) {
            copyFork(ss, &inext);
            ss->found = false;
        } else {
            ss->found = true;
        }
        return false;
    }

    copyFork(ss, &inext);
    seekNode(ss, inext);
    ss->found = false;
    return false;
}

//===========================================================================
static void addSegs(SearchState * ss) {
    assert(ss->kpos <= ss->klen);
    while (int slen = ss->klen - ss->kpos) {
        if (slen == 1) {
            auto & upd = newUpdate<UpdateHalfSeg>(ss);
            upd.endOfKey = true;
            upd.nibble = keyVal(ss->key, ss->kpos);
            return;
        }
        auto & upd = newUpdate<UpdateSeg>(ss);
        if (slen > kMaxSegLen) {
            upd.keyLen = kMaxSegLen;
        } else {
            upd.keyLen = slen;
            upd.endOfKey = true;
        }
        upd.key = ss->heap->alloc<uint8_t>((upd.keyLen + 1) / 2);
        for (auto spos = 0u; spos < upd.keyLen; ++spos, ++ss->kpos) {
            auto val = keyVal(ss->key, ss->kpos);
            if (spos % 2 == 0) {
                upd.key[spos / 2] = val << 4;
            } else {
                upd.key[spos / 2] |= val;
            }
        }
    }
}

//===========================================================================
static void copySeg(
    SearchState * ss,
    const VirtualPage & vpage,
    const UpdateBase & upd
) {
}

//===========================================================================
static void copyFork(
    SearchState * ss,
    const VirtualPage & vpage,
    const UpdateBase & upd
) {
}

//===========================================================================
static void copyAny(
    SearchState * ss,
    const VirtualPage & vpage,
    const UpdateBase & upd
) {
    if (upd.type == kNodeSeg) {
        copySeg(ss, vpage, upd);
    } else {
        assert(upd.type == kNodeFork);
        copyFork(ss, vpage, upd);
    }
}

//===========================================================================
static void applyUpdates(SearchState * ss) {
    assert(ss->updateCount == ss->updates.size());
    // From updated nodes, generate:
    //  - child to parent graph
    //  - vector of leaf nodes
    //  - set of involved source pages
    struct UpdateInfo {
        UpdateBase * upd = {};
        int parent = -1;
        size_t kidPos = 0;

        // for leaf: 0
        // for branch: 1 + number of unprocessed kids
        size_t unprocessed = 0;
    };
    vector<UpdateInfo> infos(ss->updateCount);
    vector<int> unblocked;
    for (auto&& upd : ss->updates) {
        auto id = upd.id;
        auto & ui = infos[id];
        ui.upd = &upd;
        bool branch = false;
        auto pos = -1;
        for (auto&& kid : ui.upd->refs) {
            pos += 1;
            if (kid.page.type == PageRef::kUpdate) {
                ui.unprocessed += 1;
                infos[kid.data.pos].parent = id;
                infos[kid.data.pos].kidPos = pos;
                branch = true;
            } else {
                assert(kid.page.type == PageRef::kSource);
                branch = true;
            }
        }
        if (branch)
            ui.unprocessed += 1;
        if (ui.unprocessed < 2)
            unblocked.push_back(id);
    }

    // Process updated nodes and related fringe nodes into virtual pages. Start
    // with leaf nodes, with branches becoming unblocked for processing when
    // all their leaves are done. Continues all the way up until the root node
    // is processed. Child virtual pages are merged up into their parents when
    // possible.
    while (!unblocked.empty()) {
        auto id = unblocked.back();
        unblocked.pop_back();
        auto & ui = infos[id];
        auto & upd = *ui.upd;
        upd.len = (int) nodeLen(upd);
        if (ui.unprocessed) {
            // internal branch
            assert(ui.unprocessed == 1);
            if (upd.len > ss->pages->pageSize()) {
                for (auto&& ref : upd.refs) {
                    auto & vpage = ss->vpages.emplace_back();
                    vpage.root = ref;
                    setRemoteRef(&ref, ss, (pgno_t) ss->vpages.size() - 1);
                }
                upd.len = (int) nodeLen(upd);
            }
        }

        // Update reference from parent
        if (ui.parent == -1) {
            // Adding root node as a leaf, must be adding first key to this
            // previously empty container.
            assert(unblocked.empty());
        } else {
            auto & pi = infos[ui.parent];
            assert(pi.unprocessed > 1);
            if (--pi.unprocessed == 1) {
                // Parent has had all children processed, queue for
                // processing.
                unblocked.push_back(ui.parent);
            }
        }
    }

    // Find best virtual to source page replacements
    USet spages = ss->spages;
    USet vpages;
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
        ss->pages->destroy(id);
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
        USet used;
        vpage.targetNodes = 0;
        for (auto&& kv : vpage.nodes) {
            vpage.targetNodes += (int) kv.second.size();
            if (kv.first.pgno == vpage.root.page.pgno)
                used.insert(kv.second);
        }
        seekPage(ss, vpage.targetPgno);

        // All nodes less than the total that haven't already been used
        // are available.
        USet avail(0, vpage.targetNodes);
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
// Returns true if key was inserted, false if it was already present.
bool StrTrieBase::insert(string_view key) {
    TempHeap heap;
    auto ss = heap.emplace<SearchState>(key, m_pages, &heap);
    if (empty()) {
        allocPage(ss);
    } else {
        seekRootPage(ss);
        seekNode(ss, ss->inode);
        for (;;) {
            switch (auto ntype = ::nodeType(ss->node)) {
            case kNodeSeg:
                if (insertAtSeg(ss))
                    continue;
                break;
            case kNodeFork:
                if (insertAtFork(ss))
                    continue;
                break;
            default:
                logMsgFatal() << "Invalid StrTrieBase node type: " << ntype;
            }

            if (ss->found)
                return false;
            break;
        }
    }

    // Add any trailing key segments.
    addSegs(ss);

    // Apply pending updates
    if (!ss->updates.empty())
        applyUpdates(ss);

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
    if (ss->kpos == ss->klen) {
        ss->found = nodeEndMarkFlag(ss->node);
        return false;
    }
    if (!forkBit(ss->node, ss->kval))
        return false;

    auto vals = forkVals(ss, ss->node);
    auto subs = kids(ss, ss->inode);
    auto i = 0;
    for (; i < vals.size(); ++i) {
        if (vals[i] == ss->kval) {
            ss->fpno = ss->pgno;
            ss->ifork = subs[i].first;
            ss->fpos = ss->kpos;
            break;
        }
    }
    ss->kval = keyVal(ss->key, ++ss->kpos);
    auto sub = subs.back();
    seekNode(ss, sub.first + sub.second);
    return true;
}

//===========================================================================
// Returns true if there is more to do, otherwise false and sets ss->found if
// key was matched.
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
    auto result = false;
    if (ss->kpos == ss->klen) {
        ss->found = nodeEndMarkFlag(ss->node);
        return false;
    }
    seekNode(ss, ss->inode + nodeHeaderLen(ss->node));
    return !nodeEndMarkFlag(ss->node);
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

//===========================================================================
bool StrTrieBase::erase(string_view key) {
    if (empty())
        return false;

    TempHeap heap;
    auto ss = heap.emplace<SearchState>(key, m_pages, &heap);
    seekRootPage(ss);

    findLastFork(ss);
    if (!ss->found)
        return false;

    seekNode(ss, ss->ifork);
    ss->kpos = ss->fpos;

    return true;
}


/****************************************************************************
*
*   Find
*
***/

//===========================================================================
static bool containsAtSeg(SearchState * ss) {
    auto slen = segLen(ss->node);
    if (ss->klen - ss->kpos < slen)
        return false;
    for (auto spos = 0; spos < slen; ++spos) {
        auto sval = segVal(ss->node, spos);
        if (sval != ss->kval)
            return false;
        ss->kval = keyVal(ss->key, ++ss->kpos);
    }
    if (ss->kpos == ss->klen) {
        ss->found = nodeEndMarkFlag(ss->node);
        return false;
    }
    seekNode(ss, ss->inode + nodeHeaderLen(ss->node));
    return !nodeEndMarkFlag(ss->node);
}

//===========================================================================
static bool containsAtFork(SearchState * ss) {
    if (ss->kpos == ss->klen) {
        ss->found = nodeEndMarkFlag(ss->node);
        return false;
    }
    if (!forkBit(ss->node, ss->kval))
        return false;

    auto inext = ss->inode + nodeHeaderLen(ss->node);
    auto pos = forkPos(ss->node, ss->kval);
    for (auto i = 0; i < pos; ++i) {
        inext += nodeLen(ss->node + inext);
    }
    seekNode(ss, inext);
    return true;
}

//===========================================================================
bool StrTrieBase::contains(string_view key) const {
    if (empty())
        return false;

    TempHeap heap;
    auto ss = heap.emplace<SearchState>(key, m_pages, &heap);
    seekRootPage(ss);
    seekNode(ss, ss->inode);

    for (;;) {
        switch (auto ntype = ::nodeType(ss->node)) {
        case kNodeSeg:
            if (containsAtSeg(ss))
                continue;
            break;
        case kNodeFork:
            if (containsAtFork(ss))
                continue;
            break;
        default:
            logMsgFatal() << "Invalid StrTrieBase node type: " << ntype;
        }
        return ss->found;
    }
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
static ostream & dumpAny(ostream & os, SearchState * ss) {
    auto len = nodeHeaderLen(ss->node);
    for (auto i = 0; i < len; ++i) {
        if (i % 4 == 2) os.put(' ');
        if (auto val = ss->node->data[i]) {
            hexByte(os, ss->node->data[i]);
        } else {
            os << "--";
        }
    }
    os << ' ';
    switch (auto ntype = ::nodeType(ss->node)) {
    case kNodeFork:
        os << " Fork";
        if (nodeEndMarkFlag(ss->node))
            os << ", EOK";
        break;
    case kNodeSeg:
        os << " Segment[" << segLen(ss->node) << "]";
        if (nodeEndMarkFlag(ss->node))
            os << ", EOK";
        break;
    default:
        os << " UNKNOWN(" << ntype << ")";
        break;
    }
    auto subs = kids(ss, ss->inode);
    if (!subs.empty()) {
        os << "// ";
        for (auto & kid : subs)
            os << kid.first << ' ';
    }
    os << '\n';
    for (auto & kid : subs) {
        seekNode(ss, kid.first);
        dumpAny(os, ss);
    }
}

//===========================================================================
ostream & StrTrieBase::dump(ostream & os) const {
    if (empty())
        return os;

    TempHeap heap;
    auto ss = heap.emplace<SearchState>(string_view{}, m_pages, &heap);
    seekRootPage(ss);
    seekNode(ss, 0);
    return dumpAny(os, ss);
}
