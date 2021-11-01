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
    kNodeEndMark,
    kNodeRemote,
    kNodeTypes
};
static_assert(kNodeTypes <= 8);

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
struct UpdateEndMark : UpdateBase {
    constexpr static NodeType s_type = kNodeEndMark;
};
struct UpdateRemote : UpdateBase {
    constexpr static NodeType s_type = kNodeRemote;
    PageRef page;
    int pos = -1;
};

struct VirtualPage {
    pgno_t targetPgno = (pgno_t) -1;
    NodeRef root;

    explicit VirtualPage(TempHeap * heap);
};

struct SearchFork {
    pgno_t pgno = (pgno_t) -1;
    int inode = -1;
    int fpos = -1;
    int kpos = -1;
};
struct SearchState {
    string_view key;
    int kpos = 0;
    int klen = 0;
    uint8_t kval = 0;

    // Current page
    pgno_t pgno = 0;

    // Current node
    int inode = 0;
    StrTrieBase::Node * node = nullptr;

    // Forks taken in search
    pmr::deque<SearchFork> forks;

    // Was key found?
    bool found = false;
    pmr::string foundKey;
    size_t foundKeyLen = 0;

    pmr::deque<UpdateBase *> updates;
    pmr::deque<VirtualPage> vpages;
    USet spages;

    TempHeap * heap = nullptr;
    IPageHeap * pages = nullptr;

    ostream * debugStream = nullptr;

    SearchState(
        string_view key,
        IPageHeap * pages,
        TempHeap * heap = nullptr,
        ostream * debugStream = nullptr
    );
};

} // namespace

// forward declarations
static ostream & dumpPage(ostream & os, SearchState * ss);


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
    return NodeType(*node->data & 0x7);
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
static void setSeg(
    StrTrieBase::Node * node,
    bool eok,
    size_t len,
    const uint8_t * key
) {
    assert(len && len <= 32 && len % 2 == 0);
    node->data[0] = kNodeSeg | (uint8_t(len / 2 - 1) << 4);
    setEndMarkFlag(node, eok);
    if (key)
        memcpy(node->data + 1, key, len / 2);
}

//===========================================================================
static uint8_t halfSegVal(const StrTrieBase::Node * node) {
    assert(nodeType(node) == kNodeHalfSeg);
    return node->data[0] >> 4;
}

//===========================================================================
static void setHalfSeg(
    StrTrieBase::Node * node,
    bool eok,
    size_t val
) {
    assert(val < 16);
    node->data[0] = kNodeHalfSeg | (int8_t(val) << 4);
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
static int nextForkVal(uint16_t bits, int kval) {
    assert(kval && kval < 16);
    auto mask = (1u << (kMaxForkBit - kval - 1)) - 1;
    auto nbits = bits & mask;
    if (!nbits)
        return -1;
    auto nval = leadingZeroBits(nbits);
    return nval;
}

//===========================================================================
inline static int prevForkVal(uint16_t bits, int kval) {
    assert(kval && kval < 16);
    auto mask = 65535u << (kMaxForkBit - kval);
    auto nbits = bits & mask;
    if (!nbits)
        return -1;
    auto nval = kMaxForkBit - trailingZeroBits(nbits);
    return (int) nval;
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
    for (uint8_t i = 0; val; ++i, val <<= 1) {
        if (val & 0x8000)
            out.push_back(i);
    }
    return out;
}

//===========================================================================
static bool forkBit(uint16_t bits, size_t val) {
    assert(val < kMaxForkBit);
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
    assert(val < kMaxForkBit);
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
    UpdateFork * fork,
    size_t val,
    bool enable
) {
    fork->kidBits = setForkBit(fork->kidBits, val, enable);
}

//===========================================================================
static int forkPos(uint16_t bits, size_t val) {
    assert(val < kMaxForkBit);
    return hammingWeight(bits >> (kMaxForkBit - val));
}

//===========================================================================
static int forkPos(const StrTrieBase::Node * node, size_t val) {
    assert(nodeType(node) == kNodeFork);
    return forkPos(forkBits(node), val);
}

//===========================================================================
static void setFork(
    StrTrieBase::Node * node,
    bool eok,
    uint16_t bits
) {
    node->data[0] = kNodeFork;
    setEndMarkFlag(node, eok);
    setForkBits(node, bits);
}

//===========================================================================
static void setEndMark(StrTrieBase::Node * node) {
    node->data[0] = kNodeEndMark;
    setEndMarkFlag(node);
}

//===========================================================================
static pgno_t remotePage(StrTrieBase::Node * node) {
    assert(nodeType(node) == kNodeRemote);
    return (pgno_t) ntoh32(node->data + 1);
}

//===========================================================================
static int remotePos(StrTrieBase::Node * node) {
    assert(nodeType(node) == kNodeRemote);
    return (int) ntoh16(node->data + 5);
}

//===========================================================================
static void setRemoteRef(StrTrieBase::Node * node, pgno_t pgno, size_t inode) {
    node->data[0] = kNodeRemote;
    hton32(node->data + 1, pgno);
    hton16(node->data + 5, (uint16_t) inode);
}

//===========================================================================
static int nodeHdrLen(const StrTrieBase::Node * node) {
    switch (auto type = nodeType(node)) {
    default:
        assert(type == kNodeInvalid);
        assert(!"Invalid node type");
        return 0;
    case kNodeSeg:
        return 1 + segLen(node) / 2;
    case kNodeHalfSeg:
        return 1;
    case kNodeFork:
        return 3;
    case kNodeEndMark:
        return 1;
    case kNodeRemote:
        return kRemoteNodeLen;
    }
}

//===========================================================================
static int nodeLen(const StrTrieBase::Node * node) {
    auto len = nodeHdrLen(node);
    switch (auto type = nodeType(node)) {
    default:
        assert(type == kNodeInvalid);
        assert(!"Invalid node type");
        break;
    case kNodeSeg:
    case kNodeHalfSeg:
        if (!nodeEndMarkFlag(node))
            len += nodeLen(node + len);
        break;
    case kNodeFork:
        for (auto i = 0; i < forkLen(node); ++i)
            len += nodeLen(node + len);
        break;
    case kNodeEndMark:
        break;
    case kNodeRemote:
        break;
    }
    assert(len > 0 && len < INT_MAX);
    return len;
}

//===========================================================================
// Returns vector of inode and nodeLen pairs.
static pmr::vector<pair<int, size_t>> kids(
    SearchState * ss,
    int inode
) {
    assert(inode < ss->pages->pageSize());
    pmr::vector<pair<int, size_t>> out(ss->heap);
    auto root = ss->node - ss->inode + inode;
    auto len = nodeHdrLen(root);
    auto node = root + len;
    inode += len;
    switch (nodeType(root)) {
    case kNodeSeg:
    case kNodeHalfSeg:
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
    case kNodeEndMark:
    case kNodeRemote:
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
    out->data = { .pos = (int) ss->updates.size() };
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
[[maybe_unused]]
static NodeRef * newRef(SearchState * ss, pgno_t pgno) {
    assert(pgno >= 0);
    auto ref = ss->heap->emplace<NodeRef>();
    setRemoteRef(ref, ss, pgno);
    return ref;
}

//===========================================================================
// UpdateNode helpers
//===========================================================================
static int nodeLen(const UpdateBase & upd) {
    int len = 0;
    switch(upd.type) {
    default:
        assert(!"Invalid node type");
        break;
    case kNodeSeg:
        len = 1 + static_cast<const UpdateSeg &>(upd).keyLen / 2;
        break;
    case kNodeHalfSeg:
        len = 1;
        break;
    case kNodeFork:
        len = 3;
        break;
    case kNodeEndMark:
        len = 1;
        break;
    case kNodeRemote:
        len = kRemoteNodeLen;
        break;
    }
    for (auto&& kid : upd.refs) {
        assert(kid.data.pos > -1 && kid.data.len > -1);
        len += kid.data.len;
    }
    return len;
}

//===========================================================================
template <typename T>
static T & newUpdate(SearchState * ss) {
    auto * upd = ss->heap->emplace<T>();
    upd->id = (int) ss->updates.size();
    upd->type = T::s_type;
    ss->updates.push_back(upd);
    return *upd;
}

//===========================================================================
// SearchState helpers
//===========================================================================
static void seekPage(SearchState * ss, pgno_t pgno) {
    ss->pgno = pgno;
}

//===========================================================================
static void seekRootPage(SearchState * ss) {
    seekPage(ss, (pgno_t) ss->pages->root());
}

//===========================================================================
static void allocPage(SearchState * ss) {
    ss->pgno = (pgno_t) ss->pages->create();
}

//===========================================================================
static StrTrieBase::Node * getNode(
    SearchState * ss,
    pgno_t pgno,
    size_t pos
) {
    auto ptr = ss->pages->ptr(pgno);
    assert(ptr);
    assert(pos < ss->pages->pageSize() + 1);
    return (StrTrieBase::Node *) (ptr + pos);
}

//===========================================================================
static StrTrieBase::Node * getNode(SearchState * ss, size_t pos) {
    return getNode(ss, ss->pgno, pos);
}

//===========================================================================
static void seekNode(SearchState * ss, size_t inode) {
    ss->inode = (int) inode;
    ss->node = getNode(ss, ss->inode);
}

//===========================================================================
static void seekKid(SearchState * ss, size_t pos) {
    auto root = ss->node - ss->inode;
    auto inext = ss->inode + nodeHdrLen(ss->node);
    for (auto i = 0; i < pos; ++i)
        inext += nodeLen(root + inext);
    seekNode(ss, inext);
}

//===========================================================================
static bool logFatal(SearchState * ss) {
    logMsgFatal() << "Invalid StrTrieBase node type: "
        << nodeType(ss->node);
    return false;
}


/****************************************************************************
*
*   StrTrieBase::Iter
*
***/

//===========================================================================
StrTrieBase::Iter & StrTrieBase::Iter::operator++() {
    return *this;
}


/****************************************************************************
*
*   StrTrie
*
***/

//===========================================================================
void StrTrie::clear() {
    m_heapImpl.clear();
}

//===========================================================================
std::ostream * const StrTrie::debugStream() const {
    return m_debug ? &cout : nullptr;
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
SearchState::SearchState(
    string_view key,
    IPageHeap * pages,
    TempHeap * heap,
    ostream * dstrm
)
    : key(key)
    , klen((int) key.size() * 2)
    , pages(pages)
    , heap(heap)
    , forks(heap)
    , foundKey(heap)
    , updates(heap)
    , vpages(heap)
    , spages(heap)
    , debugStream(dstrm)
{
    if (klen)
        kval = keyVal(key, kpos);
}


/****************************************************************************
*
*   Insert
*
***/

//===========================================================================
static UpdateSeg & copySeg(
    SearchState * ss,
    size_t pos,
    size_t len,
    bool eok,
    NodeRef * nextRef = nullptr,
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
        for (; pos < epos; pos += 2) {
            uint8_t val = segVal(ss->node, pos) << 4;
            if (pos + 1 < epos)
                val |= segVal(ss->node, pos + 1);
            *out++ = val;
        }
    }
    if (halfSeg > -1)
        upd.key[len8 - 1] |= halfSeg;
    if (eok) {
        upd.endOfKey = true;
    } else {
        if (!nextRef)
            nextRef = newRef(ss);
        upd.refs = { nextRef, 1 };
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

    auto spos = 0; // split point where key diverges from segment
    auto slen = segLen(ss->node);
    auto sval = segVal(ss->node, spos);
    assert(slen > 0);

    for (;;) {
        if (ss->kpos == ss->klen) {
            // Fork end of key with position in the segment.
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
            copySeg(ss, 0, slen, false);
            seekNode(ss, ss->inode + nodeHdrLen(ss->node));
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
        // Add lead seg.
        copySeg(ss, 0, spos & ~1, false);
    }
    if (spos % 2 == 1) {
        // Add lead halfSeg.
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
        setForkBit(&fork, ss->kval, true);
    } else {
        auto tail = ss->node + nodeHdrLen(ss->node);
        auto nrefs = 2;
        setForkBit(&fork, sval, true);
        spos += 1; // Advance past the val that's in the new fork node.
        if (ss->kpos == ss->klen) {
            fork.endOfKey = true;
            nrefs = 1;
        } else {
            setForkBit(&fork, ss->kval, true);
        }
        fork.refs = ss->heap->allocSpan<NodeRef>(nrefs);

        if (spos == slen) {
            if (nodeEndMarkFlag(ss->node)) {
                setUpdateRef(&fork.refs[0], ss);
                newUpdate<UpdateEndMark>(ss);
            } else {
                setSourceRef(
                    &fork.refs[0],
                    ss,
                    ss->inode + tail - ss->node,
                    nodeLen(tail)
                );
            }
        } else if ((slen - spos) % 2 == 1 && nodeType(tail) == kNodeHalfSeg) {
            // Combined rest of seg with following halfSeg.
            setUpdateRef(&fork.refs[0], ss);
            auto ref = (NodeRef *) nullptr;
            if (!nodeEndMarkFlag(tail)) {
                // Ref to node after following halfSeg.
                auto inext = ss->inode + tail - ss->node + nodeHdrLen(tail);
                ref = newRef(
                    ss,
                    inext,
                    nodeLen(ss->node - ss->inode + inext)
                );
            }
            copySeg(ss, spos, slen - spos, !ref, ref, halfSegVal(tail));
        } else {
            setUpdateRef(&fork.refs[0], ss);
            if (slen - spos > 1) {
                // Add seg part of the rest of the seg.
                auto ref = (NodeRef *) nullptr;
                if ((slen - spos) % 2 == 1) {
                    copySeg(ss, spos, (slen - spos) & ~1, false);
                } else {
                    if (!nodeEndMarkFlag(ss->node)) {
                        auto inext = ss->inode + nodeHdrLen(ss->node);
                        ref = newRef(
                            ss,
                            inext,
                            nodeLen(ss->node - ss->inode + inext)
                        );
                    }
                    copySeg(ss, spos, slen - spos, !ref, ref);
                }
            }
            if ((slen - spos) % 2 == 1) {
                // Add halfSeg part of the rest of the seg.
                auto & halfSeg = newUpdate<UpdateHalfSeg>(ss);
                halfSeg.nibble = segVal(ss->node, slen - 1);
                if (nodeEndMarkFlag(ss->node)) {
                    halfSeg.endOfKey = true;
                } else {
                    auto ref = newRef(
                        ss,
                        ss->inode + tail - ss->node,
                        nodeLen(tail)
                    );
                    halfSeg.refs = {ref, 1};
                }
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
        newUpdate<UpdateEndMark>(ss);
    } else {
        ss->kval = keyVal(ss->key, ss->kpos);
    }
    ss->found = false;
    return false;
}

//===========================================================================
static UpdateHalfSeg & copyHalfSeg(SearchState * ss) {
    assert(nodeType(ss->node) == kNodeHalfSeg);
    auto & upd = newUpdate<UpdateHalfSeg>(ss);
    upd.nibble = halfSegVal(ss->node);
    if (nodeEndMarkFlag(ss->node)) {
        upd.endOfKey = true;
    } else {
        auto ref = newRef(ss);
        upd.refs = { ref, 1 };
    }
    return upd;
}

//===========================================================================
// Returns true if there is more to do; otherwise false and sets ss->found
static bool insertAtHalfSeg (SearchState * ss) {
    // Will either split on key or will advance to next node.

    auto sval = halfSegVal(ss->node);

    for (;;) {
        if (ss->kpos == ss->klen)
            break;
        if (ss->kval != sval)
            break;
        if (++ss->kpos != ss->klen)
            ss->kval = keyVal(ss->key, ss->kpos);
        if (nodeEndMarkFlag(ss->node)) {
            if (ss->kpos == ss->klen) {
                ss->found = true;
                return false;
            }
            break;
        }
        // Continue search at next node
        copyHalfSeg(ss);
        seekNode(ss, ss->inode + nodeHdrLen(ss->node));
        return true;
    }

    auto & fork = newUpdate<UpdateFork>(ss);
    auto tail = ss->node + nodeHdrLen(ss->node);
    auto nrefs = 2;
    setForkBit(&fork, sval, true);
    if (ss->kpos == ss->klen) {
        fork.endOfKey = true;
        nrefs = 1;
    } else {
        setForkBit(&fork, ss->kval, true);
    }
    fork.refs = ss->heap->allocSpan<NodeRef>(nrefs);
    if (nodeEndMarkFlag(ss->node)) {
        setUpdateRef(&fork.refs[0], ss);
        newUpdate<UpdateEndMark>(ss);
    } else {
        setSourceRef(
            &fork.refs[0],
            ss,
            ss->inode + tail - ss->node,
            nodeLen(tail)
        );
    }
    if (nrefs == 2) {
        setUpdateRef(&fork.refs[1], ss);
        if (ss->kval < sval)
            swap(fork.refs[0], fork.refs[1]);
        if (++ss->kpos == ss->klen) {
            newUpdate<UpdateEndMark>(ss);
        } else {
            ss->kval = keyVal(ss->key, ss->kpos);
        }
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
        auto len = nodeHdrLen(ss->node);
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
    auto len = nodeHdrLen(ss->node);
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
    auto matched = forkBit(ss->node, ss->kval);
    seekNode(ss, inext);
    ss->kval = keyVal(ss->key, ++ss->kpos);
    if (matched) {
        return true;
    } else {
        ss->found = false;
        return false;
    }
}

//===========================================================================
// Returns true if there is more to do; otherwise false and sets ss->found
static bool insertAtEndMark(SearchState * ss) {
    if (ss->kpos == ss->klen) {
        ss->found = true;
    } else {
        auto & fork = newUpdate<UpdateFork>(ss);
        fork.endOfKey = true;
        setForkBit(&fork, ss->kval, true);
        auto ref = newRef(ss);
        fork.refs = {ref, 1};
        ss->found = false;
    }
    return false;
}

//===========================================================================
// Returns true if there is more to do; otherwise false and sets ss->found
static bool insertAtRemote(SearchState * ss) {
    auto pos = remotePos(ss->node);
    seekPage(ss, remotePage(ss->node));
    ss->spages.insert(ss->pgno);
    seekNode(ss, pos);
    return true;
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
            upd.refs = { newRef(ss), 1 };
        } else if (slen % 2 == 1) {
            upd.keyLen = slen & ~1;
            upd.refs = { newRef(ss), 1 };
        } else {
            upd.keyLen = slen;
            upd.endOfKey = true;
        }
        upd.key = ss->heap->alloc<uint8_t>(upd.keyLen / 2);
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

static void copyAny(SearchState * ss, const UpdateBase & upd);

//===========================================================================
static void copyAny(SearchState * ss, const NodeRef & ref) {
    if (ref.page.type == PageRef::kUpdate) {
        copyAny(ss, *ss->updates[ref.data.pos]);
    } else if (ref.page.type == PageRef::kSource) {
        auto node = getNode(ss, ref.page.pgno, ref.data.pos);
        assert(nodeLen(node) == ref.data.len);
        memcpy(ss->node, node, ref.data.len);
        seekNode(ss, ss->inode + ref.data.len);
    } else {
        assert(ref.page.type == PageRef::kVirtual);
        auto & vpage = ss->vpages[ref.page.pgno];
        setRemoteRef(ss->node, vpage.targetPgno, 0);
        seekNode(ss, ss->inode + nodeLen(ss->node));
    }
}

//===========================================================================
static void copyAny(SearchState * ss, const UpdateBase & upd) {
    if (upd.type == kNodeSeg) {
        auto & seg = static_cast<const UpdateSeg &>(upd);
        setSeg(ss->node, seg.endOfKey, seg.keyLen, seg.key);
    } else if (upd.type == kNodeHalfSeg) {
        auto & half = static_cast<const UpdateHalfSeg &>(upd);
        setHalfSeg(ss->node, half.endOfKey, half.nibble);
    } else if (upd.type == kNodeFork) {
        auto & fork = static_cast<const UpdateFork &>(upd);
        setFork(ss->node, fork.endOfKey, fork.kidBits);
    } else if (upd.type == kNodeEndMark) {
        [[maybe_unused]] auto & mark = static_cast<const UpdateEndMark &>(upd);
        setEndMark(ss->node);
    } else {
        assert(upd.type == kNodeRemote);
        auto & rref = static_cast<const UpdateRemote &>(upd);
        setRemoteRef(ss->node, rref.page.pgno, rref.pos);
    }
    seekNode(ss, ss->inode + nodeHdrLen(ss->node));
    for (auto&& ref : upd.refs)
        copyAny(ss, ref);
}

//===========================================================================
static void applyUpdates(SearchState * ss) {
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
    vector<UpdateInfo> infos(ss->updates.size());
    vector<int> unblocked;
    for (auto&& upd : ss->updates) {
        auto id = upd->id;
        auto & ui = infos[id];
        ui.upd = upd;
        bool branch = false;
        auto pos = -1;
        for (auto&& kid : ui.upd->refs) {
            pos += 1;
            if (kid.page.type == PageRef::kUpdate) {
                assert(kid.data.pos < infos.size());
                assert(infos[kid.data.pos].parent == -1);
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
    assert(!unblocked.empty());

    // Process updated nodes and related fringe nodes into virtual pages. Start
    // with leaf nodes, with branches becoming unblocked for processing when
    // all their leaves are done. Continues all the way up until the root node
    // is processed. Child virtual pages are merged up into their parents when
    // possible.
    for (;;) {
        auto id = unblocked.back();
        unblocked.pop_back();
        auto & ui = infos[id];
        auto & upd = *ui.upd;

        assert(upd.len == -1);
        for (auto&& kid : upd.refs) {
            if (kid.page.type == PageRef::kSource) {
                assert(kid.data.len > 0);
            } else {
                assert(kid.page.type == PageRef::kUpdate);
                assert(kid.data.len == -1);
                auto kupd = ss->updates[kid.data.pos];
                kid.data.len = nodeLen(*kupd);
            }
        }
        upd.len = nodeLen(upd);
        if (ui.unprocessed) {
            // internal branch
            assert(ui.unprocessed == 1);
            if (upd.len > ss->pages->pageSize()) {
                // Parent combined with kids is too big to fit on a single
                // page. Leave kids on their own pages, with any descendants
                // they've gathered, and start a new page with just the parent.
                for (auto&& ref : upd.refs) {
                    if (ref.page.type == PageRef::kSource
                        && ref.data.len <= kRemoteNodeLen
                    ) {
                        // If the entry on the kids page is no bigger than a
                        // remote link to another page, just copy it directly
                        // since putting it on a new page and linking to it
                        // won't save any space.
                        //
                        // But this isn't just a space optimization, it's real
                        // purpose is to prevent remote nodes pointing to
                        // remote nodes, potentially forever.
                        continue;
                    }
                    // Create new page with this kid and change the parents
                    // link to it to be a remote referrence.
                    auto & vpage = ss->vpages.emplace_back(ss->heap);
                    vpage.root = ref;
                    setRemoteRef(&ref, ss, (pgno_t) ss->vpages.size() - 1);
                }
                upd.len = nodeLen(upd);
            }
        }

        // Update reference from parent
        if (ui.parent == -1) {
            // Adding root node as a leaf, must be adding first key to this
            // previously empty container.
            assert(unblocked.empty());
            auto & vpage = ss->vpages.emplace_back(ss->heap);
            vpage.root.page.type = PageRef::kUpdate;
            vpage.root.data.pos = id;
            vpage.root.data.len = upd.len;
            break;
        }

        auto & pi = infos[ui.parent];
        assert(pi.unprocessed > 1);
        if (--pi.unprocessed == 1) {
            // Parent has had all children processed, queue for
            // processing.
            unblocked.push_back(ui.parent);
        }
    }

    // Allocate new pages and map them to virtual pages.
    for (auto&& vpage : ss->vpages) {
        allocPage(ss);
        vpage.targetPgno = ss->pgno;
    }
    // Copy nodes to new pages.
    for (auto&& vpage : ss->vpages) {
        seekPage(ss, vpage.targetPgno);
        seekNode(ss, 0);
        copyAny(ss, vpage.root);
    }
    ss->pages->setRoot(ss->vpages.back().targetPgno);

    // Destroy replaced source pages.
    for (auto&& id : ss->spages)
        ss->pages->destroy(id);

    if (ss->debugStream) {
        *ss->debugStream << "Pages freed (" << ss->spages.size() << "): "
            << ss->spages << '\n';
        for (auto&& vpage : ranges::reverse_view(ss->vpages)) {
            seekPage(ss, vpage.targetPgno);
            seekNode(ss, 0);
            dumpPage(*ss->debugStream, ss);
        }
    }
}

//===========================================================================
// Returns true if key was inserted, false if it was already present.
bool StrTrieBase::insert(string_view key) {
    TempHeap heap;
    auto ss = heap.emplace<SearchState>(key, m_pages, &heap, debugStream());
    if (!empty()) {
        seekRootPage(ss);
        ss->spages.insert(ss->pgno);
        seekNode(ss, ss->inode);

        using Fn = bool(SearchState *);
        constinit static Fn * const functs[8] = {
            logFatal,
            insertAtSeg,
            insertAtHalfSeg,
            insertAtFork,
            insertAtEndMark,
            insertAtRemote,
            logFatal,
            logFatal,
        };
        for (;;) {
            auto ntype = ::nodeType(ss->node);
            auto fn = functs[ntype];
            if (!(*fn)(ss)) {
                if (ss->found)
                    return false;
                break;
            }
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
    if (ss->kpos == ss->klen) {
        ss->found = nodeEndMarkFlag(ss->node);
        return false;
    }
    seekNode(ss, ss->inode + nodeHdrLen(ss->node));
    return !nodeEndMarkFlag(ss->node);
}

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
            ss->forks.emplace_back(ss->pgno, subs[i].first, ss->kpos);
            break;
        }
    }
    ss->kval = keyVal(ss->key, ++ss->kpos);
    auto sub = subs.back();
    seekNode(ss, sub.first + sub.second);
    return true;
}

//===========================================================================
static void findLastFork(SearchState * ss) {
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
            logMsgFatal() << "Invalid StrTrieBase node type: "
                << (int) ntype;
        }
    }
}

//===========================================================================
bool StrTrieBase::erase(string_view key) {
    if (empty())
        return false;

    TempHeap heap;
    auto ss = heap.emplace<SearchState>(key, m_pages, &heap, debugStream());
    seekRootPage(ss);

    findLastFork(ss);
    if (!ss->found)
        return false;

    assert(!ss->forks.empty());
    auto & fork = ss->forks.back();
    seekPage(ss, fork.pgno);
    seekNode(ss, fork.inode);
    ss->kpos = fork.kpos;

    return true;
}


/****************************************************************************
*
*   Contains
*
***/

//===========================================================================
static bool containsAtSeg(SearchState * ss) {
    auto spos = 0;
    auto slen = segLen(ss->node);
    auto sval = segVal(ss->node, spos);

    if (ss->klen - ss->kpos < slen)
        return false;
    for (;;) {
        if (ss->kval != sval)
            return false;
        if (++ss->kpos != ss->klen)
            ss->kval = keyVal(ss->key, ss->kpos);
        if (++spos == slen) {
            if (nodeEndMarkFlag(ss->node)) {
                if (ss->kpos == ss->klen) {
                    ss->found = true;
                    return false;
                }
                return false;
            }
            seekNode(ss, ss->inode + nodeHdrLen(ss->node));
            return true;
        }
        sval = segVal(ss->node, spos);
    }
}

//===========================================================================
static bool containsAtHalfSeg(SearchState * ss) {
    if (ss->kpos == ss->klen)
        return false;
    auto sval = halfSegVal(ss->node);
    if (ss->kval != sval)
        return false;
    if (++ss->kpos == ss->klen) {
        if (nodeEndMarkFlag(ss->node))
            ss->found = true;
        return false;
    }
    if (nodeEndMarkFlag(ss->node))
        return false;
    ss->kval = keyVal(ss->key, ss->kpos);
    seekNode(ss, ss->inode + nodeHdrLen(ss->node));
    return true;
}

//===========================================================================
static bool containsAtFork(SearchState * ss) {
    if (ss->kpos == ss->klen) {
        ss->found = nodeEndMarkFlag(ss->node);
        return false;
    }
    if (!forkBit(ss->node, ss->kval))
        return false;

    auto pos = forkPos(ss->node, ss->kval);
    seekKid(ss, pos);
    if (++ss->kpos < ss->klen)
        ss->kval = keyVal(ss->key, ss->kpos);
    return true;
}

//===========================================================================
static bool containsAtEndMark(SearchState * ss) {
    if (ss->kpos == ss->klen)
        ss->found = true;
    return false;
}

//===========================================================================
static bool containsAtRemote(SearchState * ss) {
    auto pos = remotePos(ss->node);
    seekPage(ss, remotePage(ss->node));
    seekNode(ss, pos);
    return true;
}

//===========================================================================
bool StrTrieBase::contains(string_view key) const {
    if (empty())
        return false;

    TempHeap heap;
    auto ss = heap.emplace<SearchState>(key, m_pages, &heap, debugStream());
    seekRootPage(ss);
    seekNode(ss, ss->inode);

    using Fn = bool(SearchState *);
    constinit static Fn * const functs[8] = {
        logFatal,
        containsAtSeg,
        containsAtHalfSeg,
        containsAtFork,
        containsAtEndMark,
        containsAtRemote,
        logFatal,
        logFatal,
    };
    for (;;) {
        auto ntype = ::nodeType(ss->node);
        auto fn = functs[ntype];
        if (!(*fn)(ss))
            return ss->found;
    }
}


/****************************************************************************
*
*   lowerBound
*
***/

//===========================================================================
static void setFoundKey(SearchState * ss) {
    ss->foundKey = ss->key.substr(0, (ss->kpos + 1) / 2);
    ss->foundKeyLen = ss->kpos;
}

//===========================================================================
static void pushFoundKeyVal(SearchState * ss, int val) {
    assert(val >= 0 && val <= 15);
    if (ss->foundKeyLen % 2 == 0) {
        ss->foundKey.push_back((uint8_t) val << 4);
    } else {
        ss->foundKey.back() |= val;
    }
    ss->foundKeyLen += 1;
}

//===========================================================================
static void seekNextFork(SearchState * ss) {
    while (!ss->forks.empty()) {
        auto & fork = ss->forks.back();
        ss->kpos = fork.kpos;
        ss->kval = keyVal(ss->key, ss->kpos);
        seekPage(ss, fork.pgno);
        seekNode(ss, fork.inode);
        auto nval = nextForkVal(forkBits(ss->node), ss->kval);
        if (nval == -1) {
            ss->forks.pop_back();
            continue;
        }

        // Found next fork
        auto pos = forkPos(ss->node, nval);
        fork.kpos = pos;
        seekKid(ss, pos);
        setFoundKey(ss);
        pushFoundKeyVal(ss, nval);
        return;
    }

    ss->inode = 0;
    ss->node = nullptr;
}

//===========================================================================
static bool lowerBoundAtSeg(SearchState * ss) {
    auto spos = 0;
    auto slen = segLen(ss->node);
    auto sval = segVal(ss->node, spos);

    for (;;) {
        if (ss->kval != sval)
            break;
        if (++ss->kpos != ss->klen)
            ss->kval = keyVal(ss->key, ss->kpos);
        if (++spos == slen) {
            if (nodeEndMarkFlag(ss->node)) {
                seekNextFork(ss);
                return false;
            }
        }
        sval = segVal(ss->node, spos);
    }

    if (ss->kval < sval) {
        setFoundKey(ss);
        for (;;) {
            pushFoundKeyVal(ss, sval);
            if (++spos == slen)
                break;
            sval = segVal(ss->node, spos);
        }
        if (nodeEndMarkFlag(ss->node)) {
            ss->inode = 0;
            ss->node = nullptr;
            return false;
        } else {
            seekNode(ss, ss->inode + nodeHdrLen(ss->node));
            return false;
        }
    } else {
        assert(ss->kval > sval);
        seekNextFork(ss);
        return false;
    }
}

//===========================================================================
static bool lowerBoundAtHalfSeg(SearchState * ss) {
    return false;
}

//===========================================================================
static bool lowerBoundAtFork(SearchState * ss) {
    return false;
}

//===========================================================================
static bool lowerBoundAtEndMark(SearchState * ss) {
    return false;
}

//===========================================================================
static bool lowerBoundAtRemote(SearchState * ss) {
    return false;
}

//===========================================================================
StrTrieBase::Iter StrTrieBase::lowerBound(std::string_view key) const {
    if (empty())
        return Iter{};

    TempHeap heap;
    auto ss = heap.emplace<SearchState>(key, m_pages, &heap, debugStream());
    seekRootPage(ss);
    seekNode(ss, ss->inode);

    using Fn = bool(SearchState *);
    constinit static Fn * const functs[8] = {
        logFatal,
        lowerBoundAtSeg,
        lowerBoundAtHalfSeg,
        lowerBoundAtFork,
        lowerBoundAtEndMark,
        lowerBoundAtRemote,
        logFatal,
        logFatal,
    };
    for (;;) {
        auto ntype = ::nodeType(ss->node);
        auto fn = functs[ntype];
        if (!(*fn)(ss)) {
            if (!ss->found)
                return Iter{};
            break;
        }
    }

    if (ss->node) {
    }

    return Iter{};
}


/****************************************************************************
*
*   Misc
*
***/

//===========================================================================
StrTrieBase::Iter StrTrieBase::begin() const {
    return Iter{};
}

//===========================================================================
StrTrieBase::Iter StrTrieBase::end() const {
    return Iter{};
}

//===========================================================================
static ostream & dumpAny(ostream & os, SearchState * ss) {
    os << setw(5) << ss->inode << ":  ";
    auto len = nodeHdrLen(ss->node);
    for (auto i = 0; i < len; ++i) {
        if (i % 2 == 1) os.put(' ');
        if (auto val = ss->node->data[i]) {
            hexByte(os, ss->node->data[i]);
        } else {
            os << "--";
        }
    }
    os << ' ';
    switch (auto ntype = nodeType(ss->node)) {
    case kNodeSeg:
        os << " Seg[" << segLen(ss->node) << "]";
        if (nodeEndMarkFlag(ss->node))
            os << ", EOK";
        break;
    case kNodeHalfSeg:
        os << " Half Seg, " << (int) halfSegVal(ss->node);
        if (nodeEndMarkFlag(ss->node))
            os << ", EOK";
        break;
    case kNodeFork:
        os << " Fork,";
        if (nodeEndMarkFlag(ss->node))
            os << " EOK";
        for (auto&& pos : forkVals(ss, ss->node))
            os << ' ' << (int) pos;
        break;
    case kNodeEndMark:
        os << " EOK";
        break;
    case kNodeRemote:
        os << " Remote, " << remotePage(ss->node) << '.'
            << remotePos(ss->node);
        break;
    default:
        os << " UNKNOWN(" << ntype << ")";
        break;
    }
    auto subs = kids(ss, ss->inode);
    if (!subs.empty()) {
        ostringstream otmp;
        for (auto & kid : subs)
            otmp << kid.first << ' ';
        if (otmp.view().size() <= 28) {
            os << "  // " << otmp.view();
        } else {
            string prefix = "          // ";
            os << '\n' << prefix;
            Cli cli;
            string body(size(prefix), '\v');
            body += otmp.view();
            cli.printText(os, body);
        }
    }
    os << '\n';
    if (subs.empty()) {
        seekNode(ss, ss->inode + len);
    } else {
        for (auto & kid : subs) {
            seekNode(ss, kid.first);
            dumpAny(os, ss);
        }
        seekNode(ss, subs.back().first + subs.back().second);
    }
    return os;
}

//===========================================================================
static ostream & dumpPage(ostream & os, SearchState * ss) {
    os << "Page " << ss->pgno << '.' << ss->inode << ":\n";
    dumpAny(os, ss);
    os << setw(5) << ss->inode << ":\n";
    return os;
}
