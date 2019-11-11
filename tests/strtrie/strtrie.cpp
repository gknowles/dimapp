// Copyright Glen Knowles 2019.
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
//  data[1] = index of next node (always present)
//  data[2 - 17] = contiguous nibbles of key
// fork
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

unsigned const kNodeLen = 18;
unsigned const kMaxSegLen = 32;
unsigned const kMaxValueLen = 16;

enum NodeType : int8_t {
    kNodeInvalid,
    kNodeSeg,
    kNodeFork,
};

struct OverflowNode {
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

} // namespace


/****************************************************************************
*
*   StrTrieBase
*
***/

struct StrTrieBase::Node {
    uint8_t data[kNodeLen];
};

struct StrTrieBase::SearchState {
    string_view key;
    size_t pgno;
    size_t nNodes;
    int inode;
    StrTrieBase::Node * node;
    int kpos;
    size_t klen;
    uint8_t kval;
    vector<OverflowNode> overflow;
};

namespace Dim {

//===========================================================================
constexpr NodeType nodeType(StrTrieBase::Node const * node) {
    return NodeType(*node->data & 0x7);
}

} // namespace

//===========================================================================
static uint8_t getKeyVal(string_view key, size_t pos) {
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
static uint8_t getSegLen(StrTrieBase::Node const * node) {
    return (*node->data >> 3) + 1;
}

//===========================================================================
static uint8_t getSegVal(StrTrieBase::Node const * node, size_t pos) {
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
static size_t getSegNodesRequired (StrTrieBase::SearchState const & ss) {
    return (ss.klen - ss.kpos + kMaxSegLen - 1) / kMaxSegLen;
}

//===========================================================================
static uint8_t getFork(
    StrTrieBase::Node const * node,
    size_t pos
) {
    assert((node->data[0] & 0x7) == kNodeFork);
    assert(pos < 16);
    return node->data[pos + 2];
}

//===========================================================================
static size_t setFork(
    StrTrieBase::Node * node,
    uint8_t kval,
    size_t inode
) {
    auto tinode = (uint8_t) inode;
    assert(kval == 255 || kval < 16);
    assert(inode < 256);
    if (kval == 255) {
        node->data[1] = tinode;
    } else {
        node->data[kval + 2] = tinode;
    }
    return tinode;
}

//===========================================================================
StrTrieBase::Node * StrTrieBase::nodeAppend(
    size_t pgno,
    StrTrieBase::Node const * node
) {
    auto ptr = (Node *) m_heap.ptr(pgno);
    auto & last = ptr->data[m_heap.pageSize() - 1];
    if (last == capacity(pgno))
        return nullptr;
    auto inode = last++;
    auto out = ptr + inode;
    if (node) {
        memcpy(out, node, sizeof(*out));
    } else {
        assert(!*out->data);
    }
    return out;
}

//===========================================================================
StrTrieBase::Node * StrTrieBase::nodeAt(size_t pgno, size_t pos) {
    auto ptr = (Node *) m_heap.ptr(pgno);
    return ptr + pos;
}

//===========================================================================
StrTrieBase::Node const * StrTrieBase::nodeAt(
    size_t pgno,
    size_t pos
) const {
    return const_cast<StrTrieBase *>(this)->nodeAt(pgno, pos);
}

//===========================================================================
size_t StrTrieBase::size(size_t pgno) const {
    auto ptr = m_heap.ptr(pgno);
    return ptr[m_heap.pageSize() - 1];
}

//===========================================================================
size_t StrTrieBase::capacity(size_t pgno) const {
    return (m_heap.pageSize() - 1) / kNodeLen;
}

//===========================================================================
static void addNode (
    vector<OverflowNode> * out,
    StrTrieBase::Node const & node
) {
    switch (auto type = nodeType(&node)) {
    default:
        assert(!"unknown node type");
        break;
    case kNodeSeg: {
            auto & seg = out->emplace_back(OverflowNode{type}).seg;
            seg.inext = node.data[1];
            seg.len = getSegLen(&node);
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
    StrTrieBase::SearchState * ss
) {
    auto ptr = ss->node - ss->inode;
    auto last = ptr + ss->nNodes;
    for (; ptr <= last; ++ptr)
        addNode(out, *ptr);
}

//===========================================================================
bool StrTrieBase::splitSegInPlace (
    StrTrieBase::SearchState * ss,
    uint8_t spos,
    uint8_t slen,
    uint8_t sval
) {
    auto tinode = 0;
	if (spos == slen - 1) {
		tinode = ss->node->data[1];
	} else {
        // add tail segment
        auto tlen = slen - spos - 1;
        StrTrieBase::Node tnode = {
            asSeg0(tlen),
            ss->node->data[1]
        };
        for (auto tpos = 0; tpos < tlen; ++tpos) {
            auto sval = getSegVal(ss->node, tpos + spos + 1);
            pushSegVal(&tnode, tpos, sval);
        }
        nodeAppend(ss->pgno, &tnode);
        tinode = (uint8_t) ss->nNodes++;
        ss->node = nodeAt(ss->pgno, ss->inode);
    }
    if (!spos) {
        // only single element after detaching tail, so no lead segment
        // needed, convert to fork
        StrTrieBase::Node tnode = { kNodeFork };
        setFork(&tnode, sval, tinode);
        setFork(&tnode, ss->kval, ss->nNodes);
        memcpy(ss->node, &tnode, sizeof(*ss->node));
        ss->inode = (int) ss->nNodes;
    } else {
        // truncate segment as lead
        setSegLen(ss->node, spos);
        // point at fork that's being added
        ss->node->data[1] = (uint8_t) ss->nNodes;
        // add fork
        StrTrieBase::Node tnode = { kNodeFork };
        setFork(&tnode, sval, tinode);
        if (ss->kpos == ss->klen) {
            setFork(&tnode, ss->kval, 255);
            nodeAppend(ss->pgno, &tnode);
            return true;
        }
        setFork(&tnode, ss->kval, ss->nNodes + 1);
        nodeAppend(ss->pgno, &tnode);
        ss->inode = (int) ++ss->nNodes;
    }

    ss->kval = getKeyVal(ss->key, ++ss->kpos);
    return false;
}

//===========================================================================
bool StrTrieBase::splitSegToOverflow (
    StrTrieBase::SearchState * ss,
    uint8_t spos,
    uint8_t slen,
    uint8_t sval
) {
    assert(spos < slen);
    copyNodes(&ss->overflow, ss);

    auto inext = ss->node->data[1];
    auto & node = ss->overflow[ss->inode];
	if (spos < slen - 1) {
        // add tail segment
        auto & tnode = ss->overflow.emplace_back(OverflowNode{kNodeSeg});
        tnode.seg.len = slen - spos - 1;
        tnode.seg.inext = inext;
        for (unsigned tpos = 0; tpos < tnode.seg.len; ++tpos) {
            auto sval = getSegVal(ss->node, tpos + spos + 1);
            tnode.seg.data[tpos] = sval;
        }
        inext = (uint8_t) ss->nNodes++;
    }
    if (!spos) {
        // only single element after detaching tail, so no lead segment
        // needed, convert to fork
        assert(inext != 255);
        node.type = kNodeFork;
        node.fork.inext[sval] = inext;
        node.fork.inext[ss->kval] = (unsigned) ss->nNodes;
    } else {
        // truncate segment as lead
        node.seg.len = spos;
        // point at fork that's being added
        node.seg.inext = (unsigned) ss->nNodes;
        // add fork
        auto & tnode = ss->overflow.emplace_back(OverflowNode{kNodeFork});
        if (sval == 255) {
            tnode.fork.endOfKey = true;
        } else {
            tnode.fork.inext[sval] = inext;
        }
        if (ss->kpos == ss->klen) {
            tnode.fork.inext[ss->kval] = (unsigned) -1;
            return true;
        }
        tnode.fork.inext[ss->kval] = (unsigned) ss->nNodes++;
    }

    ss->inode = (int) ss->nNodes;
    ss->kval = getKeyVal(ss->key, ++ss->kpos);
    return false;
}

//===========================================================================
bool StrTrieBase::insertAtSeg (SearchState * ss) {
    auto spos = (uint8_t) 0; // split point where key diverges from segment
    auto slen = getSegLen(ss->node);
    auto sval = getSegVal(ss->node, spos);
    assert(slen > 1);
    for (;;) {
        if (ss->kpos == ss->klen)
            break;
        if (spos == slen) {
			// end of segment, still more key
            ss->inode = ss->node->data[1];
            return false;
        }
        if (ss->kval != sval)
            break;
        ss->kval = getKeyVal(ss->key, ++ss->kpos);
        sval = getSegVal(ss->node, ++spos);
    }

    // change segment to [lead segment] fork [tail segment]
    auto adds = (spos > 0) // needs lead segment
        + (spos < slen - 1); // needs tail segment
    if (capacity(ss->pgno) - size(ss->pgno)
        < getSegNodesRequired(*ss) + adds
    ) {
        return splitSegInPlace(ss, spos, slen, sval);
    }

    return splitSegToOverflow(ss, spos, slen, sval);
}

//===========================================================================
bool StrTrieBase::insertAtFork (SearchState * ss) {
    auto inext = getFork(ss->node, ss->kval);
    if (ss->kpos == ss->klen) {
        assert(ss->kval == 255);
        if (!inext) {
            setFork(ss->node, ss->kval, 255);
            return true;
        } else if (inext == 255) {
            return true;
        }
        return false;
    }

    if (!inext) {
        setFork(ss->node, ss->kval, ss->nNodes);
        ss->kval = getKeyVal(ss->key, ++ss->kpos);
        if (capacity(ss->pgno) - size(ss->pgno) < getSegNodesRequired(*ss))
            copyNodes(&ss->overflow, ss);
        ss->inode = (int) ss->nNodes;
    } else if (inext == 255) {
        setFork(ss->node, ss->kval, ss->nNodes);
        ss->kval = getKeyVal(ss->key, ++ss->kpos);

		Node tnode = { kNodeFork };
		setFork(&tnode, 255, 255);
        if (capacity(ss->pgno) - size(ss->pgno)
            < getSegNodesRequired(*ss) + 1
        ) {
            copyNodes(&ss->overflow, ss);
            addNode(&ss->overflow, tnode);
    		ss->inode = (int) ++ss->nNodes;
        } else {
            nodeAppend(ss->pgno, &tnode);
    		ss->inode = (int) ++ss->nNodes;
        }
    } else {
        ss->kval = getKeyVal(ss->key, ++ss->kpos);
    }

    return false;
}

//===========================================================================
void StrTrieBase::addInPlaceSegs(SearchState * ss) {
    ss->node = nullptr;
    while (ss->klen - ss->kpos > kMaxSegLen) {
        Node tnode = {
            asSeg0(kMaxSegLen),
            (uint8_t) (ss->nNodes + 1)
        };
        ss->node = nodeAppend(ss->pgno, &tnode);
        ss->inode = (uint8_t) ss->nNodes++;
        for (auto spos = 0; spos < kMaxSegLen; ++spos, ++ss->kpos)
            pushSegVal(ss->node, spos, getKeyVal(ss->key, ss->kpos));
    }
    if (ss->kpos < ss->klen) {
        Node tnode = {
            asSeg0(ss->klen - ss->kpos),
            255
        };
        ss->node = nodeAppend(ss->pgno, &tnode);
        ss->inode = (uint8_t) ss->nNodes++;
        for (auto spos = 0; ss->kpos < ss->klen; ++spos, ++ss->kpos)
            pushSegVal(ss->node, spos, getKeyVal(ss->key, ss->kpos));
    }
    if (ss->node)
        ss->node->data[1] = 255;
}

//===========================================================================
void StrTrieBase::addOverflowSegs(SearchState * ss) {
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
            tnode.seg.data[spos] = getKeyVal(ss->key, ss->kpos);
    }
}

//===========================================================================
bool StrTrieBase::insert(string_view srckey) {
    SearchState ss = {};
    ss.key = srckey;
    ss.pgno = empty() ? m_heap.alloc() : m_heap.root();
    ss.nNodes = size(ss.pgno);
    ss.inode = 0;
    ss.node = nullptr;
    ss.kpos = 0; // nibble of key being processed
    ss.klen = ss.key.size() * 2;
    ss.kval = getKeyVal(ss.key, ss.kpos);

    while (ss.inode < ss.nNodes) {
        ss.node = nodeAt(ss.pgno, ss.inode);
        auto ntype = nodeType(ss.node);
        switch (ntype) {
        case kNodeSeg:
            if (insertAtSeg(&ss))
                return true;
            break;
        case kNodeFork:
            if (insertAtFork(&ss))
                return true;
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

    return true;
}

//===========================================================================
bool StrTrieBase::contains(std::string_view key) const {
    string out;
    return lowerBound(&out, key) && out == key;
}

//===========================================================================
bool StrTrieBase::lowerBound(string * out, std::string_view key) const {
    out->clear();
    if (empty())
        return false;
    auto kpos = 0;
    auto klen = key.size() * 2;
    auto kval = getKeyVal(key, kpos);
    auto pgno = m_heap.root();
    auto inode = 0;
    auto node = (Node const *) nullptr;

    while (inode != 255) {
        node = nodeAt(pgno, inode);
        auto ntype = nodeType(node);
        if (ntype == kNodeFork) {
            if (kpos == klen) {
                inode = node->data[1];
                break;
            }
            inode = getFork(node, kval);
            kval = getKeyVal(key, ++kpos);
            continue;
        }

        if (ntype == kNodeSeg) {
            auto slen = getSegLen(node);
            if (klen - kpos < slen)
                return false;
            for (auto spos = 0; spos < slen; ++spos) {
                auto sval = getSegVal(node, spos);
                if (sval != kval)
                    return false;
                kval = getKeyVal(key, ++kpos);
            }
            inode = node->data[1];
            if (kpos == klen)
                break;
        }
    }

    *out = key;
    return inode == 255 && kpos == klen;
}

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
    os << "---\n";
    if (empty())
        return os;

    string out;
    auto pgno = m_heap.root();
    auto nNodes = size(pgno);
    for (auto inode = 0; inode < nNodes; ++inode) {
        auto node = nodeAt(pgno, inode);
        os << inode << ": ";
        for (auto i = 0; i < kNodeLen; ++i) {
            if (i % 4 == 2) os.put(' ');
            hexByte(os, node->data[i]);
        }

        switch (auto ntype = nodeType(node)) {
        case kNodeInvalid:
            os << " -\n";
            break;
        case kNodeFork:
            os << " - Fork\n";
            break;
        case kNodeSeg:
            os << " - Segment/" << (int) getSegLen(node) << "\n";
            break;
        default:
            os << " - UNKNOWN(" << ntype << ")\n";
            break;
        }
    }
    return os;
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
