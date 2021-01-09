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
const unsigned kMaxValueLen = 16;

enum NodeType : int8_t {
    kNodeInvalid,
    kNodeSeg,
    kNodeFork,
};

struct UpdateNode {
    NodeType type;
    union {
        struct {
            unsigned inext;
            unsigned len;
            uint8_t data[16];
        } seg;
        struct {
            bool endOfKey;
            unsigned inext[16];
        } fork;
    };
};

struct SearchState {
    string_view key;
    int kpos = 0;
    size_t klen = 0;
    uint8_t kval = 0;

    // Current page
    size_t pgno = 0;
    size_t nNodes = 0;

    // Current node
    int inode = 0;
    StrTrieBase::Node * node = nullptr;

    // Fork to adjacent key
    int fpos = -1;
    int ifork = -1;

    // Was key found?
    bool found = false;

    // Used when more nodes are being added than will fit on the page.
    vector<OverflowNode> overflow;

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
constexpr NodeType nodeType(const StrTrieBase::Node * node) {
    return NodeType(*node->data & 0x7);
}

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
constexpr uint8_t asSeg0(size_t len) {
    return (uint8_t) (kNodeSeg | ((len - 1) << 3));
}

//===========================================================================
static void setSegLen(StrTrieBase::Node * node, size_t len) {
    node->data[0] = asSeg0(len);
}

//===========================================================================
static uint8_t segLen(const StrTrieBase::Node * node) {
    return (*node->data >> 3) + 1;
}

//===========================================================================
static uint8_t segVal(const StrTrieBase::Node * node, size_t pos) {
    assert((node->data[0] & 0x7) == kNodeSeg);
    return pos % 2 == 0
        ? (node->data[2 + pos / 2] >> 4)
        : (node->data[2 + pos / 2] & 0xf);
}

//===========================================================================
static uint8_t pushSegVal(
    StrTrieBase::Node * node,
    size_t pos,
    uint8_t val
) {
    assert(pos < kMaxSegLen);
    if (pos % 2 == 0) {
        node->data[2 + pos / 2] = val << 4;
    } else {
        node->data[2 + pos / 2] |= val;
    }
    return val;
}

//===========================================================================
static size_t segNodesRequired (const SearchState & ss) {
    return (ss.klen - ss.kpos + kMaxSegLen - 1) / kMaxSegLen;
}

//===========================================================================
static uint8_t getFork(
    const StrTrieBase::Node * node,
    size_t pos
) {
    assert((node->data[0] & 0x7) == kNodeFork);
    assert(pos < 16);
    return node->data[pos + 2];
}

//===========================================================================
static size_t setFork(
    StrTrieBase::Node * node,
    uint8_t val,
    size_t inode
) {
    assert(val == 255 || val < 16);
    assert(inode < 256);

    auto inext = (uint8_t) inode;
    if (val == 255) {
        node->data[1] = inext;
    } else {
        node->data[val + 2] = inext;
    }
    return inext;
}

//===========================================================================
static void appendNode (
    vector<OverflowNode> * out,
    const StrTrieBase::Node & node
) {
    switch (auto type = nodeType(&node)) {
    default:
        assert(!"unknown node type");
        break;
    case kNodeSeg: {
            auto & seg = out->emplace_back(OverflowNode{type}).seg;
            seg.inext = node.data[1];
            seg.len = segLen(&node);
            copy(node.data + 2, node.data + 18, seg.data);
        }
        break;
    case kNodeFork: {
            auto & fork = out->emplace_back(OverflowNode{type}).fork;
            fork.endOfKey = node.data[1];
            copy(node.data + 2, node.data + 18, fork.inext);
        }
        break;
    }
}

//===========================================================================
static void copyNodes (
    vector<OverflowNode> * out,
    SearchState * ss
) {
    auto ptr = ss->node - ss->inode;
    auto last = ptr + ss->nNodes;
    for (; ptr <= last; ++ptr)
        appendNode(out, *ptr);
}

//===========================================================================
// size measured in nodes
static size_t size(SearchState * ss) {
    auto ptr = ss->heap->ptr(ss->pgno);
    return ptr[ss->heap->pageSize() - 1];
}

//===========================================================================
// capacity measured in nodes
static size_t capacity(SearchState * ss) {
    return (ss->heap->pageSize() - 1) / kNodeLen;
}

//===========================================================================
static StrTrieBase::Node * append(
    SearchState * ss,
    const StrTrieBase::Node & node
) {
    auto ptr = (StrTrieBase::Node *) ss->heap->ptr(ss->pgno);
    auto & last = ptr->data[ss->heap->pageSize() - 1];
    if (last == capacity(ss))
        return nullptr;
    auto inode = last++;
    auto out = ptr + inode;
    *out = node;
    return out;
}

//===========================================================================
static void setNode(SearchState * ss, size_t inode) {
    ss->inode = (int) inode;
    auto ptr = (StrTrieBase::Node *) ss->heap->ptr(ss->pgno);
    assert(ptr);
    ss->node = ptr + ss->inode;
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
// Returns true if there is more to do
static bool splitSegInPlace (
    SearchState * ss,
    uint8_t spos,
    uint8_t slen,
    uint8_t sval
) {
    auto inext = (uint8_t) 0;
	if (spos >= slen - 1) {
		inext = ss->node->data[1];
	} else {
        // Add tail segment
        auto tlen = slen - spos - 1;
        StrTrieBase::Node tnode = {
            asSeg0(tlen),
            ss->node->data[1]
        };
        for (auto tpos = 0; tpos < tlen; ++tpos) {
            auto sval = segVal(ss->node, tpos + spos + 1);
            pushSegVal(&tnode, tpos, sval);
        }
        append(ss, tnode);
        inext = (uint8_t) ss->nNodes++;
    }
    if (!spos) {
        // Only single element after detaching tail, so no lead segment
        // needed, convert to fork
        StrTrieBase::Node tnode = { kNodeFork };
        setFork(&tnode, sval, inext);
        ss->inode = (ss->kpos + 1 == ss->klen) ? 255 : (int) ss->nNodes;
        setFork(&tnode, ss->kval, ss->inode);
        *ss->node = tnode;
    } else {
        // Truncate segment as lead
        setSegLen(ss->node, spos);
        // Point at fork that's being added
        ss->node->data[1] = (uint8_t) ss->nNodes;
        // Add fork
        StrTrieBase::Node tnode = { kNodeFork };
        setFork(&tnode, sval, inext);
        if (ss->kpos == ss->klen) {
            setFork(&tnode, ss->kval, 255);
            append(ss, tnode);
            return false;
        }
        setFork(&tnode, ss->kval, ss->nNodes + 1);
        append(ss, tnode);
        ss->inode = (int) ++ss->nNodes;
    }

    ss->kval = keyVal(ss->key, ++ss->kpos);
    return true;
}

//===========================================================================
// Returns true if there is more to do
static bool splitSegToOverflow (
    SearchState * ss,
    uint8_t spos,
    uint8_t slen,
    uint8_t sval
) {
    assert(spos < slen);
    copyNodes(&ss->overflow, ss);

    auto inext = (uint8_t) 0;
    auto * node = &ss->overflow[ss->inode];
    if (spos >= slen - 1) {
        inext = node->seg.data[1];
    } else {
        // Add tail segment
        auto & tnode = ss->overflow.emplace_back(OverflowNode{kNodeSeg});
        tnode.seg.len = slen - spos - 1;
        tnode.seg.inext = inext;
        for (unsigned tpos = 0; tpos < tnode.seg.len; ++tpos) {
            auto sval = segVal(ss->node, tpos + spos + 1);
            tnode.seg.data[tpos] = sval;
        }
        inext = (uint8_t) ss->nNodes++;
    }
    if (!spos) {
        // Only single element after detaching tail, so no lead segment
        // needed, convert to fork
        assert(inext != 255);
        node->type = kNodeFork;
        node->fork.inext[sval] = inext;
        ss->inode = (ss->kpos + 1 == ss->klen) ? 255 : (int) ss->nNodes;
        node->fork.inext[ss->kval] = ss->inode;
    } else {
        // Truncate segment as lead
        node->seg.len = spos;
        // Point at fork that's being added
        node->seg.inext = (unsigned) ss->nNodes;
        // Add fork
        auto & tnode = ss->overflow.emplace_back(OverflowNode{kNodeFork});
        if (sval == 255) {
            tnode.fork.endOfKey = true;
        } else {
            tnode.fork.inext[sval] = inext;
        }
        if (ss->kpos == ss->klen) {
            tnode.fork.inext[ss->kval] = (unsigned) -1;
            return false;
        }
        tnode.fork.inext[ss->kval] = (unsigned) ss->nNodes + 1;
        ss->inode = (int) ++ss->nNodes;
    }

    ss->kval = keyVal(ss->key, ++ss->kpos);
    return true;
}

//===========================================================================
// Returns true if there is more to do
static bool insertAtSeg (SearchState * ss) {
    // May split on first, middle, last, after last, or advance to next node.

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
    auto adds = (spos > 0) // needs lead segment
        + (spos < slen); // needs tail segment
    if (capacity(ss) - size(ss)
        >= segNodesRequired(*ss) + adds
    ) {
        return splitSegInPlace(ss, spos, slen, sval);
    }

    return splitSegToOverflow(ss, spos, slen, sval);
}

//===========================================================================
// Returns true if there is more to do
static bool insertAtFork (SearchState * ss) {
    if (ss->kpos == ss->klen) {
        assert(ss->kval == 255);
        auto inext = ss->node->data[1];
        if (!inext) {
            setFork(ss->node, 255, 255);
        } else {
            assert(inext == 255);
            ss->found = true;
        }
        return false;
    }

    auto inext = getFork(ss->node, ss->kval);
    if (!inext) {
        if (ss->kpos + 1 == ss->klen) {
            setFork(ss->node, ss->kval, 255);
            return false;
        }
        setFork(ss->node, ss->kval, ss->nNodes);
        ss->kval = keyVal(ss->key, ++ss->kpos);
        if (capacity(ss) - size(ss)
            < segNodesRequired(*ss)
        ) {
            copyNodes(&ss->overflow, ss);
        }
        ss->inode = (int) ss->nNodes;
    } else if (inext == 255) {
        setFork(ss->node, ss->kval, ss->nNodes);
        ss->kval = keyVal(ss->key, ++ss->kpos);

		StrTrieBase::Node tnode = { kNodeFork };
		setFork(&tnode, 255, 255);
        if (capacity(ss) - size(ss)
            < segNodesRequired(*ss) + 1
        ) {
            copyNodes(&ss->overflow, ss);
            appendNode(&ss->overflow, tnode);
    		ss->inode = (int) ++ss->nNodes;
        } else {
            append(ss, tnode);
    		ss->inode = (int) ++ss->nNodes;
        }
    } else {
        ss->kval = keyVal(ss->key, ++ss->kpos);
        ss->inode = inext;
    }

    return true;
}

//===========================================================================
static void addInPlaceSegs(SearchState * ss) {
    ss->node = nullptr;
    while (ss->klen - ss->kpos > kMaxSegLen) {
        StrTrieBase::Node tnode = {
            asSeg0(kMaxSegLen),
            (uint8_t) (ss->nNodes + 1)
        };
        ss->node = append(ss, tnode);
        ss->inode = (uint8_t) ss->nNodes++;
        for (auto spos = 0; spos < kMaxSegLen; ++spos, ++ss->kpos)
            pushSegVal(ss->node, spos, keyVal(ss->key, ss->kpos));
    }
    if (ss->kpos < ss->klen) {
        StrTrieBase::Node tnode = {
            asSeg0(ss->klen - ss->kpos),
            255
        };
        ss->node = append(ss, tnode);
        ss->inode = (uint8_t) ss->nNodes++;
        for (auto spos = 0; ss->kpos < ss->klen; ++spos, ++ss->kpos)
            pushSegVal(ss->node, spos, keyVal(ss->key, ss->kpos));
    }
    if (ss->node)
        ss->node->data[1] = 255;
}

//===========================================================================
static void addOverflowSegs(SearchState * ss) {
    for (; ss->kpos < ss->klen; ss->kpos += kMaxSegLen) {
        auto & tnode = ss->overflow.emplace_back(OverflowNode{kNodeSeg});
        ss->nNodes += 1;
        if (ss->klen - ss->kpos > kMaxSegLen) {
            tnode.seg.len = kMaxSegLen;
            tnode.seg.inext = ss->inode;
        } else {
            tnode.seg.len = (unsigned) (ss->klen - ss->kpos);
            tnode.seg.inext = 255;
        }
        for (auto spos = 0u; spos < tnode.seg.len; ++spos, ++ss->kpos)
            tnode.seg.data[spos] = keyVal(ss->key, ss->kpos);
    }
}

//===========================================================================
// Returns true if key was inserted
bool StrTrieBase::insert(string_view key) {
    auto ss = SearchState(key, &m_heap);
    ss.pgno = empty() ? m_heap.alloc() : m_heap.root();
    ss.nNodes = size(&ss);

    while (ss.inode < ss.nNodes) {
        setNode(&ss, ss.inode);
        switch (auto ntype = ::nodeType(ss.node)) {
        case kNodeSeg:
            if (!insertAtSeg(&ss))
                return !ss.found;   // return true if inserted
            break;
        case kNodeFork:
            if (!insertAtFork(&ss))
                return !ss.found;   // return true if inserted
            break;
        default:
            logMsgFatal() << "Invalid StrTrieBase node type: " << ntype;
        }
    }

    // add key segments
    if (ss.overflow.empty()) {
        addInPlaceSegs(&ss);
    } else {
        addOverflowSegs(&ss);
    }

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
    ss->inode = getFork(ss->node, ss->kval);
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
        setNode(ss, ss->inode);
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
    ss.pgno = m_heap.root();
    ss.nNodes = size(&ss);

    findLastFork(&ss);
    if (!ss.found)
        return false;

    setNode(&ss, ss.ifork);
    ss.kpos = ss.fpos;
    ss.inode = getFork(ss.node, ss.kval);
    setFork(ss.node, ss.kval, 0);
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
    ss.pgno = m_heap.root();

    for (;;) {
        setNode(&ss, ss.inode);
        auto ntype = ::nodeType(ss.node);
        if (ntype == kNodeFork) {
            if (ss.kpos == ss.klen) {
                ss.inode = ss.node->data[1];
                return ss.inode == 255;
            }
            ss.inode = getFork(ss.node, ss.kval);
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
    ss.pgno = m_heap.root();
    auto noEqual = false;

    while (ss.inode != 255) {
        setNode(&ss, ss.inode);
        auto ntype = ::nodeType(ss.node);
        if (ntype == kNodeFork) {
            if (ss.kpos == ss.klen) {
                ss.inode = ss.node->data[1];
                break;
            }
            ss.inode = getFork(ss.node, ss.kval);
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
    ss.pgno = m_heap.root();
    ss.nNodes = size(&ss);
    ss.inode = 0;
    for (; ss.inode < ss.nNodes; ++ss.inode) {
        setNode(&ss, ss.inode);
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
