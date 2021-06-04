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
    enum { kInvalid, kSource, kUpdate } type = kInvalid;
    pgno_t pgno = 0;

    auto operator<=>(const PageRef & other) const = default;
};

struct NodeRef {
    PageRef page;
    int pos = -1;
    pgno_t vpgno = (pgno_t) -1;

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
    union {
        struct {
            NodeRef next;   // next.pgno is the vpage number
            unsigned len;
            uint8_t data[kForkChildCount];
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
    PageRef root;
    map<PageRef, UnsignedSet> nodes;
    UnsignedSet refPages;
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
static NodeRef getRemoteRef(SearchState * ss, size_t inode) {
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
    [[maybe_unused]] auto id = ss->updates.size();
    auto & upd = ss->updates.emplace_back();
    auto i = ss->updateByRef.insert(pair(ref, ss->updates.size() - 1)).first;
    assert(i->second == id);
    return upd;
}

//===========================================================================
static UpdateNode & newSeg(
    SearchState * ss,
    int idest
) {
    auto & upd = newUpdate(ss, idest);
    upd.type = kNodeSeg;
    upd.seg.next = {};
    upd.seg.len = 0;
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
    if (zero) {
        upd.fork.endOfKey = false;
        uninitialized_default_construct_n(upd.fork.next, size(upd.fork.next));
    }
    return upd;
}

//===========================================================================
static void mapFringeNode(
    SearchState * ss,
    VirtualPage * vpage,
    UnsignedSet * nodes,
    int inode
) {
    if (!inode) {
        // no node
        return;
    } else if (inode >= numNodes(*ss)) {
        // remote node
        auto ref = getRemoteRef(ss, inode);
        assert(ref.pos == 0);
        assert(ref.page.type == PageRef::kSource);
        vpage->refPages.insert(ref.page.pgno);
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
        .type = PageRef::kSource,
        .pgno = ss->pgno
    };
    auto & nodes = vpage.nodes[vpage.root];
    mapFringeNode(ss, &vpage, &nodes, inode);
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
    unsigned nval = ss->node->data[1];
    if (nval < numNodes(*ss)) {
        upd.seg.next.pos = nval;
        upd.seg.next.page = {};
    } else {
        upd.seg.next = getRemoteRef(ss, nval);
    }
    for (unsigned i = 0; i < len; ++i) {
        auto sval = segVal(ss->node, i + pos);
        upd.seg.data[i] = sval;
    }
    return upd;
}

//===========================================================================
// Returns true if there is more to do
static bool insertAtSeg(SearchState * ss) {
    // Will either split on first, middle, last, after last, or will advance
    // to next node.

    auto spos = (uint8_t) 0; // split point where key diverges from segment
    auto slen = segLen(ss->node);
    auto sval = segVal(ss->node, spos);
    assert(slen > 1);
    for (;;) {
        if (ss->kval != sval)
            break;
        ss->kval = keyVal(ss->key, ++ss->kpos);
        if (++spos == slen) {
			// End of segment, still more key
            ss->inode = ss->node->data[1];
            if (ss->inode == 255) {
                // Fork with after segment end mark
                sval = 255;
                break;
            }
            // Continue search at next node
            return true;
        }
        sval = segVal(ss->node, spos);
    }

    // Replace segment with [lead segment] fork [tail segment]
    assert(spos < slen);
    auto inext = (unsigned) 0;
    UpdateNode * tail = nullptr;
    if (spos >= slen - 1) {
        inext = ss->node->data[1];
    } else {
        // Add tail segment
        inext = ss->nNodes++;
        tail = &copySeg(ss, inext, spos + 1, slen - spos - 1);
    }
    PageRef page = { PageRef::kUpdate, ss->pgno };
    if (!spos) {
        // Only single element after detaching tail, so no lead segment needed,
        // add as fork.
        assert(inext != 255);
        auto & upd = newFork(ss, ss->inode);
        upd.fork.next[sval].pos = inext;
        upd.fork.next[sval].page = {};
        ss->inode = (ss->kpos + 1 == ss->klen) ? -1 : ss->nNodes;
        upd.fork.next[ss->kval].pos = ss->inode;
        upd.fork.next[ss->kval].page = {};
    } else {
        // Copy truncated node as the lead segment.
        auto & lead = copySeg(ss, ss->inode, 0, spos);
        // Point at fork that's about to be added.
        lead.seg.next.pos = ss->nNodes;

        // Add fork.
        ss->inode = ++ss->nNodes;
        auto & upd = newFork(ss, ss->inode);
        upd.fork.next[sval].pos = inext;
        upd.fork.next[sval].page = {};
        if (ss->kpos == ss->klen) {
            upd.fork.next[ss->kval].pos = -1;
            upd.fork.next[ss->kval].page = {};
            return false;
        }
        upd.fork.next[ss->kval].pos = ss->inode;
        upd.fork.next[ss->kval].page = {};
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
    auto nodeCount = numNodes(*ss);
    for (auto i = 0; i < size(upd.fork.next); ++i) {
        unsigned nval = ss->node->data[i + 2];
        if (nval < nodeCount) {
            upd.fork.next[i].pos = nval;
            upd.fork.next[i].page = {};
        } else {
            upd.fork.next[i] = getRemoteRef(ss, nval);
        }
    }
    return upd;
}

//===========================================================================
// Returns true if there is more to do
static bool insertAtFork (SearchState * ss) {
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


    auto inext = forkChild(ss->node, ss->kval);
    if (!inext) {
        if (ss->kpos + 1 == ss->klen) {
            upd.fork.next[ss->kval].pos = -1;
        } else {
            upd.fork.next[ss->kval].pos = ss->nNodes;
        }
        ss->kval = keyVal(ss->key, ++ss->kpos);
        ss->inode = ss->nNodes;
    } else if (inext == 255) {
        upd.fork.next[ss->kval].pos = ss->nNodes;
        ss->kval = keyVal(ss->key, ++ss->kpos);

        ss->inode = ++ss->nNodes;
        auto & sub = newFork(ss, ss->inode);
        sub.fork.endOfKey = true;
    } else {
        ss->kval = keyVal(ss->key, ++ss->kpos);
        ss->inode = inext;
    }

    return true;
}

//===========================================================================
static void addSegs(SearchState * ss) {
    for (; ss->kpos < ss->klen; ss->kpos += kMaxSegLen) {
        auto & upd = newSeg(ss, ss->nNodes);
        ss->inode = ss->nNodes++;
        if (ss->klen - ss->kpos > kMaxSegLen) {
            upd.seg.len = kMaxSegLen;
            upd.seg.next.page = {};
            upd.seg.next.pos = ss->inode;
        } else {
            upd.seg.len = (unsigned) (ss->klen - ss->kpos);
            upd.seg.next.page = {};
            upd.seg.next.pos = -1;
        }
        for (auto spos = 0u; spos < upd.seg.len; ++spos, ++ss->kpos)
            upd.seg.data[spos] = keyVal(ss->key, ss->kpos);
    }
}

//===========================================================================
static int applyNode(
    SearchState * ss,
    const NodeRef & ref
) {
    auto newPos = ref.pos;
    auto & vpage = ss->vpages[ref.vpgno];
    NodeRef kid;
    if (ref.page.type == PageRef::kSource) {
        seekPage(ss, ref.page.pgno);
        seekNode(ss, ref.pos);
        for (auto&& inode : kids(*ss->node)) {
            if (inode >= ss->nNodes) {
                // TODO: get kid.page from foreign page references and check
                // if that page (and it's node 1) are on the current vpage.
                auto remote = getRemoteRef(ss, inode);
                if (vpage.refPages.contains(remote.page.pgno)) {
                } else {
                }
            } else {
                kid.page = ref.page;
                kid.vpgno = ref.vpgno;
                kid.pos = inode;
            }
            auto pos = applyNode(ss, kid);
            if (kid.pos == pos) {
            }
        }
    }

    assert(ref.page.type == PageRef::kUpdate);
    auto id = ss->updateByRef[ref];
    auto & upd = ss->updates[id];
    for (auto&& kid : kids(upd)) {
        auto pos = applyNode(ss, kid);
        if (kid.pos == pos && kid.page == ref.page)
            continue;
        kid = ref;
        kid.pos = pos;
        vpage.updateRefs.insert(pos);
    }

    return newPos;
}

//===========================================================================
static void applyUpdates(SearchState * ss) {
    struct UpdateInfo {
        NodeRef ref;
        size_t parent = (size_t) -1;
        size_t parentPos = (size_t) -1;
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
            if (kid.page.type == PageRef::kInvalid) {
                // skip empty child
            } else if (kid.page.type == PageRef::kUpdate) {
                ui.unprocessed += 1;
                auto id = ss->updateByRef[kid];
                assert(id);
                infos[id].parent = kv.second;
                infos[id].parentPos = pos;
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

    // Process updated nodes
    while (!unprocessed.empty()) {
        auto id = unprocessed.back();
        unprocessed.pop_back();
        auto & ui = infos[id];
        if (!ui.unprocessed) {
            // leaf
            auto & vpage = ss->vpages.emplace_back();
            vpage.root = ui.ref.page;
            auto & nodes = vpage.nodes[vpage.root];
            nodes.insert(ui.ref.pos);
        } else {
            // internal branch
            VirtualPage vmerge;
            vmerge.root = ui.ref.page;
            vmerge.nodes[vmerge.root].insert(ui.ref.pos);
            for (auto&& kid : kids(ss->updates[id])) {
                if (kid.page.type == PageRef::kInvalid)
                    continue;
                if (kid.vpgno == -1) {
                    assert(kid.page.type == PageRef::kSource);
                    // map fringe page
                    seekPage(ss, kid.page.pgno);
                    mapFringeNode(ss, kid.pos);
                    kid.vpgno = (pgno_t) ss->vpages.size() - 1;
                }
                auto & vpage = ss->vpages[kid.vpgno];
                vmerge.refPages.insert(vpage.refPages);
                for (auto&& [ref, nodes] : vpage.nodes)
                    vmerge.nodes[ref].insert(nodes);
            }
            auto nRefs = vmerge.refPages.size();
            auto nNodes = 0;
            for (auto&& kv : vmerge.nodes)
                nNodes += (int) kv.second.size();
            if (!checkCount(ss, nNodes, nRefs)) {
                auto & vpage = ss->vpages.emplace_back();
                vpage.root = ui.ref.page;
                vpage.nodes[vpage.root].insert(ui.ref.pos);
            } else {
                ss->vpages.emplace_back(move(vmerge));
                for (auto&& kid : kids(ss->updates[id])) {
                    if (kid.vpgno != -1) {
                        auto & vpage = ss->vpages[kid.vpgno];
                        vpage.nodes.clear();
                        vpage.refPages.clear();
                    }
                }
            }
        }

        // Update reference from parent
        if (ui.parent == -1) {
            // Adding root node as a leaf, must be adding first key to this
            // previously empty container.
            assert(unprocessed.empty());
        } else {
            auto & kid = kids(ss->updates[ui.parent])[ui.parentPos];
            kid.vpgno = (pgno_t) ss->vpages.size() - 1;
            auto & ni = infos[ui.parent];
            assert(ni.unprocessed > 1);
            if (--ni.unprocessed == 1) {
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
            cnt.count = vpage.nodes.size();
        }
        for (auto&& cnt : vcnts)
            counts.emplace_back(cnt.second);
    }
    sort(counts.begin(), counts.end(), [](auto & a, auto & b) {
        return a.count > b.count;
    });
    map<pgno_t, pgno_t> vtos;
    for (auto&& cnt : counts) {
        if (vpages.contains(cnt.vid) && spages.contains(cnt.sid)) {
            vtos[cnt.vid] = cnt.sid;
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
        vtos[id] = ss->pgno;
    }

    // Map node changes to source pages
    for (auto&& [vid, sid] : vtos) {
        auto & vpage = ss->vpages[vid];
        UnsignedSet used;
        auto nNodes = 0;
        for (auto&& kv : vpage.nodes) {
            nNodes += (int) kv.second.size();
            if (kv.first.pgno == vpage.root.pgno)
                used.insert(kv.second);
        }
        used.erase(nNodes, dynamic_extent);
        applyNode(ss, { .page = vpage.root, .pos = 0, .vpgno = vid });
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
        os << ss.inode << ": ";
        for (auto i = 0; i < kNodeLen; ++i) {
            if (i % 4 == 2) os.put(' ');
            hexByte(os, ss.node->data[i]);
        }

        switch (auto ntype = ::nodeType(ss.node)) {
        case kNodeInvalid:
            os << " -\n";
            break;
        case kNodeFork:
            os << " - Fork\n";
            break;
        case kNodeSeg:
            os << " - Segment[" << (int) segLen(ss.node) << "], nxt="
                << (int) ss.node->data[1] << "\n";
            break;
        default:
            os << " - UNKNOWN(" << ntype << ")\n";
            break;
        }
    }
    return os;
}
