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
// Multiroot
//  data[0] & 0x7 = kNodeMultiroot
//  data[0] & 0xf0 = number of roots (1 - 16)
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
//  data[0] & 0xf0 = index of node (0 - 15)
//  data(1, 4) = pgno
// EndMark
//  data[0] & 0x7 = kNodeEndMark
//  data[0] & 0x8 = true (end of key - no content)

const size_t kMaxForkBit = 16;
const size_t kMaxSegLen = 32;

const size_t kRemoteNodeLen = 5;
const size_t kForkNodeHdrLen = 3;

const size_t kMinPageSize = kForkNodeHdrLen + kMaxForkBit * kRemoteNodeLen;
const size_t kMaxPageSize = 65'536;

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
    int pos;
    int len;

    auto operator<=>(const LocalRef & other) const = default;
};
struct NodeRef {
    PageRef page;
    LocalRef data = { -1, -1 };

    auto operator<=>(const NodeRef & other) const = default;
};

enum NodeType : int8_t {
    kNodeInvalid,
    kNodeMultiroot,
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
struct UpdateMultiroot : UpdateBase {
    constexpr static NodeType s_type = kNodeMultiroot;
    int roots = 0;
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

struct VirtualPage {
    pgno_t targetPgno = (pgno_t) -1;
    pmr::vector<NodeRef> roots;

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
static void setMultiroot(StrTrieBase::Node * node, size_t roots) {
    assert(roots >= 1 && roots <= 16);
    node->data[0] = kNodeMultiroot | (int8_t(roots - 1) << 4);
}

//===========================================================================
inline static int multirootLen(const StrTrieBase::Node * node) {
    assert(nodeType(node) == kNodeMultiroot);
    return (node->data[0] >> 4) + 1;
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
    assert(kval && kval < kMaxForkBit);
    auto mask = (1u << (kMaxForkBit - kval - 1)) - 1;
    auto nbits = bits & mask;
    if (!nbits)
        return -1;
    auto nval = leadingZeroBits(nbits);
    return nval;
}

//===========================================================================
inline static int prevForkVal(uint16_t bits, int kval) {
    assert(kval && kval < kMaxForkBit);
    auto mask = 65535u << (kMaxForkBit - kval);
    auto nbits = bits & mask;
    if (!nbits)
        return -1;
    auto nval = kMaxForkBit - trailingZeroBits(nbits);
    return (int) nval;
}

//===========================================================================
static pmr::basic_string<uint8_t> forkVals(
    SearchState * ss,
    const StrTrieBase::Node * node,
    int kval = -1
) {
    assert(nodeType(node) == kNodeFork);
    pmr::basic_string<uint8_t> out(ss->heap);
    auto bits = forkBits(node);
    if (kval > -1)
        bits |= (0x8000 >> kval);
    out.reserve(hammingWeight(bits));
    for (uint8_t i = 0; bits; ++i, bits <<= 1) {
        if (bits & 0x8000)
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
    return (int) node->data[0] >> 4;
}

//===========================================================================
static void setRemoteRef(StrTrieBase::Node * node, pgno_t pgno, size_t pos) {
    assert(pos < 16);
    node->data[0] = kNodeRemote | (int8_t(pos) << 4);
    hton32(node->data + 1, pgno);
}

//===========================================================================
static int nodeHdrLen(const StrTrieBase::Node * node) {
    switch (auto type = nodeType(node)) {
    default:
        assert(type == kNodeInvalid);
        assert(!"Invalid node type");
        return 0;
    case kNodeMultiroot:
        return 1;
    case kNodeSeg:
        return 1 + segLen(node) / 2;
    case kNodeHalfSeg:
        return 1;
    case kNodeFork:
        return kForkNodeHdrLen;
    case kNodeEndMark:
        return 1;
    case kNodeRemote:
        return kRemoteNodeLen;
    }
}

//===========================================================================
static size_t numKids(const StrTrieBase::Node * node) {
    switch (auto type = nodeType(node)) {
    default:
        assert(type == kNodeInvalid);
        assert(!"Invalid node type");
        return 0;
    case kNodeMultiroot:
        return multirootLen(node);
    case kNodeSeg:
    case kNodeHalfSeg:
        return nodeEndMarkFlag(node) ? 0 : 1;
    case kNodeFork:
        return forkLen(node);
    case kNodeEndMark:
        return 0;
    case kNodeRemote:
        return 0;
    }
}

//===========================================================================
static int nodeLen(const StrTrieBase::Node * node) {
    auto len = nodeHdrLen(node);
    auto num = numKids(node);
    for (auto i = 0; i < num; ++i)
        len += nodeLen(node + len);
    assert(len > 0 && len < INT_MAX);
    return len;
}

//===========================================================================
// Returns vector of inode and nodeLen pairs.
static pmr::basic_string<LocalRef> kids(
    SearchState * ss,
    int inode
) {
    assert(inode < ss->pages->pageSize());
    pmr::basic_string<LocalRef> out(ss->heap);
    auto root = ss->node - ss->inode + inode;
    auto len = nodeHdrLen(root);
    auto node = root + len;
    inode += len;
    if (auto num = numKids(root)) {
        out.reserve(num);
        for (auto i = 0; i < num; ++i) {
            auto len = nodeLen(node);
            out.push_back({inode, len});
            inode += len;
            node += len;
        }
    }
    return out;
}

//===========================================================================
// NodeRef helpers
//===========================================================================
static void setUpdateRef(NodeRef * out, SearchState * ss) {
    out->page = { .type = PageRef::kUpdate };
    out->data = { .pos = (int) ss->updates.size(), .len = -1 };
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
static void setRemoteRef(
    NodeRef * out,
    SearchState * ss,
    pgno_t pgno,
    int pos
) {
    out->page = { .type = PageRef::kVirtual, .pgno = pgno };
    out->data = { .pos = pos, .len = kRemoteNodeLen };
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
    setRemoteRef(ref, ss, pgno, 0);
    return ref;
}

//===========================================================================
// UpdateNode helpers
//===========================================================================
static int nodeHdrLen(const UpdateBase & upd) {
    switch(upd.type) {
    default:
        assert(upd.type == kNodeInvalid);
        assert(!"Invalid node type");
        return 0;
    case kNodeMultiroot:
        return 1;
    case kNodeSeg:
        return 1 + static_cast<const UpdateSeg &>(upd).keyLen / 2;
    case kNodeHalfSeg:
        return 1;
    case kNodeFork:
        return kForkNodeHdrLen;
    case kNodeEndMark:
        return 1;
    case kNodeRemote:
        return kRemoteNodeLen;
    }
}

//===========================================================================
static int nodeLen(const UpdateBase & upd) {
    int len = nodeHdrLen(upd);
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
static void seekRemote(SearchState* ss, size_t inode) {
    auto node = getNode(ss, inode);
    auto pos = remotePos(node);
    seekPage(ss, remotePage(node));
    seekNode(ss, 0);
    if (::nodeType(ss->node) != kNodeMultiroot) {
        assert(pos == 0);
    }
    else {
        auto roots = kids(ss, 0);
        assert(pos < roots.size());
        seekNode(ss, roots[pos].pos);
    }
}

//===========================================================================
static bool logFatal(SearchState * ss) {
    logMsgFatal() << "Invalid StrTrieBase node type: "
        << nodeType(ss->node);
    return false;
}


/****************************************************************************
*
*   VirtualPage
*
***/

//===========================================================================
VirtualPage::VirtualPage(TempHeap * heap)
    : roots(heap)
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
*   StrTrieBase::Iter
*
***/

//===========================================================================
StrTrieBase::Iter & StrTrieBase::Iter::operator++() {
    return *this;
}

//===========================================================================
StrTrieBase::Iter StrTrieBase::begin() const {
    return Iter{};
}

//===========================================================================
StrTrieBase::Iter StrTrieBase::end() const {
    return Iter{};
}


/****************************************************************************
*
*   StrTrieBase
*
***/

//===========================================================================
StrTrieBase::StrTrieBase(IPageHeap * pages)
    : m_pages(pages)
{
    assert(pages->pageSize() >= kMinPageSize);
    assert(pages->pageSize() <= kMaxPageSize);
}

//===========================================================================
void StrTrieBase::construct(IPageHeap * pages) {
    assert(pages->pageSize() >= kMinPageSize);
    assert(pages->pageSize() <= kMaxPageSize);
    assert(!m_pages);
    m_pages = pages;
}


/****************************************************************************
*
*   Insert
*
***/

//===========================================================================
static NodeRef * addHalfSeg(SearchState * ss, uint8_t val, bool eok) {
    auto & upd = newUpdate<UpdateHalfSeg>(ss);
    upd.nibble = val;
    if (eok) {
        upd.endOfKey = true;
        return nullptr;
    }
    auto ref = newRef(ss);
    upd.refs = {ref, 1};
    return &upd.refs[0];
}

//===========================================================================
static void addKeyHalfSeg(SearchState * ss) {
    if (++ss->kpos == ss->klen) {
        addHalfSeg(ss, ss->kval, true);
    } else {
        addHalfSeg(ss, ss->kval, false);
        ss->kval = keyVal(ss->key, ss->kpos);
    }
}

//===========================================================================
static NodeRef * addForkWithEnd(
    SearchState * ss, 
    size_t pos, 
    bool withNewRef
) {
    auto & fork = newUpdate<UpdateFork>(ss);
    fork.endOfKey = true;
    setForkBit(&fork, pos, true);
    NodeRef * ref = nullptr;
    if (withNewRef) {
        ref = newRef(ss);
    } else {
        ref = ss->heap->emplace<NodeRef>();
    }
    fork.refs = {ref, 1};
    return &fork.refs[0];
}

//===========================================================================
static [[nodiscard]] pair<NodeRef *, NodeRef *> addFork(
    SearchState * ss, 
    size_t sval,
    size_t kval
) {
    assert(sval != kval);
    auto & fork = newUpdate<UpdateFork>(ss);
    setForkBit(&fork, sval, true);
    setForkBit(&fork, ss->kval, true);
    fork.refs = ss->heap->allocSpan<NodeRef>(2);
    if (sval < kval) {
        return { &fork.refs[0], &fork.refs[1] };
    } else {
        return { &fork.refs[1], &fork.refs[0] };
    }
}

//===========================================================================
static void copySegPrefix(SearchState * ss, size_t pos) {
    assert(nodeType(ss->node) == kNodeSeg);
    assert(pos <= segLen(ss->node));
    assert(pos % 2 == 0);
    // TODO: merge with preceeding NodeSeg if present.
    if (!pos) 
        return;
    auto & upd = newUpdate<UpdateSeg>(ss);
    upd.keyLen = (int) pos;
    auto len8 = upd.keyLen / 2;
    upd.key = ss->heap->alloc<uint8_t>(len8);
    auto data = segData(ss->node);
    memcpy(upd.key, data, len8);
    upd.refs = { newRef(ss), 1 };
}

//===========================================================================
static void copyRef(SearchState * ss, NodeRef * ref) {
    if (nodeEndMarkFlag(ss->node)) {
        setUpdateRef(ref, ss);
        newUpdate<UpdateEndMark>(ss);
    } else {
        auto hdrLen = nodeHdrLen(ss->node);
        setSourceRef(
            ref, 
            ss, 
            ss->inode + hdrLen, 
            nodeLen(ss->node + hdrLen)
        );
    }    
}

//===========================================================================
static void copyKeyRef(SearchState * ss, NodeRef * ref) {
    setUpdateRef(ref, ss);
    if (++ss->kpos == ss->klen) {
        newUpdate<UpdateEndMark>(ss);
    } else {
        ss->kval = keyVal(ss->key, ss->kpos);
    }    
}

//===========================================================================
static void copySegSuffix(SearchState * ss, NodeRef * ref, size_t pos) {
    assert(nodeType(ss->node) == kNodeSeg);
    assert(pos <= segLen(ss->node));
    assert(pos % 2 == 0);
    auto len = segLen(ss->node) - pos;
    if (!len) {
        copyRef(ss, ref);
        return;
    }

    setUpdateRef(ref, ss);
    auto & upd = newUpdate<UpdateSeg>(ss);
    upd.keyLen = (int) len;
    auto len8 = upd.keyLen / 2;
    upd.key = ss->heap->alloc<uint8_t>(len8);
    auto data = segData(ss->node);
    memcpy(upd.key, data + pos / 2, len8);
    if (nodeEndMarkFlag(ss->node)) {
        upd.endOfKey = true;
        return;
    } 
    // TODO: merge with following NodeSeg if present.
    auto hdrLen = nodeHdrLen(ss->node);
    auto srcRef = newRef(ss, ss->inode + hdrLen, nodeLen(ss->node + hdrLen));
    upd.refs = { srcRef, 1 };
}

//===========================================================================
static void copyHalfSegSuffix(SearchState * ss, size_t spos) {
    assert(nodeType(ss->node) == kNodeSeg);
    auto sval = segVal(ss->node, ++spos);
    auto slen = segLen(ss->node);
    if (spos + 1 == slen && nodeEndMarkFlag(ss->node)) {
        addHalfSeg(ss, sval, true);
    } else {
        auto ref = addHalfSeg(ss, sval, false);
        copySegSuffix(ss, ref, spos + 1);
    }
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

    assert(ss->kpos % 2 == 0);
    auto spos = 0; // split point where key diverges from segment
    auto slen = segLen(ss->node);
    auto sval = segVal(ss->node, spos);
    assert(slen > 0);

    for (;;) {
        if (ss->kpos == ss->klen) {
            // Fork end of key with position in the segment.
            copySegPrefix(ss, spos);
            addForkWithEnd(ss, sval, true);
            copyHalfSegSuffix(ss, spos);
            break;
        }
        if (ss->kval != sval) {
            // Fork inside the key and the segment.
            if (spos % 2 == 0) {
                // Fork in the first half of the byte.
                copySegPrefix(ss, spos);
                auto [sref, kref] = addFork(ss, sval, ss->kval);
                setUpdateRef(sref, ss);
                copyHalfSegSuffix(ss, spos);
                setUpdateRef(kref, ss);
                ss->kval = keyVal(ss->key, ++ss->kpos);
                addKeyHalfSeg(ss);
                break;
            } else {
                // Fork in the second half of the byte.
                copySegPrefix(ss, spos - 1);
                addHalfSeg(ss, segVal(ss->node, spos - 1), false);
                auto [sref, kref] = addFork(ss, sval, ss->kval);
                copySegSuffix(ss, sref, spos + 1);
                copyKeyRef(ss, kref);
                break;
            }
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
                // Fork key with the end mark that's after the segment.
                copySegPrefix(ss, spos);
                addForkWithEnd(ss, ss->kval, true);
                ss->kval = keyVal(ss->key, ++ss->kpos);
                addKeyHalfSeg(ss);
                break;
            }
            // Continue search at next node
            copySegPrefix(ss, spos);
            seekNode(ss, ss->inode + nodeHdrLen(ss->node));
            return true;
        }
        sval = segVal(ss->node, spos);
    }

    // Continue processing the key.
    assert(ss->kpos % 2 == 0);
    ss->found = false;
    return false;
}

//===========================================================================
static UpdateHalfSeg & copyHalfSeg(SearchState * ss, bool eok) {
    assert(nodeType(ss->node) == kNodeHalfSeg);
    auto & upd = newUpdate<UpdateHalfSeg>(ss);
    upd.nibble = halfSegVal(ss->node);
    if (eok) {
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
    // Will either split on value, after value (possible if it's the end of the
    // key), or will advance to next node.

    auto sval = halfSegVal(ss->node);

    for (;;) {
        if (ss->kpos == ss->klen) {
            // Fork end of key with half seg.
            auto ref = addForkWithEnd(ss, sval, false);
            copyRef(ss, ref);
            break;
        }
        if (ss->kval != sval) {
            // Fork inside the key with the half seg.
            auto [sref, kref] = addFork(ss, sval, ss->kval);
            copyRef(ss, sref);
            copyKeyRef(ss, kref);
            break;
        }
        if (++ss->kpos != ss->klen)
            ss->kval = keyVal(ss->key, ss->kpos);
        if (nodeEndMarkFlag(ss->node)) {
            if (ss->kpos == ss->klen) {
                // End of both key and half seg? Key was found, abort.
                ss->found = true;
                return false;
            }
            // Fork key with the end mark that's after the segment.
            copyHalfSeg(ss, false);
            addForkWithEnd(ss, ss->kval, true);
            ss->kval = keyVal(ss->key, ++ss->kpos);
            break;
        }
        // Continue search at next node
        copyHalfSeg(ss, false);
        seekNode(ss, ss->inode + nodeHdrLen(ss->node));
        return true;
    }

    // Continue processing the key.
    ss->found = false;
    return false;
}

//===========================================================================
static UpdateFork & copyFork(SearchState * ss, int * inext) {
    assert(nodeType(ss->node) == kNodeFork);
    *inext = -1;

    auto & upd = newUpdate<UpdateFork>(ss);
    auto bits = forkBits(ss->node);
    auto refs = kids(ss, ss->inode);

    if (ss->kpos == ss->klen) {
        upd.refs = ss->heap->allocSpan<NodeRef>(refs.size());
        upd.endOfKey = true;
        upd.kidBits = bits;
        for (auto i = 0; i < refs.size(); ++i)
            setSourceRef(&upd.refs[i], ss, refs[i].pos, refs[i].len);
        return upd;
    }

    upd.endOfKey = nodeEndMarkFlag(ss->node);
    upd.kidBits = setForkBit(bits, ss->kval, true);
    auto fpos = forkPos(upd.kidBits, ss->kval);
    if (bits != upd.kidBits) {
        upd.refs = ss->heap->allocSpan<NodeRef>(refs.size() + 1);
        for (auto i = 0; i < fpos; ++i)
            setSourceRef(&upd.refs[i], ss, refs[i].pos, refs[i].len);
        setUpdateRef(&upd.refs[fpos], ss);
        for (auto i = fpos; i < refs.size(); ++i)
            setSourceRef(&upd.refs[i + 1], ss, refs[i].pos, refs[i].len);
        return upd;
    }

    *inext = refs[fpos].pos;
    upd.refs = ss->heap->allocSpan<NodeRef>(refs.size());

    auto fnode = getNode(ss, refs[fpos].pos);
    auto rpno = (pgno_t) -1;
    pmr::basic_string<LocalRef> rrefs;
    if (nodeType(fnode) == kNodeRemote) {
        rpno = remotePage(fnode);
        fnode = getNode(ss, rpno, 0);
        if (nodeType(fnode) == kNodeMultiroot) {
            auto pgno = ss->pgno;
            auto inode = ss->inode;
            seekPage(ss, rpno);
            seekNode(ss, 0);
            rrefs = kids(ss, 0);
            seekPage(ss, pgno);
            seekNode(ss, inode);
        }
    }
    for (auto i = 0; i < refs.size(); ++i) {
        if (i == fpos) {
            setUpdateRef(&upd.refs[i], ss);
            continue;
        }
        if (rpno == -1) {
            setSourceRef(&upd.refs[i], ss, refs[i].pos, refs[i].len);
            continue;
        }
        fnode = getNode(ss, refs[i].pos);
        if (nodeType(fnode) == kNodeRemote && rpno == remotePage(fnode)) {
            assert(!rrefs.empty());
            auto & rref = rrefs[remotePos(fnode)];
            setSourceRef(&upd.refs[i], ss, rref.pos, rref.len);
            upd.refs[i].page.pgno = rpno;
        } else {
            setSourceRef(&upd.refs[i], ss, refs[i].pos, refs[i].len);
        }
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
    bool matched = forkBit(ss->node, ss->kval);
    if (++ss->kpos != ss->klen)
        ss->kval = keyVal(ss->key, ss->kpos);
    if (matched) {
        seekNode(ss, inext);
        return true;
    } else {
        if (ss->kpos == ss->klen)
            newUpdate<UpdateEndMark>(ss);
        ss->found = false;
        return false;
    }
}

//===========================================================================
// Returns true if there is more to do; otherwise false and sets ss->found
static bool insertAtEndMark(SearchState * ss) {
    if (ss->kpos == ss->klen) {
        ss->found = true;
        return false;
    }

    auto & fork = newUpdate<UpdateFork>(ss);
    fork.endOfKey = true;
    setForkBit(&fork, ss->kval, true);
    if (++ss->kpos != ss->klen)
        ss->kval = keyVal(ss->key, ss->kpos);
    auto ref = newRef(ss);
    fork.refs = {ref, 1};
    ss->found = false;
    return false;
}

//===========================================================================
// Returns true if there is more to do; otherwise false and sets ss->found
static bool insertAtRemote(SearchState * ss) {
    seekRemote(ss, ss->inode);
    ss->spages.insert(ss->pgno);
    return true;
}

//===========================================================================
static void addSegs(SearchState * ss) {
    assert(ss->kpos <= ss->klen);
    if (ss->kpos % 2 == 1) 
        addKeyHalfSeg(ss);
    while (int slen = ss->klen - ss->kpos) {
        assert(slen > 1);
        auto & upd = newUpdate<UpdateSeg>(ss);
        if (slen > kMaxSegLen) {
            upd.keyLen = kMaxSegLen;
            upd.refs = { newRef(ss), 1 };
        } else {
            upd.keyLen = slen;
            upd.endOfKey = true;
        }
        upd.key = ss->heap->alloc<uint8_t>(upd.keyLen / 2);
        memcpy(upd.key, ss->key.data() + ss->kpos / 2, upd.keyLen / 2);
        ss->kpos += upd.keyLen;
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
        setRemoteRef(ss->node, vpage.targetPgno, ref.data.pos);
        seekNode(ss, ss->inode + nodeLen(ss->node));
    }
}

//===========================================================================
static void copyAny(SearchState * ss, const UpdateBase & upd) {
    if (upd.type == kNodeMultiroot) {
        auto & mroot = static_cast<const UpdateMultiroot &>(upd);
        setMultiroot(ss->node, mroot.roots);
    } else if (upd.type == kNodeSeg) {
        auto & seg = static_cast<const UpdateSeg &>(upd);
        setSeg(ss->node, seg.endOfKey, seg.keyLen, seg.key);
    } else if (upd.type == kNodeHalfSeg) {
        auto & half = static_cast<const UpdateHalfSeg &>(upd);
        setHalfSeg(ss->node, half.endOfKey, half.nibble);
    } else if (upd.type == kNodeFork) {
        auto & fork = static_cast<const UpdateFork &>(upd);
        setFork(ss->node, fork.endOfKey, fork.kidBits);
    } else {
        assert(upd.type == kNodeEndMark);
        [[maybe_unused]] auto & mark = static_cast<const UpdateEndMark &>(upd);
        setEndMark(ss->node);
    }
    seekNode(ss, ss->inode + nodeHdrLen(ss->node));
    for (auto&& ref : upd.refs)
        copyAny(ss, ref);
}

//===========================================================================
// If the update node is too large to fit on a page with all it's descendants,
// allocate virtual pages for some or all of the descendant trees.
static void harvestPages(SearchState * ss, UpdateBase * upd) {
    upd->len = nodeLen(*upd);
    auto psize = ss->pages->pageSize();
    if (upd->len <= psize)
        return;

    // Parent combined with kids is too big to fit on a single page. Leave kids
    // on their own pages, with any descendants they've gathered, and start a
    // new page with just the parent.
    auto count = 0;
    NodeRef * refs[kMaxForkBit];
    int nrefs = 0;
    for (auto&& ref : upd->refs) {
        // If the data on the kids page is no bigger than a remote link to
        // another page, just copy it directly since putting it on a new page
        // and linking to it won't save any space.
        //
        // But the real purpose of this optimization is to prevent moving a
        // remote reference to it's own page, making remote nodes pointing to
        // remote nodes, potentially forever.
        if (ref.data.len > kRemoteNodeLen) {
            refs[nrefs++] = &ref;
            count += ref.data.len;
        }
    }
    sort(refs, refs + nrefs, [](auto& a, auto& b) {
        return a->data.len > b->data.len;
    });

    int vmap[kMaxForkBit];
    struct VPInfo {
        pgno_t pgno;
        int len;
        int pos;
    };
    VPInfo vinfoData[3] = {};
    auto vinfos = span<VPInfo>(vinfoData, 0);

ADD_VPAGE:
    vinfos = span<VPInfo>(vinfoData, vinfos.size() + 1);
    auto & nvi = vinfos.back();
    assert(vinfos.size() <= 3);
    nvi.pgno = (pgno_t) ss->vpages.size();
    nvi.pos = (int) vinfos.size() - 1;
    ss->vpages.emplace_back(ss->heap);
    for (auto&& vi : vinfos)
        vi.len = 0;
    for (auto i = 0; i < nrefs; ++i) {
        vmap[i] = vinfos[0].pos;
        bool multi = vinfos[0].len;
        vinfos[0].len += refs[i]->data.len;
        if (vinfos[0].len > psize - multi)
            goto ADD_VPAGE;
        for (auto vpos = 1; vpos < vinfos.size(); ++vpos) {
            if (vinfos[vpos - 1].len > vinfos[vpos].len) {
                swap(vinfos[vpos - 1], vinfos[vpos]);
            } else {
                break;
            }
        }
    }
    for (auto&& vi : vinfos) {
        assert(vi.len);
        vi.pos = 0;
    }

    for (auto i = 0; i < nrefs; ++i) {
        // Add kid to most empty new page and change the parents link to it
        // to be a remote reference.
        auto & vi = vinfos[vmap[i]];
        ss->vpages[vi.pgno].roots.push_back(*refs[i]);
        setRemoteRef(refs[i], ss, vi.pgno, vi.pos++);
    }
    for ([[maybe_unused]] auto & vi : vinfos) 
        assert(vi.pos);
    upd->len = nodeLen(*upd);
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
        assert(ui.unprocessed <= 1);
        harvestPages(ss, &upd);

        // Update reference from parent
        if (ui.parent == -1) {
            // Adding root node as a leaf, must be adding first key to this
            // previously empty container.
            assert(unblocked.empty());
            auto & vpage = ss->vpages.emplace_back(ss->heap);
            vpage.roots.push_back({
                .page = { .type = PageRef::kUpdate },
                .data = { .pos = id, .len = upd.len },
            });
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
        if (auto roots = vpage.roots.size(); roots == 1) {
            copyAny(ss, vpage.roots[0]);
        } else {
            assert(roots <= 16);
            setMultiroot(ss->node, roots);
            seekNode(ss, ss->inode + nodeHdrLen(ss->node));
            for (auto&& root : vpage.roots)
                copyAny(ss, root);
        }
    }
    auto rpno = ss->vpages.back().targetPgno;
    ss->pages->setRoot(rpno);

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
            logFatal,   // Invalid
            logFatal,   // Multiroot
            insertAtSeg,
            insertAtHalfSeg,
            insertAtFork,
            insertAtEndMark,
            insertAtRemote,
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
            ss->forks.emplace_back(ss->pgno, subs[i].pos, ss->kpos);
            break;
        }
    }
    ss->kval = keyVal(ss->key, ++ss->kpos);
    auto sub = subs.back();
    seekNode(ss, sub.pos + sub.len);
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
    auto slen = segLen(ss->node);

    if (ss->klen - ss->kpos < slen)
        return false;
    if (memcmp(ss->key.data() + ss->kpos / 2, segData(ss->node), slen / 2))
        return false;
    ss->kpos += slen;
    if (ss->kpos != ss->klen)
        ss->kval = keyVal(ss->key, ss->kpos);
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

//===========================================================================
static bool containsAtHalfSeg(SearchState * ss) {
    if (ss->kpos == ss->klen)
        return false;
    auto sval = halfSegVal(ss->node);
    if (ss->kval != sval)
        return false;
    if (++ss->kpos == ss->klen) {
        if (nodeEndMarkFlag(ss->node)) {
            ss->found = true;
            return false;
        }
    } else {
        ss->kval = keyVal(ss->key, ss->kpos);
        if (nodeEndMarkFlag(ss->node))
            return false;
    }
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
    seekRemote(ss, ss->inode);
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
        logFatal,   // Invalid
        logFatal,   // Multiroot
        containsAtSeg,
        containsAtHalfSeg,
        containsAtFork,
        containsAtEndMark,
        containsAtRemote,
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
    seekRemote(ss, ss->inode);
    return true;
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
        logFatal,   // Invalid
        logFatal,   // Multiroot
        lowerBoundAtSeg,
        lowerBoundAtHalfSeg,
        lowerBoundAtFork,
        lowerBoundAtEndMark,
        lowerBoundAtRemote,
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
*   StrTrie
*
***/

//===========================================================================
StrTrie::StrTrie () {
    construct(&m_heapImpl);
}

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
*   Stats
*
***/

namespace {

struct Stats {
    size_t usedPages;
    size_t totalPages;
    size_t pageSize;

    size_t usedBytes;
    size_t totalNodes;
    size_t numMultiroot;

    size_t segCount;
    size_t segTotalLen;
    size_t segMaxLen;
    size_t segShortLen;

    size_t halfSegCount;
    size_t halfSegLast;
    size_t halfSegPairs;
    
    size_t numFork;
    size_t totalForkLen;
    size_t numEndMark;
    size_t numRemote;
};

} // namespace

//===========================================================================
static int addStats(Stats * out, StrTrie::Node * node) {
    auto len = nodeHdrLen(node);
    auto num = numKids(node);
    out->usedBytes += len;
    out->totalNodes += 1;
    switch (nodeType(node)) {
    default:
        assert(!"Invalid node type");
        break;
    case kNodeMultiroot:
        out->numMultiroot += 1;
        break;
    case kNodeSeg:
    {
        auto slen = segLen(node);
        out->segCount += 1;
        out->segTotalLen += slen;
        if (slen <= 16) {
            out->segShortLen += 1;
        } else if (slen == kMaxSegLen) {
            out->segMaxLen += 1;
        }
        break;
    }
    case kNodeHalfSeg:
        out->halfSegCount += 1;
        if (nodeEndMarkFlag(node)) {
            out->halfSegLast += 1;
        } else if (nodeType(node + nodeHdrLen(node)) == kNodeHalfSeg) {
            out->halfSegPairs += 1;
        }
        break;
    case kNodeFork:
        out->numFork += 1;
        out->totalForkLen += num;
        break;
    case kNodeEndMark:
        out->numEndMark += 1;
        break;
    case kNodeRemote:
        out->numRemote += 1;
        break;
    }
    for (auto i = 0; i < num; ++i)
        len += addStats(out, node + len);
    return len;
}

//===========================================================================
void StrTrie::dumpStats(std::ostream & os) {
    Stats stats = {};
    stats.totalPages = m_heapImpl.pageCount();
    stats.pageSize = m_heapImpl.pageSize();
    for (size_t i = 0; i < stats.totalPages; ++i) {
        if (!m_heapImpl.empty(i)) {
            stats.usedPages += 1;
            auto node = (Node *) m_heapImpl.ptr(i);
            addStats(&stats, node);
        }
    }
    size_t totalBytes = stats.totalPages * stats.pageSize;
    os << "Bytes: " << totalBytes << '\n'
        << "Pages: " << stats.totalPages << " total, " 
            << stats.usedPages << " used (" 
            << 100.0 * stats.usedPages / stats.totalPages << "%)\n"
        << "Fill factor: " << 100.0 * stats.usedBytes / totalBytes << "%\n";
    os << "Node detail:\n"
        << "  Total: " << stats.totalNodes << " nodes\n"
        << "  Multiroot: " << stats.numMultiroot << " nodes\n"
        << "  Seg: " << stats.segCount << " nodes, " 
            << (double) stats.segTotalLen / stats.segCount << " avg len, "
            << 100.0 * stats.segShortLen / stats.segCount << "% short, "
            << 100.0 * stats.segMaxLen / stats.segCount << "% full\n"
        << "  HalfSeg: " << stats.halfSegCount << " nodes, "
            << stats.halfSegPairs << " pairs, "
            << 100.0 * stats.halfSegLast / stats.halfSegCount << "% last\n"
        << "  Fork: " << stats.numFork << " nodes, "
            << (double) stats.totalForkLen / stats.numFork << " avg forks\n"
        << "  EndMark: " << stats.numEndMark << " nodes\n"
        << "  Remote: " << stats.numRemote << " nodes\n";
}


/****************************************************************************
*
*   Dump data
*
***/

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
    case kNodeMultiroot:
        os << " Multiroot[" << multirootLen(ss->node) << "]";
        break;
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
            otmp << kid.pos << ' ';
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
            seekNode(ss, kid.pos);
            dumpAny(os, ss);
        }
        seekNode(ss, subs.back().pos + subs.back().len);
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
