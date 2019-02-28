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
    kNodeValue,
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
// Value
//  data[0] & 0x7 = kNodeValue
//  (data[0] >> 3) + 1 = length in bytes of value [1, 16]
//  data[1] = index of next node (0 for none)
//  data[2 - 17] = bytes of value

} // namespace


/****************************************************************************
*
*   StrTrie
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
static void setValueLen(uint8_t * node, size_t len) {
    node[0] = (uint8_t) (kNodeValue | ((len - 1) << 3));
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
bool StrTrie::pageEmpty() const {
    return m_pages.empty();
}

//===========================================================================
size_t StrTrie::pageRoot() const {
    return 0;
}

//===========================================================================
size_t StrTrie::pageNew() {
    m_pages.push_back(make_unique<uint8_t[]>(kPageSize));
    return m_pages.size() - 1;
}

//===========================================================================
uint8_t * StrTrie::nodeAppend(size_t pgno, uint8_t const * node) {
    auto ptr = m_pages[pgno].get();
    if (ptr[kPageSize - 1] == capacity(pgno))
        return nullptr;
    auto inode = ptr[kPageSize - 1]++;
    auto out = ptr + inode * kNodeLen;
    if (node) {
        memcpy(out, node, kNodeLen);
    } else {
        assert(!*out);
    }
    return out;
}

//===========================================================================
uint8_t * StrTrie::nodeAt(size_t pgno, size_t pos) {
    auto ptr = m_pages[pgno].get();
    return ptr + pos * kNodeLen;
}

//===========================================================================
uint8_t const * StrTrie::nodeAt(size_t pgno, size_t pos) const {
    return const_cast<StrTrie *>(this)->nodeAt(pgno, pos);
}

//===========================================================================
size_t StrTrie::size(size_t pgno) const {
    auto ptr = m_pages[pgno].get();
    return ptr[kPageSize - 1];
}

//===========================================================================
size_t StrTrie::capacity(size_t pgno) const {
    return (kPageSize - 1) / kNodeLen;
}

//===========================================================================
bool StrTrie::insert(string_view key, string_view value) {
    bool inserted = true;
    auto pgno = empty() ? pageNew() : pageRoot();
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
        logMsgFatal() << "Invalid StrTrie node type: " << ntype;

    NEXT_NODE:
        if (kpos == klen)
            goto START_VALUE;
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
    inode = (uint8_t) nNodes;

START_VALUE:
    auto vpos = (size_t) 0;
    auto vlen = value.size();
    while (inode < nNodes) {
        // replace
        auto slen = min(vlen - vpos, (size_t) kMaxValueLen);
        setSegLen(node, slen);
        memcpy(node + 2, value.data() + vpos, slen);
        vpos += slen;
        inode = node[1];
        node = nodeAt(pgno, inode);
        if (vpos == vlen) {
            node[1] = 0;
            goto REMOVE_TRAILING_VALUE_NODES;
        }
        if (!inode) {
            inode = node[1] = (uint8_t) nNodes;
            node = nodeAt(pgno, inode);
            inserted = false;
            break;
        }
    }
    for (;;) {
        assert(inode == size(pgno));
        node = nodeAppend(pgno, nullptr);
        nNodes += 1;
        auto slen = min(vlen - vpos, (size_t) kMaxValueLen);
        setValueLen(node, slen);
        memcpy(node + 2, value.data() + vpos, slen);
        vpos += slen;
        if (vpos == vlen)
            return inserted;
        inode = node[1] = (uint8_t) nNodes;
        node = nodeAt(pgno, inode);
    }

REMOVE_TRAILING_VALUE_NODES:
    while (inode) {
        node = nodeAt(pgno, inode);
        inode = node[1];
        memset(node, 0, kNodeLen);
    }
    return false;
}

//===========================================================================
bool StrTrie::find(string * out, std::string_view key) const {
    out->clear();
    if (empty())
        return false;
    auto kpos = 0;
    auto klen = key.size() * 2;
    auto kval = getKeyVal(key, kpos);
    auto pgno = pageRoot();
    auto inode = 0;
    auto node = (uint8_t const *) nullptr;

    for (;;) {
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

        if (ntype == kNodeValue)
            return false;

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

    if (!inode)
        return false;
    node = nodeAt(pgno, inode);
    auto ntype = nodeType(node);
    if (ntype == kNodeSwitch) {
        inode = node[1];
        node = nodeAt(pgno, inode);
        ntype = nodeType(node);
    }
    if (ntype != kNodeValue)
        return false;
    for (;;) {
        out->append((char const *) node + 2, getSegLen(node));
        inode = node[1];
        if (!inode)
            return true;
        node = nodeAt(pgno, inode);
        assert(nodeType(node) == kNodeValue);
    }
}

//===========================================================================
StrTrie::Iterator StrTrie::begin() const {
    return Iterator{};
}

//===========================================================================
StrTrie::Iterator StrTrie::end() const {
    return Iterator{};
}

//===========================================================================
ostream & StrTrie::dump(ostream & os) const {
    os << "---\n";
    if (empty())
        return os;

    string out;
    auto pgno = pageRoot();
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
        case kNodeValue:
            os << " - Value/" << (int) getSegLen(node) << " \"";
            for (auto i = 0; i < getSegLen(node); ++i) {
                auto ch = node[i + 2];
                if (isprint(ch)) {
                    os << ch;
                } else {
                    os << '%';
                    hexByte(os, ch);
                }
            }
            os << "\"\n";
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
*   StrTrie::Iterator
*
***/

//===========================================================================
StrTrie::Iterator & StrTrie::Iterator::operator++() {
    return *this;
}
