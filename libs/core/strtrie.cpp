// Copyright Glen Knowles 2019 - 2024.
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
// Multiroot (1)
//  data[0] & 0x7 = kNodeMultiroot
//  data[0] & 0xf0 = number of roots (1 - 16)
// Segment (2) - must be byte aligned with key, no odd nibble starts.
//  data[0] & 0x7 = kNodeSeg
//  data[0] & 0x8 = has end of key
//  data[0] & 0xf0 = keyLen (1 - 16) in full bytes (no odd nibble)
//  data[1, keyLen] = bytes of key
//  : followed by next node - only if not eok
// Half Segment (3)
//  data[0] & 0x7 = kNodeHalfSeg
//  data[0] & 0x8 = has end of key
//  data[0] & 0xf0 = key (1 nibble in length)
//  : followed by next node - only if not eok
// Fork (4)
//  data[0] & 0x7 = kNodeFork
//  data[0] & 0x8 = has end of key
//  data[1, 2] = bitmap of existing children, all 16 bits means all 16 kids
//  : followed by nodes[popcount(bitmap)]
// EndMark (5)
//  data[0] & 0x7 = kNodeEndMark
//  data[0] & 0x8 = true (end of key - no content)
// Remote (6)
//  data[0] & 0x7 = kNodeRemote
//  data[0] & 0x8 = false (has end of key)
//  data[0] & 0xf0 = index of node (0 - 15)
//  data[1, 4] = pgno

const size_t kMaxForkBit = 16;
const size_t kMaxSegLen = 32;

const size_t kRemoteNodeLen = 5;
const size_t kForkNodeHdrLen = 3;

const size_t kMinPageSize = kForkNodeHdrLen + kMaxForkBit * kRemoteNodeLen;
const size_t kMaxPageSize = 65'536;

const size_t kMaxNodeTypes = 8;

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
static_assert(kNodeTypes <= kMaxNodeTypes);

struct UpdateBase {
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
    int kpos = -1;
};
struct SearchState {
    string_view key;
    int kpos = 0;
    int klen = 0;
    uint8_t kval = 0;

    // Current page
    pgno_t pgno = (pgno_t) -1;

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
        TempHeap * heap,
        IPageHeap * pages,
        string_view key,
        ostream * debugStream = nullptr
    );
};

} // namespace

// forward declarations
static ostream & dumpPage(ostream & os, SearchState * ss);
static void seekRootNode(SearchState * ss);


/****************************************************************************
*
*   StrTrieBase::Node
*
***/

struct StrTrieBase::Node {
    uint8_t data[1];

    static SearchState * makeState(
        TempHeap * heap,
        const StrTrieBase * cont,
        string_view key,
        bool seek = true
    );
};
static_assert(sizeof StrTrieBase::Node == 1);

//===========================================================================
SearchState * StrTrieBase::Node::makeState(
    TempHeap * heap,
    const StrTrieBase * cont,
    string_view key,
    bool seek
) {
    auto ss = heap->emplace<SearchState>(
        heap,
        cont->m_pages,
        key,
        cont->debugStream()
    );
    if (seek) {
        assert(!ss->pages->empty());
        seekRootNode(ss);
    }
    return ss;
}


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
static int multirootLen(const StrTrieBase::Node * node) {
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
static string_view segView(
    const StrTrieBase::Node * node,
    size_t spos = 0,
    size_t slen = dynamic_extent
) {
    assert(nodeType(node) == kNodeSeg);
    assert(spos % 2 == 0);
    assert(slen == dynamic_extent || slen % 2 == 0);
    if (slen == dynamic_extent)
        slen = segLen(node) - spos;
    return {(const char *) segData(node) + spos / 2, slen / 2};
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
    return popcount(forkBits(node));
}

//===========================================================================
static int firstForkVal(uint16_t bits) {
    auto val = countl_zero(bits);
    return val;
}

//===========================================================================
static int lastForkVal(uint16_t bits) {
    auto val = kMaxForkBit - countr_zero(bits) - 1;
    return (int) val;
}

//===========================================================================
static int nextForkVal(uint16_t bits, int kval) {
    assert(kval >= 0 && kval < kMaxForkBit);
    auto mask = (1u << (kMaxForkBit - kval - 1)) - 1;
    auto nbits = uint16_t(bits & mask);
    if (!nbits)
        return -1;
    auto nval = countl_zero(nbits);
    return nval;
}

//===========================================================================
static int prevForkVal(uint16_t bits, int kval) {
    assert(kval >= 0 && kval < kMaxForkBit);
    auto mask = 65535u << (kMaxForkBit - kval);
    auto nbits = uint16_t(bits & mask);
    if (!nbits)
        return -1;
    auto nval = kMaxForkBit - countr_zero(nbits) - 1;
    return (int) nval;
}

//===========================================================================
static pmr::basic_string<uint8_t> forkVals(SearchState * ss, uint16_t bits) {
    pmr::basic_string<uint8_t> out(ss->heap);
    out.reserve(popcount(bits));
    for (uint8_t i = 0; bits; ++i, bits <<= 1) {
        if (bits & 0x8000)
            out.push_back(i);
    }
    return out;
}

//===========================================================================
static pmr::basic_string<uint8_t> forkVals(
    SearchState * ss,
    const StrTrieBase::Node * node,
    int kval = -1
) {
    assert(nodeType(node) == kNodeFork);
    auto bits = forkBits(node);
    if (kval > -1)
        bits |= (0x8000 >> kval);
    return forkVals(ss, bits);
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
static int forkPos(uint16_t bits, size_t val) {
    assert(val < kMaxForkBit);
    return popcount((uint16_t) (bits >> (kMaxForkBit - val)));
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
static pgno_t remotePage(const StrTrieBase::Node * node) {
    assert(nodeType(node) == kNodeRemote);
    return (pgno_t) ntoh32(node->data + 1);
}

//===========================================================================
static int remotePos(const StrTrieBase::Node * node) {
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
[[nodiscard]] static NodeRef makeUpdateRef(SearchState * ss) {
    NodeRef out;
    out.page = { .type = PageRef::kUpdate };
    out.data = { .pos = (int) ss->updates.size(), .len = -1 };
    return out;
}

//===========================================================================
[[nodiscard]] static NodeRef makeSourceRef(
    SearchState * ss,
    size_t pos,
    size_t len
) {
    assert(pos < INT_MAX);
    assert(len < INT_MAX);
    NodeRef out;
    out.page = { .type = PageRef::kSource, .pgno = ss->pgno };
    out.data = { .pos = (int) pos, .len = (int) len };
    return out;
}

//===========================================================================
[[nodiscard]] static NodeRef makeRemoteRef(
    SearchState * ss,
    pgno_t pgno,
    int pos
) {
    NodeRef out;
    out.page = { .type = PageRef::kVirtual, .pgno = pgno };
    out.data = { .pos = pos, .len = kRemoteNodeLen };
    return out;
}

//===========================================================================
// Allocs NodeRef pointing at next update node.
static NodeRef * newRef(SearchState * ss) {
    auto ref = ss->heap->emplace<NodeRef>(makeUpdateRef(ss));
    return ref;
}

//===========================================================================
// Allocs NodeRef pointing at position on current source page.
static NodeRef * newRef(SearchState * ss, ptrdiff_t srcPos, size_t srcLen) {
    assert(srcPos >= 0);
    auto ref = ss->heap->emplace<NodeRef>(makeSourceRef(ss, srcPos, srcLen));
    return ref;
}

//===========================================================================
// Allocs NodeRef pointing at node on remote virtual page.
[[maybe_unused]]
static NodeRef * newRef(SearchState * ss, pgno_t pgno) {
    assert(pgno >= 0);
    auto ref = ss->heap->emplace<NodeRef>(makeRemoteRef(ss, pgno, 0));
    return ref;
}

//===========================================================================
// SearchState helpers
//===========================================================================
static void allocPage(SearchState * ss) {
    ss->pgno = (pgno_t) ss->pages->create();
}

//===========================================================================
static bool logFatal(SearchState * ss) {
    logMsgFatal() << "Invalid StrTrieBase node type: "
        << (int) nodeType(ss->node);
    return false;
}

//===========================================================================
static StrTrieBase::Node * getNodeForUpdate(
    SearchState * ss,
    pgno_t pgno,
    size_t pos
) {
    auto ptr = ss->pages->wptr(pgno);
    assert(ptr);
    assert(pos < ss->pages->pageSize() + 1);
    return reinterpret_cast<StrTrieBase::Node *>(ptr + pos);
}

//===========================================================================
static void seekNodeForUpdate(SearchState * ss, pgno_t pgno, size_t inode) {
    ss->pgno = pgno;
    ss->inode = (int) inode;
    ss->node = getNodeForUpdate(ss, ss->pgno, ss->inode);
}

//===========================================================================
static void seekNodeForUpdate(SearchState * ss, size_t inode) {
    seekNodeForUpdate(ss, ss->pgno, inode);
}

//===========================================================================
static const StrTrieBase::Node * getNode(
    SearchState * ss,
    pgno_t pgno,
    size_t pos
) {
    auto ptr = ss->pages->ptr(pgno);
    assert(ptr);
    assert(pos < ss->pages->pageSize() + 1);
    return reinterpret_cast<const StrTrieBase::Node *>(ptr + pos);
}

//===========================================================================
static const StrTrieBase::Node * getNode(SearchState * ss, size_t pos) {
    return getNode(ss, ss->pgno, pos);
}

//===========================================================================
static void seekNode(SearchState * ss, pgno_t pgno, size_t inode) {
    ss->pgno = pgno;
    ss->inode = (int) inode;
    ss->node = const_cast<StrTrieBase::Node *>(getNode(ss, ss->inode));
}

//===========================================================================
static void seekNode(SearchState * ss, size_t inode) {
    seekNode(ss, ss->pgno, inode);
}

//===========================================================================
static void seekRootNode(SearchState * ss) {
    seekNode(ss, (pgno_t) ss->pages->root(), 0);
}

//===========================================================================
static int getKidOffset(SearchState * ss, size_t pos) {
    assert(pos <= numKids(ss->node));
    auto root = ss->node - ss->inode;
    auto inext = ss->inode + nodeHdrLen(ss->node);
    for (auto i = 0; i < pos; ++i)
        inext += nodeLen(root + inext);
    return inext;
}

//===========================================================================
static void seekKidForUpdate(SearchState * ss, size_t pos) {
    auto inode = getKidOffset(ss, pos);
    seekNodeForUpdate(ss, inode);
}

//===========================================================================
static void seekKid(SearchState * ss, size_t pos) {
    auto inode = getKidOffset(ss, pos);
    seekNode(ss, inode);
}

//===========================================================================
static void seekRemote(SearchState * ss) {
    auto pos = remotePos(ss->node);
    seekNode(ss, remotePage(ss->node), 0);
    if (::nodeType(ss->node) != kNodeMultiroot) {
        assert(pos == 0);
    } else {
        auto roots = kids(ss, 0);
        assert(pos < roots.size());
        seekNode(ss, roots[pos].pos);
    }
}

//===========================================================================
static void consumeIfRemote(SearchState * ss, bool forUpdate) {
    if (nodeType(ss->node) == kNodeRemote) {
        seekRemote(ss);
        if (forUpdate)
            ss->spages.insert(ss->pgno);
    }
}

//===========================================================================
static bool nextKeyVal(SearchState * ss) {
    assert(ss->kpos < ss->klen);
    if (++ss->kpos != ss->klen) {
        ss->kval = keyVal(ss->key, ss->kpos);
        return true;
    } else {
        return false;
    }
}

//===========================================================================
static void setFoundKey(SearchState * ss) {
    if (ss->kpos % 2 == 0) {
        ss->foundKey = ss->key.substr(0, ss->kpos / 2);
        ss->foundKeyLen = ss->kpos;
    } else {
        ss->foundKey = ss->key.substr(0, (ss->kpos + 1) / 2);
        ss->foundKey.back() &= 0xf0;
        ss->foundKeyLen = ss->kpos;
    }
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
static void pushFoundKeyVal(SearchState * ss, string_view val) {
    assert(ss->foundKeyLen % 2 == 0);
    ss->foundKey += val;
    ss->foundKeyLen += 2 * val.size();
}

//===========================================================================
// Pushes seg or half seg value onto found key, advances to next node (following
// remote node if present), and returns false if there is no following node.
static bool pushFoundKeyConsume(SearchState * ss, bool forUpdate = false) {
    if (nodeType(ss->node) == kNodeSeg) {
        pushFoundKeyVal(ss, segView(ss->node));
    } else {
        assert(nodeType(ss->node) == kNodeHalfSeg);
        pushFoundKeyVal(ss, halfSegVal(ss->node));
    }
    if (nodeEndMarkFlag(ss->node))
        return false;

    seekKid(ss, 0);
    consumeIfRemote(ss, forUpdate);
    return true;
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
    TempHeap * heap,
    IPageHeap * pages,
    string_view key,
    ostream * dstrm
)
    : key(key)
    , klen((int) key.size() * 2)
    , forks(heap)
    , foundKey(heap)
    , updates(heap)
    , vpages(heap)
    , spages(heap)
    , heap(heap)
    , pages(pages)
    , debugStream(dstrm)
{
    if (klen)
        kval = keyVal(key, kpos);
}


/****************************************************************************
*
*   StrTrieBase
*
***/

//===========================================================================
StrTrieBase::StrTrieBase(IPageHeap * pages) noexcept
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

//===========================================================================
StrTrieBase::Iter StrTrieBase::begin() const {
    return ++end();
}

//===========================================================================
StrTrieBase::Iter StrTrieBase::end() const {
    auto impl = make_shared<Iter::Impl>(this);
    auto iter = Iter(impl);
    return iter;
}

//===========================================================================
StrTrieBase::reverse_iterator StrTrieBase::rbegin() const {
    return reverse_iterator(end());
}

//===========================================================================
StrTrieBase::reverse_iterator StrTrieBase::rend() const {
    return reverse_iterator(begin());
}


/****************************************************************************
*
*   Update helpers (UpdateBase and derived)
*
***/

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
requires derived_from<T, UpdateBase>
static T & newUpdate(SearchState * ss) {
    auto * upd = ss->heap->emplace<T>();
    upd->id = (int) ss->updates.size();
    upd->type = T::s_type;
    ss->updates.push_back(upd);
    return *upd;
}

//===========================================================================
static string_view segView(
    UpdateBase * upd,
    size_t spos = 0,
    size_t slen = dynamic_extent
) {
    assert(upd->type == kNodeSeg);
    assert(spos % 2 == 0);
    assert(slen == dynamic_extent || slen % 2 == 0);
    auto seg = static_cast<UpdateSeg *>(upd);
    if (slen == dynamic_extent)
        slen = seg->keyLen - spos;
    return {(const char *) seg->key + spos / 2, slen / 2};
}

//===========================================================================
static void setSegKey(UpdateSeg * out, SearchState * ss, string_view key) {
    out->keyLen = (int) key.size() * 2;
    out->key = (uint8_t *) ss->heap->strDup(key);
}

//===========================================================================
static void setForkBit(
    UpdateFork * fork,
    size_t val,
    bool enable
) {
    fork->kidBits = setForkBit(fork->kidBits, val, enable);
}


/****************************************************************************
*
*   Update helpers (SearchState)
*
***/

//===========================================================================
[[nodiscard]] static NodeRef addKeyRef(SearchState * ss) {
    auto out = makeUpdateRef(ss);
    if (!nextKeyVal(ss))
        newUpdate<UpdateEndMark>(ss);
    return out;
}

//===========================================================================
[[nodiscard]] static NodeRef copyRef(SearchState * ss) {
    NodeRef out;
    if (nodeEndMarkFlag(ss->node)) {
        out = makeUpdateRef(ss);
        newUpdate<UpdateEndMark>(ss);
    } else {
        auto hdrLen = nodeHdrLen(ss->node);
        out = makeSourceRef(
            ss,
            ss->inode + hdrLen,
            nodeLen(ss->node + hdrLen)
        );
    }
    return out;
}

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
    auto kval = ss->kval;
    addHalfSeg(ss, kval, !nextKeyVal(ss));
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
[[nodiscard]] static pair<NodeRef *, NodeRef *> addFork(
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
static void copySeg(SearchState * ss) {
    assert(nodeType(ss->node) == kNodeSeg);
    auto & upd = newUpdate<UpdateSeg>(ss);
    setSegKey(&upd, ss, segView(ss->node));
    if (nodeEndMarkFlag(ss->node)) {
        upd.endOfKey = true;
    } else {
        upd.refs = { newRef(ss), 1 };
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
    setSegKey(&upd, ss, segView(ss->node, 0, pos));
    upd.refs = { newRef(ss), 1 };
}

//===========================================================================
[[nodiscard]] static NodeRef copySegSuffix(SearchState * ss, size_t pos) {
    assert(nodeType(ss->node) == kNodeSeg);
    assert(pos <= segLen(ss->node));
    assert(pos % 2 == 0);
    auto len = segLen(ss->node) - pos;
    if (!len)
        return copyRef(ss);

    auto out = makeUpdateRef(ss);
    auto & upd = newUpdate<UpdateSeg>(ss);
    setSegKey(&upd, ss, segView(ss->node, pos));
    if (nodeEndMarkFlag(ss->node)) {
        upd.endOfKey = true;
    } else {
        // TODO: merge with following NodeSeg if present.
        auto hdrLen = nodeHdrLen(ss->node);
        auto srcRef = newRef(
            ss,
            ss->inode + hdrLen,
            nodeLen(ss->node + hdrLen)
        );
        upd.refs = { srcRef, 1 };
    }
    return out;
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
static void copyHalfSegSuffix(SearchState * ss, size_t spos) {
    assert(nodeType(ss->node) == kNodeSeg);
    auto sval = segVal(ss->node, ++spos);
    auto slen = segLen(ss->node);
    if (spos + 1 == slen && nodeEndMarkFlag(ss->node)) {
        addHalfSeg(ss, sval, true);
    } else {
        auto ref = addHalfSeg(ss, sval, false);
        *ref = copySegSuffix(ss, spos + 1);
    }
}

//===========================================================================
static UpdateFork & copyForkWithKey(SearchState * ss, int * inext) {
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
            upd.refs[i] = makeSourceRef(ss, refs[i].pos, refs[i].len);
        return upd;
    }

    upd.endOfKey = nodeEndMarkFlag(ss->node);
    upd.kidBits = setForkBit(bits, ss->kval, true);
    auto fpos = forkPos(upd.kidBits, ss->kval);
    if (bits != upd.kidBits) {
        upd.refs = ss->heap->allocSpan<NodeRef>(refs.size() + 1);
        for (auto i = 0; i < fpos; ++i)
            upd.refs[i] = makeSourceRef(ss, refs[i].pos, refs[i].len);
        upd.refs[fpos] = makeUpdateRef(ss);
        for (auto i = fpos; i < refs.size(); ++i)
            upd.refs[i + 1] = makeSourceRef(ss, refs[i].pos, refs[i].len);
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
            seekNode(ss, rpno, 0);
            rrefs = kids(ss, 0);
            seekNode(ss, pgno, inode);
        }
    }
    for (auto i = 0; i < refs.size(); ++i) {
        if (i == fpos) {
            upd.refs[i] = makeUpdateRef(ss);
            continue;
        }
        if (rpno == -1) {
            upd.refs[i] = makeSourceRef(ss, refs[i].pos, refs[i].len);
            continue;
        }
        fnode = getNode(ss, refs[i].pos);
        if (nodeType(fnode) == kNodeRemote && rpno == remotePage(fnode)) {
            // Remote reference to same page that update extends to, use source
            // from that page.
            assert(!rrefs.empty());
            auto & rref = rrefs[remotePos(fnode)];
            upd.refs[i] = makeSourceRef(ss, rref.pos, rref.len);
            upd.refs[i].page.pgno = rpno;
        } else {
            // Remote page different from what update will, use this remote
            // reference.
            upd.refs[i] = makeSourceRef(ss, refs[i].pos, refs[i].len);
        }
    }
    return upd;
}


/****************************************************************************
*
*   Apply updates
*
***/

static void copyAny(SearchState * ss, const UpdateBase & upd);
static void applyDestroys(SearchState * ss);

//===========================================================================
static void copyAny(SearchState * ss, const NodeRef & ref) {
    if (ref.page.type == PageRef::kUpdate) {
        copyAny(ss, *ss->updates[ref.data.pos]);
    } else if (ref.page.type == PageRef::kSource) {
        auto node = getNode(ss, ref.page.pgno, ref.data.pos);
        assert(nodeLen(node) == ref.data.len);
        memcpy(ss->node, node, ref.data.len);
        seekNodeForUpdate(ss, ss->inode + ref.data.len);
    } else {
        assert(ref.page.type == PageRef::kVirtual);
        auto & vpage = ss->vpages[ref.page.pgno];
        setRemoteRef(ss->node, vpage.targetPgno, ref.data.pos);
        seekNodeForUpdate(ss, ss->inode + nodeLen(ss->node));
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
    seekKidForUpdate(ss, 0);
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

    int vmap[kMaxForkBit] = {};
    struct VPInfo {
        pgno_t pgno;
        int len;
        int pos;
    };
    basic_string<VPInfo> vinfos;

    for (;;) {
        vinfos.push_back({
            .pgno = (pgno_t) ss->vpages.size(),
            .len = 0,
            .pos = 0
        });
        assert(vinfos.size() <= 3);
        auto & nvi = vinfos.back();
        ss->vpages.emplace_back(ss->heap);
        for (auto i = 0; i < nrefs; ++i) {
            if (vmap[i])
                continue;
            bool multi = nvi.len;
            if (nvi.len + refs[i]->data.len <= psize - multi) {
                vmap[i] = (int) vinfos.size();
                nvi.len += refs[i]->data.len;
                upd->len -= refs[i]->data.len - kRemoteNodeLen;
            }
        }

        // NOTE: nvi.len *must* be >0, >= psize/2 is just a conjecture!
        assert(nvi.len >= psize / 2);

        if (upd->len <= psize)
            break;
    }
    for (auto i = 0; i < nrefs; ++i) {
        // Add kids to new pages and change the parents link to them to be
        // remote references.
        if (auto vpos = vmap[i]) {
            auto & vi = vinfos[vpos - 1];
            ss->vpages[vi.pgno].roots.push_back(*refs[i]);
            *refs[i] = makeRemoteRef(ss, vi.pgno, vi.pos++);
        }
    }
    for ([[maybe_unused]] auto & vi : vinfos)
        assert(vi.pos);
    assert(upd->len == nodeLen(*upd));
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

        // - for leaf: 0
        // - for branch: 1 + number of unprocessed kids
        size_t unprocessed = 0;
    };
    vector<UpdateInfo> infos(ss->updates.size());
    vector<int> unblocked;
    for (auto&& upd : ss->updates) {
        auto id = upd->id;
        auto & ui = infos[id];
        ui.upd = upd;
        for (auto pos = 0; auto&& kid : ui.upd->refs) {
            if (kid.page.type == PageRef::kUpdate) {
                assert(kid.data.pos < infos.size());
                assert(infos[kid.data.pos].parent == -1);
                ui.unprocessed += 1;
                infos[kid.data.pos].parent = id;
                infos[kid.data.pos].kidPos = pos;
            } else {
                assert(kid.page.type == PageRef::kSource);
            }
            pos += 1;
        }
        if (!ui.upd->refs.empty()) {
            // Add one for being a branch instead of just a leaf.
            ui.unprocessed += 1;
        }
        if (ui.unprocessed < 2)
            unblocked.push_back(id);
    }
    assert(!unblocked.empty());

    // Process updated nodes and related fringe nodes into virtual pages. Start
    // with leaf nodes, with branches becoming unblocked for processing when all
    // their leaves are done. Continues all the way up until the root node is
    // processed. Child virtual pages are merged up into their parents when
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

        // Update reference from parent.
        if (ui.parent == -1) {
            // Adding root node.
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
            // Parent has had all children processed, queue for processing.
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
        seekNodeForUpdate(ss, vpage.targetPgno, 0);
        if (auto roots = vpage.roots.size(); roots == 1) {
            copyAny(ss, vpage.roots[0]);
        } else {
            assert(roots <= 16);
            setMultiroot(ss->node, roots);
            seekKidForUpdate(ss, 0);
            for (auto&& root : vpage.roots)
                copyAny(ss, root);
        }
    }

    auto rpno = ss->vpages.back().targetPgno;
    ss->pages->setRoot(rpno);
    applyDestroys(ss);
}

//===========================================================================
static void applyDestroys(SearchState * ss) {
    // Destroy replaced source pages.
    for (auto&& id : ss->spages)
        ss->pages->destroy(id);

    if (ss->debugStream) {
        *ss->debugStream << "Pages freed (" << ss->spages.size() << "): "
            << ss->spages << '\n';
        if (ss->vpages.empty())
            *ss->debugStream << "Page: No Pages\n";
        for (auto&& vpage : ranges::reverse_view(ss->vpages)) {
            seekNode(ss, vpage.targetPgno, 0);
            dumpPage(*ss->debugStream, ss);
        }
    }
}


/****************************************************************************
*
*   Insert
*
***/

//===========================================================================
// If the search should continue:
//      returns true
//      sets ss->node and ss->inode to the next node
// If the search is over:
//      returns false
//      sets ss->found to true if the key was found, otherwise remains false
static bool insertAtSeg(SearchState * ss) {
    // Will either split on first, middle, last, after last (possible if it's
    // the end of the key), or will advance to next node.

    assert(ss->kpos % 2 == 0);
    auto spos = 0; // Split point where key diverges from segment.
    auto slen = segLen(ss->node);
    auto sval = segVal(ss->node, spos);
    assert(slen > 0);

    for (;;) {
        if (ss->kpos == ss->klen) {
            // Fork end of key with position in the segment.
            //
            // Insert "a" [61] into Seg "abc" [61 62 63] makes:
            //  +---------+  +-----------+
            //  | Seg "a" |--| Fork--EOK |
            //  +---------+  |   |       |
            //               |   |       |  +--------+  +---------+
            //               |   +--  6  |--| Half 2 |--| Seg "c" |
            //               +-----------+  +--------+  +---------+
            copySegPrefix(ss, spos);
            addForkWithEnd(ss, sval, true);
            copyHalfSegSuffix(ss, spos);
            break;
        }
        if (ss->kval != sval) {
            // Fork inside the key and the segment.
            if (spos % 2 == 0) {
                // Fork in the first half of the byte.
                //
                // Insert "ayz" [61 79 7a] info Seg "abc" [61 62 63] makes:
                //  +---------+  +-----------+  +--------+  +----------+
                //  | Seg "a" |--| Fork-- 6  |--| Half 2 |--| Seg "c"  |
                //  +---------+  |   |       |  +--------+  +----------+
                //               |   |       |  +--------+  +----------+
                //               |   +--  7  |--| Half 9 |--| Seg "z"  |
                //               +-----------+  +--------+  +----------+
                copySegPrefix(ss, spos);
                auto [sref, kref] = addFork(ss, sval, ss->kval);
                *sref = makeUpdateRef(ss);
                copyHalfSegSuffix(ss, spos);
                *kref = makeUpdateRef(ss);
                nextKeyVal(ss);
                addKeyHalfSeg(ss);
                break;
            } else {
                // Fork in the second half of the byte.
                //
                // Insert "aef" [61 65 66] into Seg "abc" [61 62 63] makes:
                //  +---------+  +--------+  +-----------+  +----------+
                //  | Seg "a" |--| Half 6 |--| Fork-- 2  |--| Seg "c"  |
                //  +---------+  +--------+  |   |       |  +----------+
                //                           |   |       |  +----------+
                //                           |   +--  5  |--| Seg "f"  |
                //                           +-----------+  +----------+
                copySegPrefix(ss, spos - 1);
                addHalfSeg(ss, segVal(ss->node, spos - 1), false);
                auto [sref, kref] = addFork(ss, sval, ss->kval);
                *sref = copySegSuffix(ss, spos + 1);
                *kref = addKeyRef(ss);
                break;
            }
        }
        nextKeyVal(ss);
        if (++spos == slen) {
            if (nodeEndMarkFlag(ss->node)) {
                if (ss->kpos == ss->klen) {
                    // End of both segment and key? Key was found, abort.
                    ss->found = true;
                    return false;
                }
                // Fork key with the end mark that's after the segment.
                //
                // Insert "abcdef" [61 62 63 64 65 66] into
                // Seg "abc" [61 62 63] makes:
                //  +-----------+  +-----------+  +--------+  +----------+
                //  | Seg "abc" |--| Fork-- 6  |--| Half 4 |--| Seg "ef" |
                //  +-----------+  |   |       |  +--------+  +----------+
                //                 |   |       |
                //                 |   +-- EOK |
                //                 +-----------+
                copySegPrefix(ss, spos);
                addForkWithEnd(ss, ss->kval, true);
                nextKeyVal(ss);
                addKeyHalfSeg(ss);
                break;
            }
            // Continue search at next node.
            copySegPrefix(ss, spos);
            seekKid(ss, 0);
            return true;
        }
        sval = segVal(ss->node, spos);
    }

    // Continue processing the key.
    assert(ss->kpos % 2 == 0);
    return false;
}

//===========================================================================
// Returns true if there is more to do; otherwise false and sets ss->found.
static bool insertAtHalfSeg (SearchState * ss) {
    // Will either split on value, after value (possible if it's the end of the
    // key), or will advance to next node.

    auto sval = halfSegVal(ss->node);

    for (;;) {
        if (ss->kpos == ss->klen) {
            // Fork end of key with half seg.
            //
            // Insert "" [] into { Half 6 } makes:
            //  +-----------+
            //  | Fork--EOK |
            //  |   |       |
            //  |   |       |
            //  |   +--  6  |
            //  +-----------+
            auto ref = addForkWithEnd(ss, sval, false);
            *ref = copyRef(ss);
            break;
        }
        if (ss->kval != sval) {
            // Fork inside the key with the half seg.
            //
            // Insert "a" [61] into { Half 7 } makes:
            //  +-----------+  +--------+
            //  | Fork-- 6  |--| Half 1 |
            //  |   |       |  +--------+
            //  |   |       |
            //  |   +--  7  |
            //  +-----------+
            auto [sref, kref] = addFork(ss, sval, ss->kval);
            *sref = copyRef(ss);
            *kref = addKeyRef(ss);
            break;
        }
        nextKeyVal(ss);
        if (nodeEndMarkFlag(ss->node)) {
            if (ss->kpos == ss->klen) {
                // End of both key and half seg? Key was found, abort.
                ss->found = true;
                return false;
            }
            // Fork key with the end mark that's after the half seg.
            //
            // Insert "ab" [61 62] into { Half 6 } makes:
            //  +--------+  +-----------+  +---------+
            //  | Half 6 |--| Fork-- 1  |--| Seg "b" |
            //  +--------+  |   |       |  +---------+
            //              |   |       |
            //              |   +-- EOK |
            //              +-----------+
            copyHalfSeg(ss, false);
            addForkWithEnd(ss, ss->kval, true);
            nextKeyVal(ss);
            break;
        }
        // Continue search at next node.
        copyHalfSeg(ss, false);
        seekKid(ss, 0);
        return true;
    }

    // Continue processing the key.
    return false;
}

//===========================================================================
// Returns true if there is more to do; otherwise false and sets ss->found.
static bool insertAtFork (SearchState * ss) {
    // Will either add branch to fork, or will advance to next node.

    int inext;
    if (ss->kpos == ss->klen) {
        if (nodeEndMarkFlag(ss->node)) {
            ss->found = true;
        } else {
            copyForkWithKey(ss, &inext);
        }
        return false;
    }

    copyForkWithKey(ss, &inext);
    bool matched = forkBit(ss->node, ss->kval);
    nextKeyVal(ss);
    if (matched) {
        seekNode(ss, inext);
        return true;
    } else {
        if (ss->kpos == ss->klen)
            newUpdate<UpdateEndMark>(ss);
        return false;
    }
}

//===========================================================================
// Returns true if there is more to do; otherwise false and sets ss->found.
static bool insertAtEndMark(SearchState * ss) {
    assert(ss->kpos % 2 == 0);
    if (ss->kpos == ss->klen) {
        ss->found = true;
        return false;
    }

    // Fork key with end mark.
    //
    // Insert "a" [61] into { EndMark } makes:
    //  +-----------+  +--------+
    //  | Fork-- 6  |--| Half 1 |
    //  |   |       |  +--------+
    //  |   |       |
    //  |   +-- EOK |
    //  +-----------+
    addForkWithEnd(ss, ss->kval, true);
    nextKeyVal(ss);
    return false;
}

//===========================================================================
// Returns true if there is more to do; otherwise false and sets ss->found.
static bool insertAtRemote(SearchState * ss) {
    seekRemote(ss);
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

//===========================================================================
// Returns true if key was inserted, false if it was already present.
bool StrTrieBase::insert(string_view key) {
    TempHeap heap;
    auto ss = Node::makeState(&heap, this, key, false);
    if (!empty()) {
        seekRootNode(ss);
        ss->spages.insert(ss->pgno);

        using Fn = bool(SearchState *);
        constinit static Fn * const functs[] = {
            logFatal,   // Invalid
            logFatal,   // Multiroot
            insertAtSeg,
            insertAtHalfSeg,
            insertAtFork,
            insertAtEndMark,
            insertAtRemote,
            logFatal,
        };
        static_assert(size(functs) == kMaxNodeTypes);
        for (;;) {
            auto ntype = ::nodeType(ss->node);
            auto fn = functs[ntype];
            if (!(*fn)(ss))
                break;
        }
        if (ss->found)
            return false;
    }

    // Add any trailing key segments.
    addSegs(ss);

    // Apply pending updates.
    assert(!ss->updates.empty());
    applyUpdates(ss);

    // Was an insert; return true.
    return true;
}


/****************************************************************************
*
*   Erase
*
***/

//===========================================================================
static bool eraseAtSeg(SearchState * ss) {
    assert(ss->kpos % 2 == 0);
    auto slen = segLen(ss->node);

    if (ss->klen - ss->kpos < slen)
        return false;
    if (memcmp(ss->key.data() + ss->kpos / 2, segData(ss->node), slen / 2))
        return false;

    ss->kpos += slen - 1;
    nextKeyVal(ss);
    if (nodeEndMarkFlag(ss->node)) {
        if (ss->kpos == ss->klen) {
            copySeg(ss);
            ss->found = true;
        }
        return false;
    }
    copySeg(ss);
    seekKid(ss, 0);
    return true;
}

//===========================================================================
static bool eraseAtHalfSeg(SearchState * ss) {
    if (ss->kpos == ss->klen)
        return false;
    auto sval = halfSegVal(ss->node);
    if (ss->kval != sval)
        return false;

    nextKeyVal(ss);
    if (nodeEndMarkFlag(ss->node)) {
        if (ss->kpos == ss->klen) {
            copyHalfSeg(ss, true);
            ss->found = true;
        }
        return false;
    }
    copyHalfSeg(ss, false);
    seekKid(ss, 0);
    return true;
}

//===========================================================================
static bool eraseAtFork(SearchState * ss) {
    // Set foundKeyLen to length including the first variance, which in this
    // case is one past the current position whether or not it's just an end
    // mark.
    ss->foundKeyLen = ss->kpos + 1;

    int inext;
    if (ss->kpos == ss->klen) {
        if (nodeEndMarkFlag(ss->node)) {
            copyForkWithKey(ss, &inext);
            ss->found = true;
        }
        return false;
    }
    if (!forkBit(ss->node, ss->kval))
        return false;

    copyForkWithKey(ss, &inext);
    seekNode(ss, inext);
    // Advance kpos *after* copying the old kval.
    nextKeyVal(ss);
    return true;
}

//===========================================================================
static bool eraseAtEndMark(SearchState * ss) {
    if (ss->kpos == ss->klen)
        ss->found = true;
    return false;
}

//===========================================================================
static bool eraseAtRemote(SearchState * ss) {
    seekRemote(ss);
    ss->spages.insert(ss->pgno);
    return true;
}

//===========================================================================
static void eraseForkKid(SearchState * ss, UpdateFork * fork) {
    auto vals = forkVals(ss, fork->kidBits);
    for (auto i = 0; i < fork->refs.size(); ++i) {
        if (fork->refs[i].page.type == PageRef::kUpdate) {
            setForkBit(fork, vals[i], false);
            copy(
                fork->refs.begin() + i + 1,
                fork->refs.end(),
                fork->refs.begin() + i
            );
            fork->refs = fork->refs.subspan(0, fork->refs.size() - 1);
            break;
        }
    }
    if (vals.size() == fork->refs.size()) {
        assert(fork->endOfKey);
        fork->endOfKey = false;
    }
}

//===========================================================================
static void eraseForkWithEnd(SearchState * ss) {
    ss->updates.pop_back(); // Remove fork
    if (ss->updates.size()) {
        // If parent is seg or half seg, update parent with end mark.
        auto upd = ss->updates.back();
        auto ntype = upd->type;
        if (ntype == kNodeSeg) {
            auto & half = *static_cast<UpdateHalfSeg *>(upd);
            half.endOfKey = true;
            half.refs = {};
            return;
        } else if (ntype == kNodeHalfSeg) {
            auto & seg = *static_cast<UpdateSeg *>(upd);
            seg.endOfKey = true;
            seg.refs = {};
            return;
        }
    }
    // Otherwise replace fork with end mark.
    newUpdate<UpdateEndMark>(ss);
}

//===========================================================================
static void eraseForkWithSegs(SearchState * ss, const UpdateFork & fork) {
    ss->updates.pop_back(); // Remove fork
    auto index = fork.refs[0].page.type == PageRef::kSource ? 0 : 1;

    // Is fork on low nibble of byte, rather than high one?
    auto lowFork = ss->foundKeyLen % 2 == 0;

    // Set foundKeyLen to 0 to clear foundKey for use as a compound seg node.
    ss->foundKeyLen = 0;

    auto & sref = fork.refs[index];
    assert(sref.page.type == PageRef::kSource);
    seekNode(ss, sref.page.pgno, sref.data.pos);

    auto vals = forkVals(ss, fork.kidBits);
    auto sval = vals[index];
    bool endMark = false;

    if (lowFork
        && !ss->updates.empty()
        && ss->updates.back()->type == kNodeHalfSeg
    ) {
        //   +------+------+
        // --| half | fork |--
        //   +------+---+--+
        //              |     +--------+
        //              +-----| erased |
        //                    +--------+

        auto half = static_cast<UpdateHalfSeg *>(ss->updates.back());
        ss->updates.pop_back();
        if (!ss->updates.empty()
            && ss->updates.back()->type == kNodeSeg
        ) {
            auto seg = static_cast<UpdateSeg *>(ss->updates.back());
            if (seg->keyLen < kMaxSegLen) {
                // +-----+  +------+------+
                // | SEG |--| half | fork |--
                // +-----+  +------+---+--+
                ss->updates.pop_back();
                pushFoundKeyVal(ss, segView(seg));
            }
        }

        //   +------+------+
        // --| HALF | FORK |--
        //   +------+---+--+
        pushFoundKeyVal(ss, half->nibble);
        pushFoundKeyVal(ss, sval);

        if (nodeType(ss->node) == kNodeEndMark) {
            //   +------+------+  +-----+
            // --| half | fork |--| END |
            //   +------+---+--+  +-----+
            endMark = true;
        } else if (nodeType(ss->node) == kNodeSeg
            && ss->foundKeyLen + segLen(ss->node) <= kMaxSegLen
        ) {
            //   +------+------+  +-----+
            // --| half | fork |--| SEG |--
            //   +------+---+--+  +-----+
            endMark = !pushFoundKeyConsume(ss, true);
        }
    } else if (!lowFork
        && nodeType(ss->node) == kNodeHalfSeg
    ) {
        //   +------+------+
        // --| fork | half |--
        //   +---+--+------+
        //       |     +--------+
        //       +-----| erased |
        //             +--------+

        if (!ss->updates.empty()
            && ss->updates.back()->type == kNodeSeg
        ) {
            auto seg = static_cast<UpdateSeg *>(ss->updates.back());
            if (seg->keyLen < kMaxSegLen) {
                // +-----+  +------+------+
                // | SEG |--| fork | half |--
                // +-----+  +---+--+------+
                ss->updates.pop_back();
                pushFoundKeyVal(ss, segView(seg));
            }
        }

        //   +------+------+
        // --| FORK | HALF |--
        //   +---+--+------+
        pushFoundKeyVal(ss, sval);
        endMark = !pushFoundKeyConsume(ss, true);

        if (!endMark
            && nodeType(ss->node) == kNodeSeg
            && ss->foundKeyLen + segLen(ss->node) <= kMaxSegLen
        ) {
            //   +------+------+  +-----+
            // --| fork | half |--| SEG |
            //   +---+--+------+  +-----+
            endMark = !pushFoundKeyConsume(ss, true);
        }
    } else {
        //   +------+
        // --| fork |--
        //   +---+--+
        //       |     +--------+
        //       +-----| erased |
        //             +--------+

        if (nodeType(ss->node) == kNodeEndMark) {
            //   +------+  +-----+
            // --| fork |--| END |
            //   +---+--+  +-----+
            endMark = true;
        }
    }

    // Add replacement seg or half seg node.
    if (ss->foundKeyLen) {
        assert(ss->foundKeyLen >= 2 && ss->foundKeyLen % 2 == 0);
        auto & seg = newUpdate<UpdateSeg>(ss);
        setSegKey(&seg, ss, ss->foundKey);
        seg.endOfKey = endMark;
    } else {
        auto & half = newUpdate<UpdateHalfSeg>(ss);
        half.nibble = sval;
        half.endOfKey = endMark;
    }
    if (!endMark) {
        auto srcRef = newRef(ss, ss->inode, nodeLen(ss->node));
        ss->updates.back()->refs = { srcRef, 1 };
    }
}

//===========================================================================
bool StrTrieBase::erase(string_view key) {
    if (empty())
        return false;

    TempHeap heap;
    auto ss = Node::makeState(&heap, this, key);
    ss->spages.insert(ss->pgno);

    // Walk node path, add entries to ss->updates, and set ss->foundKeyLen to
    // the shortest length at which the key uniquely differs with all other
    // values. This will be one past the end if it's equal to a prefix of
    // another value.
    using Fn = bool(SearchState *);
    constinit static Fn * const functs[] = {
        logFatal,   // Invalid
        logFatal,   // Multiroot
        eraseAtSeg,
        eraseAtHalfSeg,
        eraseAtFork,
        eraseAtEndMark,
        eraseAtRemote,
        logFatal,
    };
    static_assert(size(functs) == kMaxNodeTypes);
    for (;;) {
        auto ntype = ::nodeType(ss->node);
        auto fn = functs[ntype];
        if (!(*fn)(ss))
            break;
    }
    if (!ss->found)
        return false;

    // Unwind to closest fork.
    while (!ss->updates.empty()) {
        auto upd = ss->updates.back();
        if (upd->type == kNodeFork)
            break;
        ss->updates.pop_back();
    }
    if (ss->updates.empty()) {
        // No fork, must be the last key, just remove pages.
        applyDestroys(ss);
        ss->pages->setRoot((pgno_t) -1);
        assert(ss->pages->empty());
        return true;
    }

    auto & fork = *static_cast<UpdateFork *>(ss->updates.back());

    if (fork.refs.size() + fork.endOfKey > 2) {
        // Has more than two kids (including eok), keep fork but with branch
        // removed.
        eraseForkKid(ss, &fork);
    } else if (fork.endOfKey
        && fork.refs[0].page.type == PageRef::kUpdate
    ) {
        // Only remaining kid is an end mark, replace fork with end mark.
        eraseForkWithEnd(ss);
    } else {
        // One remaining kid, replace fork with half seg and merge it with
        // adjacent segs and half segs.
        eraseForkWithSegs(ss, fork);
    }

    // Apply pending updates
    assert(!ss->updates.empty());
    applyUpdates(ss);

    // Was an erase, return true.
    return true;
}


/****************************************************************************
*
*   Clear
*
***/

//===========================================================================
static int nodeLenAndPages(USet * out, const StrTrieBase::Node * node) {
    auto len = nodeHdrLen(node);
    auto type = nodeType(node);
    if (type == kNodeRemote) {
        out->insert(remotePage(node));
        return len;
    }
    auto num = numKids(node);
    for (auto i = 0; i < num; ++i)
        len += nodeLenAndPages(out, node + len);
    assert(len > 0 && len < INT_MAX);
    return len;
}

//===========================================================================
void StrTrieBase::clear() {
    if (empty())
        return;

    TempHeap heap;
    auto ss = Node::makeState(&heap, this, {}, false);
    USet processed(ss->heap);
    USet unprocessed(ss->heap);
    unprocessed.insert((pgno_t) ss->pages->root());
    while (unprocessed) {
        ss->spages.clear();
        for (auto && pgno : unprocessed) {
            seekNode(ss, pgno, 0);
            nodeLenAndPages(&ss->spages, ss->node);
        }
        processed.insert(unprocessed);
        swap(ss->spages, unprocessed);
        unprocessed.erase(processed);
    }
    swap(ss->spages, processed);
    applyDestroys(ss);
    ss->pages->setRoot((pgno_t) -1);
    assert(ss->pages->empty());
}


/****************************************************************************
*
*   Contains
*
***/

//===========================================================================
static bool containsAtSeg(SearchState * ss) {
    assert(ss->kpos % 2 == 0);
    auto slen = segLen(ss->node);

    if (ss->klen - ss->kpos < slen)
        return false;
    if (memcmp(ss->key.data() + ss->kpos / 2, segData(ss->node), slen / 2))
        return false;

    ss->kpos += slen - 1;
    nextKeyVal(ss);
    if (nodeEndMarkFlag(ss->node)) {
        if (ss->kpos == ss->klen)
            ss->found = true;
        return false;
    }
    seekKid(ss, 0);
    return true;
}

//===========================================================================
static bool containsAtHalfSeg(SearchState * ss) {
    if (ss->kpos == ss->klen)
        return false;
    auto sval = halfSegVal(ss->node);
    if (ss->kval != sval)
        return false;

    nextKeyVal(ss);
    if (nodeEndMarkFlag(ss->node)) {
        if (ss->kpos == ss->klen)
            ss->found = true;
        return false;
    }
    seekKid(ss, 0);
    return true;
}

//===========================================================================
static bool containsAtFork(SearchState * ss) {
    if (ss->kpos == ss->klen) {
        if (nodeEndMarkFlag(ss->node))
            ss->found = true;
        return false;
    }
    if (!forkBit(ss->node, ss->kval))
        return false;

    auto pos = forkPos(ss->node, ss->kval);
    seekKid(ss, pos);
    // Advance kpos *after* using the old kval to get forkPos.
    nextKeyVal(ss);
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
    seekRemote(ss);
    return true;
}

//===========================================================================
bool StrTrieBase::contains(string_view key) const {
    if (empty())
        return false;

    TempHeap heap;
    auto ss = Node::makeState(&heap, this, key);

    using Fn = bool(SearchState *);
    constinit static Fn * const functs[] = {
        logFatal,   // Invalid
        logFatal,   // Multiroot
        containsAtSeg,
        containsAtHalfSeg,
        containsAtFork,
        containsAtEndMark,
        containsAtRemote,
        logFatal,
    };
    static_assert(size(functs) == kMaxNodeTypes);
    for (;;) {
        auto ntype = ::nodeType(ss->node);
        auto fn = functs[ntype];
        if (!(*fn)(ss))
            return ss->found;
    }
}


/****************************************************************************
*
*   Find Helpers
*
***/

//===========================================================================
static void seekFront(SearchState * ss) {
    for (;;) {
        auto ntype = nodeType(ss->node);
        if (ntype == kNodeSeg) {
            if (!pushFoundKeyConsume(ss))
                break;
        } else if (ntype == kNodeHalfSeg) {
            if (!pushFoundKeyConsume(ss))
                break;
        } else if (ntype == kNodeFork) {
            auto & fork = ss->forks.emplace_back();
            fork.kpos = (int) ss->foundKeyLen;
            fork.pgno = ss->pgno;
            fork.inode = ss->inode;
            if (nodeEndMarkFlag(ss->node))
                break;
            auto bits = forkBits(ss->node);
            auto sval = firstForkVal(bits);
            pushFoundKeyVal(ss, sval);
            seekKid(ss, 0);
        } else if (ntype == kNodeEndMark) {
            break;
        } else {
            assert(ntype == kNodeRemote);
            seekRemote(ss);
        }
    }
}

//===========================================================================
static void seekNext(SearchState * ss) {
    while (!ss->forks.empty()) {
        auto & fork = ss->forks.back();
        seekNode(ss, fork.pgno, fork.inode);
        int nval = -1;
        if (fork.kpos == ss->klen) {
            nval = firstForkVal(forkBits(ss->node));
        } else {
            ss->kval = keyVal(ss->key, fork.kpos);
            nval = nextForkVal(forkBits(ss->node), ss->kval);
        }
        if (nval == -1) {
            ss->forks.pop_back();
            continue;
        }

        // Found alternate fork.
        ss->kpos = fork.kpos;
        setFoundKey(ss);
        ss->kpos += 1;
        ss->kval = (uint8_t) nval;
        pushFoundKeyVal(ss, ss->kval);
        auto pos = forkPos(ss->node, ss->kval);
        seekKid(ss, pos);
        seekFront(ss);
        return;
    }

    ss->inode = 0;
    ss->node = nullptr;
}

//===========================================================================
static void seekBack(SearchState * ss) {
    for (;;) {
        auto ntype = nodeType(ss->node);
        if (ntype == kNodeSeg) {
            if (!pushFoundKeyConsume(ss))
                break;
        } else if (ntype == kNodeHalfSeg) {
            if (!pushFoundKeyConsume(ss))
                break;
        } else if (ntype == kNodeFork) {
            auto & fork = ss->forks.emplace_back();
            fork.kpos = (int) ss->foundKeyLen;
            fork.pgno = ss->pgno;
            fork.inode = ss->inode;
            auto bits = forkBits(ss->node);
            auto sval = lastForkVal(bits);
            pushFoundKeyVal(ss, sval);
            auto pos = forkLen(ss->node) - 1;
            if (nodeEndMarkFlag(ss->node))
                pos += 1;
            seekKid(ss, pos);
        } else if (ntype == kNodeEndMark) {
            break;
        } else {
            assert(ntype == kNodeRemote);
            seekRemote(ss);
        }
    }
}

//===========================================================================
static void seekPrev(SearchState * ss) {
    while (!ss->forks.empty()) {
        auto & fork = ss->forks.back();
        seekNode(ss, fork.pgno, fork.inode);
        if (fork.kpos == ss->klen) {
            ss->forks.pop_back();
            continue;
        }
        ss->kval = keyVal(ss->key, fork.kpos);
        int nval = prevForkVal(forkBits(ss->node), ss->kval);
        if (nval == -1 && !nodeEndMarkFlag(ss->node)) {
            ss->forks.pop_back();
            continue;
        }

        // Found alternate fork.
        ss->kpos = fork.kpos;
        setFoundKey(ss);
        if (nval != -1) {
            ss->kpos += 1;
            ss->kval = (uint8_t) nval;
            pushFoundKeyVal(ss, ss->kval);
            auto pos = forkPos(ss->node, ss->kval);
            seekKid(ss, pos);
            seekBack(ss);
        }
        return;
    }

    ss->inode = 0;
    ss->node = nullptr;
}


/****************************************************************************
*
*   StrTrieBase::Iter
*
***/

struct StrTrieBase::Iter::Impl {
    const StrTrieBase * cont = nullptr;
    string current;
    vector<SearchFork> forks;
    bool endMark = true;
};

//===========================================================================
StrTrieBase::Iter::Iter(const Iter & from)
    : m_impl(make_shared<Impl>(*from.m_impl))
{}

//===========================================================================
StrTrieBase::Iter::Iter(const StrTrieBase * cont)
    : m_impl(make_shared<Impl>(cont))
{}

//===========================================================================
StrTrieBase::Iter::Iter(shared_ptr<Impl> impl)
    : m_impl(impl)
{}

//===========================================================================
StrTrieBase::Iter::operator bool() const {
    return !m_impl->endMark;
}

//===========================================================================
bool StrTrieBase::Iter::operator==(
    const Iter & other
) const {
    return m_impl->current == other.m_impl->current
        && m_impl->endMark == other.m_impl->endMark;
}

//===========================================================================
auto StrTrieBase::Iter::operator*() const
    -> const value_type &
{
    return m_impl->current;
}

//===========================================================================
StrTrieBase::Iter & StrTrieBase::Iter::operator++() {
    if (m_impl->cont->empty()) {
        assert(m_impl->endMark);
        return *this;
    }

    TempHeap heap;
    auto ss = Node::makeState(&heap, m_impl->cont, m_impl->current);
    if (m_impl->endMark) {
        seekRootNode(ss);
        seekFront(ss);
    } else {
        ss->forks.assign(m_impl->forks.begin(), m_impl->forks.end());
        seekNext(ss);
    }
    if (ss->node) {
        m_impl->current = ss->foundKey;
        m_impl->forks.assign(ss->forks.begin(), ss->forks.end());
        m_impl->endMark = false;
    } else {
        m_impl->current.clear();
        m_impl->forks.clear();
        m_impl->endMark = true;
    }
    return *this;
}

//===========================================================================
StrTrieBase::Iter & StrTrieBase::Iter::operator--() {
    if (m_impl->cont->empty()) {
        assert(m_impl->endMark);
        return *this;
    }

    TempHeap heap;
    auto ss = Node::makeState(&heap, m_impl->cont, m_impl->current);
    if (m_impl->endMark) {
        seekRootNode(ss);
        seekBack(ss);
    } else {
        ss->forks.assign(m_impl->forks.begin(), m_impl->forks.end());
        seekPrev(ss);
    }
    if (ss->node) {
        m_impl->current = ss->foundKey;
        m_impl->forks.assign(ss->forks.begin(), ss->forks.end());
        m_impl->endMark = false;
    } else {
        m_impl->current.clear();
        m_impl->forks.clear();
        m_impl->endMark = true;
    }
    return *this;
}


/****************************************************************************
*
*   find
*
***/

//===========================================================================
// Can be called from findAtSeg() and findAtHalfSeg().
template<bool kLess, bool kGreater, bool kEqual>
static bool findAtEndMark(SearchState * ss) {
    if constexpr (kEqual) {
        if (ss->kpos == ss->klen) {
            setFoundKey(ss);
            return false;
        }
    }
    if constexpr (kLess) {
        seekPrev(ss);
    } else if constexpr (kGreater) {
        seekNext(ss);
    }
    return false;
}

//===========================================================================
template<bool kLess, bool kGreater, bool kEqual>
static bool findAtSeg(SearchState * ss) {
    auto slen = segLen(ss->node);
    int rc = 0;

    auto kdataLen = ss->klen - ss->kpos;
    auto kdata = ss->key.data() + ss->kpos / 2;
    if (kdataLen < slen) {
        rc = memcmp(kdata, segData(ss->node), kdataLen / 2);
        if (!rc)
            rc = -1;
    } else {
        rc = memcmp(kdata, segData(ss->node), slen / 2);
    }

    if (rc == 0) {
        ss->kpos += slen - 1;
        nextKeyVal(ss);
        if (nodeEndMarkFlag(ss->node))
            return findAtEndMark<kLess, kGreater, kEqual>(ss);
        seekKid(ss, 0);
        return true;
    }

    if constexpr (!kLess && !kGreater) {
        // Not found and must be equal? Fail.
        ss->inode = 0;
        ss->node = nullptr;
    } else if (kLess) {
        // Not found, use nearest value.
        if (rc > 0) {
            // Back up to nearest fork and walk down adjacent branch.
            seekPrev(ss);
        } else {
            // Continue down this chain.
            setFoundKey(ss);
            pushFoundKeyConsume(ss);
            seekBack(ss);
        }
    } else if (kGreater) {
        // Not found, use nearest value.
        if (rc < 0) {
            // Back up to nearest fork and walk down adjacent branch.
            seekNext(ss);
        } else {
            // Continue down this chain.
            setFoundKey(ss);
            pushFoundKeyConsume(ss);
            seekFront(ss);
        }
    }
    return false;
}

//===========================================================================
template<bool kLess, bool kGreater, bool kEqual>
static bool findAtHalfSeg(SearchState * ss) {
    auto sval = halfSegVal(ss->node);
    auto rc = ss->kpos == ss->klen
        ? strong_ordering::less
        : ss->kval <=> sval;

    if (rc == 0) {
        nextKeyVal(ss);
        if (nodeEndMarkFlag(ss->node))
            return findAtEndMark<kLess, kGreater, kEqual>(ss);
        seekKid(ss, 0);
        return true;
    }

    if constexpr (!kLess && !kGreater) {
        // Not found and must be equal? Fail.
        ss->inode = 0;
        ss->node = nullptr;
    } else if (kLess) {
        // Not found, use nearest value.
        if (rc > 0) {
            // Back up to nearest fork and walk down adjacent branch.
            seekPrev(ss);
        } else {
            // Continue down this chain.
            setFoundKey(ss);
            pushFoundKeyConsume(ss);
            seekBack(ss);
        }
    } else if (kGreater) {
        // Not found, use nearest value.
        if (rc < 0) {
            // Back up to nearest fork and walk down adjacent branch.
            seekNext(ss);
        } else {
            // Continue down this chain.
            setFoundKey(ss);
            pushFoundKeyConsume(ss);
            seekFront(ss);
        }
    }
    return false;
}

//===========================================================================
template<bool kLess, bool kGreater, bool kEqual>
static bool findAtFork(SearchState * ss) {
    if (ss->kpos == ss->klen) {
        if constexpr (kEqual) {
            if (nodeEndMarkFlag(ss->node)) {
                setFoundKey(ss);
                return false;
            }
        }
        if constexpr (kLess) {
            seekPrev(ss);
        } else if constexpr (kGreater) {
            seekFront(ss);
        }
        return false;
    }
    auto bits = forkBits(ss->node);
    if (forkBit(bits, ss->kval)) {
        // kval == sval
        auto & fork = ss->forks.emplace_back();
        fork.kpos = ss->kpos;
        fork.pgno = ss->pgno;
        fork.inode = ss->inode;
        auto pos = forkPos(bits, ss->kval);
        seekKid(ss, pos);
        // Advance kpos *after* using the old kval to get forkPos.
        nextKeyVal(ss);
        return true;
    }

    // Not found, must be equal? fail.
    if constexpr (!kLess && !kGreater) {
        ss->inode = 0;
        ss->node = nullptr;
    } else if (kLess) {
        // Not found, use nearest value.
        auto sval = prevForkVal(bits, ss->kval);
        if (sval == -1) {
            // Back up to previous fork and walk down adjacent branch.
            seekPrev(ss);
        } else {
            // Follow value down this fork.
            setFoundKey(ss);
            pushFoundKeyVal(ss, sval);
            seekKid(ss, forkPos(bits, sval));
            seekBack(ss);
        }
    } else if (kGreater) {
        // Not found, use nearest value.
        auto sval = nextForkVal(bits, ss->kval);
        if (sval == -1) {
            // Back up to previous fork and walk down adjacent branch.
            seekNext(ss);
        } else {
            // Follow value down this fork.
            setFoundKey(ss);
            pushFoundKeyVal(ss, sval);
            seekKid(ss, forkPos(bits, sval));
            seekFront(ss);
        }
    }
    return false;
}

//===========================================================================
template<bool kLess, bool kGreater, bool kEqual>
static bool findAtRemote(SearchState * ss) {
    seekRemote(ss);
    return true;
}

//===========================================================================
template<bool kLess, bool kGreater, bool kEqual>
static StrTrieBase::Iter find(const StrTrieBase * cont,  string_view key) {
    static_assert(!kLess || !kGreater);
    static_assert(kLess || kGreater || kEqual);

    auto out = make_shared<StrTrieBase::Iter::Impl>(cont);
    if (cont->empty())
        return StrTrieBase::Iter(out);

    TempHeap heap;
    auto ss = StrTrieBase::Node::makeState(&heap, cont, key);

    using Fn = bool(SearchState *);
    constinit static Fn * const functs[] = {
        logFatal,   // Invalid
        logFatal,   // Multiroot
        findAtSeg<kLess, kGreater, kEqual>,
        findAtHalfSeg<kLess, kGreater, kEqual>,
        findAtFork<kLess, kGreater, kEqual>,
        findAtEndMark<kLess, kGreater, kEqual>,
        findAtRemote<kLess, kGreater, kEqual>,
        logFatal,
    };
    static_assert(size(functs) == kMaxNodeTypes);
    for (;;) {
        auto ntype = ::nodeType(ss->node);
        auto fn = functs[ntype];
        if (!(*fn)(ss))
            break;
    }

    if (ss->node) {
        out->current = ss->foundKey;
        out->forks.assign(ss->forks.begin(), ss->forks.end());
        out->endMark = false;
    }
    return StrTrieBase::Iter(out);
}

//===========================================================================
StrTrieBase::Iter StrTrieBase::find(string_view key) const {
    return ::find<false, false, true>(this, key);
}

//===========================================================================
StrTrieBase::Iter StrTrieBase::findLess(string_view key) const {
    return ::find<true, false, false>(this, key);
}

//===========================================================================
StrTrieBase::Iter StrTrieBase::findLessEqual(string_view key) const {
    return ::find<true, false, true>(this, key);
}

//===========================================================================
StrTrieBase::Iter StrTrieBase::lowerBound(string_view key) const {
    return ::find<false, true, true>(this, key);
}

//===========================================================================
StrTrieBase::Iter StrTrieBase::upperBound(string_view key) const {
    return ::find<false, true, false>(this, key);
}

//===========================================================================
auto StrTrieBase::equalRange(string_view key) const
    -> pair<Iter, Iter>
{
    auto first = lowerBound(key);
    auto second = first;
    if (second && *second == key)
        ++second;
    pair<Iter, Iter> out = { move(first), move(second) };
    return out;
}

//===========================================================================
string StrTrieBase::front() const {
    return *begin();
}

//===========================================================================
string StrTrieBase::back() const {
    return *--end();
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
void StrTrie::debugStream(std::ostream * os) {
    m_dstream = os;
}

//===========================================================================
ostream * const StrTrie::debugStream() const {
    return m_dstream;
}


/****************************************************************************
*
*   Stats
*
***/

namespace {

struct PageStats {
    size_t parent = 0;
    size_t keys = 0;
    size_t depth = 0;
};

struct Stats {
    size_t usedPages;
    size_t totalPages;
    size_t pageSize;
    size_t totalKeys;

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

    UnsignedSet remotePages;
};

} // namespace

//===========================================================================
static int addStats(Stats * out, size_t pgno, StrTrie::Node * node) {
    auto len = nodeHdrLen(node);
    auto num = numKids(node);
    out->usedBytes += len;
    out->totalNodes += 1;
    auto ntype = nodeType(node);
    switch (ntype) {
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
        {
            out->numRemote += 1;
            auto rpno = remotePage(node);
            assert(rpno < out->totalPages);
            out->remotePages.insert(rpno);
            break;
        }
    }
    if (nodeEndMarkFlag(node))
        out->totalKeys += 1;
    for (auto i = 0; i < num; ++i)
        len += addStats(out, i, node + len);
    return len;
}

//===========================================================================
static size_t pageDepthStats(size_t pgno, vector<PageStats> & pgstats) {
    auto & ps = pgstats[pgno];
    if (!ps.depth)
        ps.depth = pageDepthStats(ps.parent, pgstats);
    return ps.depth + 1;
}

//===========================================================================
void StrTrie::dumpStats(std::ostream & os) const {
    Stats stats = {};
    stats.totalPages = m_heapImpl.pageCount();
    stats.pageSize = m_heapImpl.pageSize();
    vector<PageStats> pgstats(stats.totalPages);
    pgstats[m_heapImpl.root()] = {
        .parent = (size_t) -1,
        .depth = 1
    };
    for (size_t i = 0; i < stats.totalPages; ++i) {
        if (!m_heapImpl.empty(i)) {
            stats.usedPages += 1;
            auto node = (Node *) m_heapImpl.ptr(i);
            auto keys = stats.totalKeys;
            stats.remotePages.clear();
            addStats(&stats, i, node);
            pgstats[i].keys = stats.totalKeys - keys;
            for (auto&& rpno : stats.remotePages) {
                assert(!pgstats[rpno].parent);
                pgstats[rpno].parent = i;
            }
        } else {
            pgstats[i].parent = (size_t) -2;
        }
    }
    size_t totalBytes = stats.totalPages * stats.pageSize;
    os << "Bytes: " << totalBytes << '\n'
        << "Pages: " << stats.totalPages << " total, "
            << stats.usedPages << " used ("
            << 100.0 * stats.usedPages / stats.totalPages << "%), "
            << "size=" << stats.pageSize << "\n"
        << "Keys: " << stats.totalKeys << " total\n"
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

    struct DepthStats {
        size_t pages;
        size_t keys;
    };
    vector<DepthStats> dstats;
    for (auto i = 0; i < stats.totalPages; ++i) {
        auto & ps = pgstats[i];
        if (ps.parent == -2)
            continue;
        if (!ps.depth)
            ps.depth = pageDepthStats(ps.parent, pgstats);
        if (ps.depth >= dstats.size())
            dstats.resize(ps.depth + 1);
        dstats[ps.depth].pages += 1;
        dstats[ps.depth].keys += ps.keys;
    }
    os << "Density by page depth:\n";
    for (auto i = 1; i < dstats.size(); ++i) {
        auto & d = dstats[i];
        os << "  " << i << ": " << d.pages << " pages, " << d.keys
            << " keys\n";
    }
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
        if (auto&& vals = forkVals(ss, ss->node); !vals.empty()) {
            for (auto i = 0; i < vals.size(); ++i) {
                os << ' ' << (int) vals[i];
                if (i % 5 == 4)
                    os << ' ';
            }
        }
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
        for (auto i = 0; i < subs.size(); ++i) {
            otmp << subs[i].pos << ' ';
            if (i % 5 == 4)
                otmp << ' ';
        }
        if (otmp.view().size() <= 28) {
            os << "  // " << otmp.view();
        } else {
            os << "\n          // " << otmp.view();
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
