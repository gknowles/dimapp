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

namespace {

enum NodeType {
    kNodeInvalid,
    kNodeSeg,
    kNodeSwitch,
};

unsigned const kNodeLen = 18;
unsigned const kMaxSegLen = 32;
unsigned const kMaxValueLen = 16;

// Segment
//  data[0] & 0x7 = kNodeSeg
//  (data[0] >> 3) + 1 = length in nibbles [1, 32]
//  data[1] = index of next node (always present)
//  data[2 - 17] = contiguous nibbles of key
// Switch
//  data[0] & 0x7 = kNodeSwitch
//  data[1] = index of value node (0 for none)
//  data[2 - 17] = index by next nibble to get next node for key (0 for none)

} // namespace


/****************************************************************************
*
*   StrTrieBase
*
***/

//===========================================================================
NodeType nodeType(uint8_t const * node) {
    return NodeType(*node & 0x7);
}

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
static void setSegLen(uint8_t * node, size_t len) {
    node[0] = asSeg0(len);
}

//===========================================================================
static uint8_t getSegLen(uint8_t const * node) {
    return (*node >> 3) + 1;
}

//===========================================================================
static uint8_t getSegVal(uint8_t const * node, size_t pos) {
    assert((node[0] & 0x7) == kNodeSeg);
    return pos % 2 == 0
        ? (node[2 + pos / 2] >> 4)
        : (node[2 + pos / 2] & 0xf);
}

//===========================================================================
static uint8_t pushSegVal(
    uint8_t * node,
    size_t pos,
    uint8_t val
) {
    if (pos % 2 == 0) {
        node[2 + pos / 2] = val << 4;
    } else {
        node[2 + pos / 2] |= val;
    }
    return val;
}

//===========================================================================
static size_t setSwitchVal(
    uint8_t * node,
    uint8_t kval,
    size_t inode
) {
    auto tinode = (uint8_t) inode;
    assert(kval == 255 || kval < 16);
    assert(inode < 256);
    if (kval == 255) {
        node[1] = tinode;
    } else {
        node[kval + 2] = tinode;
    }
    return tinode;
}

//===========================================================================
uint8_t * StrTrieBase::nodeAppend(size_t pgno, uint8_t const * node) {
    auto ptr = m_heap.ptr(pgno);
    auto & last = ptr[m_heap.pageSize() - 1];
    if (last == capacity(pgno))
        return nullptr;
    auto inode = last++;
    auto out = ptr + inode * kNodeLen;
    if (node) {
        memcpy(out, node, kNodeLen);
    } else {
        assert(!*out);
    }
    return out;
}

//===========================================================================
uint8_t * StrTrieBase::nodeAt(size_t pgno, size_t pos) {
    auto ptr = m_heap.ptr(pgno);
    return ptr + pos * kNodeLen;
}

//===========================================================================
uint8_t const * StrTrieBase::nodeAt(size_t pgno, size_t pos) const {
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
bool StrTrieBase::insert(string_view key) {
    auto pgno = empty() ? m_heap.alloc() : m_heap.root();
    auto nNodes = size(pgno);
    auto inode = 0;
    auto node = (uint8_t *) nullptr;
    auto kpos = 0; // nibble of key being processed
    auto klen = key.size() * 2;
    auto kval = getKeyVal(key, kpos);
    while (inode < nNodes) {
        node = nodeAt(pgno, inode);
        auto ntype = nodeType(node);
        if (ntype == kNodeSeg) {
            auto spos = 0; // split point where key diverges from segment
            auto sval = getSegVal(node, spos);
            auto slen = getSegLen(node);
            assert(slen > 1);
            while (kpos < klen) {
                if (spos == slen) {
                    inode = node[1];
                    goto NEXT_NODE;
                }
                if (kval != sval)
                    break;
                kval = getKeyVal(key, ++kpos);
                sval = getSegVal(node, ++spos);
            }

            // change segment to [lead segment] switch [tail segment]
            auto tinode = 0;
            if (spos != slen - 1) {
                // add tail segment
                auto tlen = slen - spos - 1;
                uint8_t tnode[kNodeLen] = {
                    asSeg0(tlen),
                    node[1]
                };
                for (auto tpos = 0; tpos < tlen; ++tpos) {
                    auto sval = getSegVal(node, tpos + spos + 1);
                    pushSegVal(tnode, tpos, sval);
                }
                nodeAppend(pgno, tnode);
                tinode = (uint8_t) nNodes++;
                node = nodeAt(pgno, inode);
            }
            if (!spos) {
                // convert segment to switch
                uint8_t tnode[kNodeLen] = { kNodeSwitch };
                setSwitchVal(tnode, sval, tinode);
                setSwitchVal(tnode, kval, nNodes);
                memcpy(node, tnode, kNodeLen);
                inode = (int) nNodes;
                goto NEXT_NODE;
            }
            // truncate segment as lead
            setSegLen(node, spos);
            node[1] = (uint8_t) nNodes; // point at switch that's being added
            // add switch
            uint8_t tnode[kNodeLen] = { kNodeSwitch };
            setSwitchVal(tnode, sval, tinode);
            setSwitchVal(tnode, kval, nNodes + 1);
            node = nodeAppend(pgno, tnode);
            inode = (uint8_t) ++nNodes;
            goto NEXT_NODE;
        }
        if (ntype == kNodeSwitch) {
            inode = node[kval + 2];
            if (!inode) {
                setSwitchVal(node, kval, nNodes);
                inode = (int) nNodes;
            }
            goto NEXT_NODE;
        }
        logMsgFatal() << "Invalid StrTrieBase node type: " << ntype;

    NEXT_NODE:
        if (kpos == klen)
            return false;
        kval = getKeyVal(key, ++kpos);
    }
    // add key segments
    node = (uint8_t *) nullptr;
    while (klen - kpos > kMaxSegLen) {
        uint8_t tnode[kNodeLen] = { 
            asSeg0(kMaxSegLen), 
            (uint8_t) (nNodes + 1)
        };
        node = nodeAppend(pgno, tnode);
        inode = (uint8_t) nNodes++;
        for (auto spos = 0; spos < kMaxSegLen; ++spos, ++kpos)
            pushSegVal(node, spos, getKeyVal(key, kpos));
    }
    if (kpos < klen) {
        uint8_t tnode[kNodeLen] = { 
            asSeg0(klen - kpos), 
            (uint8_t) (nNodes + 1)
        };
        node = nodeAppend(pgno, tnode);
        inode = (uint8_t) nNodes++;
        for (auto spos = 0; kpos < klen; ++spos, ++kpos) 
            pushSegVal(node, spos, getKeyVal(key, kpos));
    }
    if (node) 
        node[1] = 255;
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
    auto node = (uint8_t const *) nullptr;

    while (inode != 255) {
        node = nodeAt(pgno, inode);
        auto ntype = nodeType(node);
        if (ntype == kNodeSwitch) {
            if (kpos == klen) {
                inode = node[1];
                break;
            }
            inode = node[kval + 2];
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
            inode = node[1];
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
            hexByte(os, node[i]);
        }

        switch (auto ntype = nodeType(node)) {
        case kNodeInvalid:
            os << " -\n";
            break;
        case kNodeSwitch:
            os << " - Switch\n";
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
