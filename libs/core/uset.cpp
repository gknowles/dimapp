// Copyright Glen Knowles 2017 - 2018.
// Distributed under the Boost Software License, Version 1.0.
//
// uset.cpp - dim core
#include "pch.h"
#pragma hdrstop

using namespace std;
using namespace Dim;
using Node = UnsignedSet::Node;


/****************************************************************************
*
*   Declarations
*
***/

unsigned const kDataSize = 512;
static_assert(hammingWeight(kDataSize) == 1);
static_assert(kDataSize >= 256);

unsigned const kBitWidth = 32;

unsigned const kLeafBits = hammingWeight(8 * kDataSize - 1);
unsigned const kStepBits =
    hammingWeight(pow2Ceil(kDataSize / sizeof Node + 1) / 2 - 1);
unsigned const kMaxDepth =
    (kBitWidth - kLeafBits + kStepBits - 1) / kStepBits;

constexpr unsigned maxDepth() {
    return kMaxDepth;
}

constexpr unsigned valueMask(unsigned depth) {
    assert(depth <= kMaxDepth);
    unsigned bits = kLeafBits + kStepBits * (kMaxDepth - depth);
    if (bits >= kBitWidth)
        return numeric_limits<unsigned>::max();
    return (1 << bits) - 1;
}

constexpr uint16_t numNodes(unsigned depth) {
    uint16_t ret = 0;
    if (depth) {
        ret = (uint16_t) 1
            << hammingWeight(valueMask(depth) ^ valueMask(depth - 1));
    }
    return ret;
}

constexpr unsigned relBase(unsigned value, unsigned depth) {
    return (value & ~valueMask(depth)) >> 8;
}
constexpr unsigned relValue(unsigned value, unsigned depth) {
    return value & valueMask(depth);
}
constexpr unsigned absBase(Node const & node) {
    return node.base << 8;
}
constexpr unsigned absSize(Node const & node) {
    return valueMask(node.depth) + 1;
}

namespace {

enum NodeType {
    kEmpty,     // contains no values
    kFull,      // contains all values in node's domain
    kVector,    // has vector of values
    kBitmap,    // has bitmap of values
    kMeta,      // vector of nodes
    kNodeTypes,
    kMetaEnd,   // marks end of node vector
};

struct IImplBase {
    using value_type = unsigned;

    // Requires that type, base, and depth are set, but otherwise treats
    // the node as uninitialized.
    virtual void init(Node & node, bool full) = 0;

    // Requires that type is set, but otherwise treats the target node as
    // uninitialized.
    virtual void init(Node & node, Node const & from) = 0;

    virtual void destroy(Node & node) = 0;
    virtual bool insert(Node & node, unsigned value) = 0;
    virtual void insert(
        Node & node,
        unsigned const * first,
        unsigned const * last
    ) = 0;
    virtual bool erase(Node & node, unsigned value) = 0;

    virtual size_t size(Node const & node) const = 0;
    virtual bool findFirst(
        Node const ** onode,
        unsigned * ovalue,
        Node const & node,
        unsigned first
    ) const = 0;

    // [first] is empty - return true
    // [first, end of node] is full - update onode & ovalue, return false
    // otherwise - update onode & ovalue, return true
    virtual bool lastContiguous(
        Node const ** onode,
        unsigned * ovalue,
        Node const & node,
        unsigned first
    ) const = 0;
};

struct EmptyImpl final : IImplBase {
    void init(Node & node, bool full) override;
    void init(Node & node, Node const & from) override;
    void destroy(Node & node) override;
    bool insert(Node & node, unsigned value) override;
    void insert(
        Node & node,
        unsigned const * first,
        unsigned const * last
    ) override;
    bool erase(Node & node, unsigned value) override;

    size_t size(Node const & node) const override;
    bool findFirst(
        Node const ** onode,
        unsigned * ovalue,
        Node const & node,
        unsigned first
    ) const override;
    bool lastContiguous(
        Node const ** onode,
        unsigned * ovalue,
        Node const & node,
        unsigned first
    ) const override;
};

struct FullImpl final : IImplBase {
    void init(Node & node, bool full) override;
    void init(Node & node, Node const & from) override;
    void destroy(Node & node) override;
    bool insert(Node & node, unsigned value) override;
    void insert(
        Node & node,
        unsigned const * first,
        unsigned const * last
    ) override;
    bool erase(Node & node, unsigned value) override;

    size_t size(Node const & node) const override;
    bool findFirst(
        Node const ** onode,
        unsigned * ovalue,
        Node const & node,
        unsigned first
    ) const override;
    bool lastContiguous(
        Node const ** onode,
        unsigned * ovalue,
        Node const & node,
        unsigned first
    ) const override;
};

struct VectorImpl final : IImplBase {
    static constexpr size_t maxValues() {
        return kDataSize / sizeof *Node::values;
    }

    void init(Node & node, bool full) override;
    void init(Node & node, Node const & from) override;
    void destroy(Node & node) override;
    bool insert(Node & node, unsigned value) override;
    void insert(
        Node & node,
        unsigned const * first,
        unsigned const * last
    ) override;
    bool erase(Node & node, unsigned value) override;

    size_t size(Node const & node) const override;
    bool findFirst(
        Node const ** onode,
        unsigned * ovalue,
        Node const & node,
        unsigned first
    ) const override;
    bool lastContiguous(
        Node const ** onode,
        unsigned * ovalue,
        Node const & node,
        unsigned first
    ) const override;

private:
    void convertAndInsert(Node & node, unsigned value);
};

struct BitmapImpl final : IImplBase {
    static constexpr size_t numInt64s() {
        return kDataSize / sizeof uint64_t;
    }
    static constexpr size_t numBits() { return 64 * numInt64s(); }

    void init(Node & node, bool full) override;
    void init(Node & node, Node const & from) override;
    void destroy(Node & node) override;
    bool insert(Node & node, unsigned value) override;
    void insert(
        Node & node,
        unsigned const * first,
        unsigned const * last
    ) override;
    bool erase(Node & node, unsigned value) override;

    size_t size(Node const & node) const override;
    bool findFirst(
        Node const ** onode,
        unsigned * ovalue,
        Node const & node,
        unsigned first
    ) const override;
    bool lastContiguous(
        Node const ** onode,
        unsigned * ovalue,
        Node const & node,
        unsigned first
    ) const override;
};

struct MetaImpl final : IImplBase {
    static constexpr size_t maxNodes() {
        return kDataSize / sizeof *Node::nodes;
    }

    void init(Node & node, bool full) override;
    void init(Node & node, Node const & from) override;
    void destroy(Node & node) override;
    bool insert(Node & node, unsigned value) override;
    void insert(
        Node & node,
        unsigned const * first,
        unsigned const * last
    ) override;
    bool erase(Node & node, unsigned value) override;

    size_t size(Node const & node) const override;
    bool findFirst(
        Node const ** onode,
        unsigned * ovalue,
        Node const & node,
        unsigned first
    ) const override;
    bool lastContiguous(
        Node const ** onode,
        unsigned * ovalue,
        Node const & node,
        unsigned first
    ) const override;

private:
    unsigned nodePos(Node const & node, unsigned value) const;
};

} // namespace


/****************************************************************************
*
*   Helpers
*
***/

static EmptyImpl s_emptyImpl;
static FullImpl s_fullImpl;
static VectorImpl s_vectorImpl;
static BitmapImpl s_bitmapImpl;
static MetaImpl s_metaImpl;

static IImplBase * s_impls[] = {
    &s_emptyImpl,
    &s_fullImpl,
    &s_vectorImpl,
    &s_bitmapImpl,
    &s_metaImpl,
};
static_assert(size(s_impls) == kNodeTypes);

//===========================================================================
static IImplBase * impl(Node const & node) {
    assert(node.type < size(s_impls));
    return s_impls[node.type];
}


/****************************************************************************
*
*   EmptyImpl
*
***/

//===========================================================================
void EmptyImpl::init(Node & node, bool full) {
    assert(node.type == kEmpty);
    assert(!full);
    node.numBytes = 0;
    node.numValues = 0;
    node.values = nullptr;
}

//===========================================================================
void EmptyImpl::init(Node & node, Node const & from) {
    assert(node.type == kEmpty);
    assert(from.type == kEmpty);
    node = from;
}

//===========================================================================
void EmptyImpl::destroy(Node & node)
{}

//===========================================================================
bool EmptyImpl::insert(Node & node, unsigned value) {
    assert(node.type == kEmpty);
    assert(relBase(value, node.depth) == node.base);
    destroy(node);
    node.type = kVector;
    s_vectorImpl.init(node, false);
    s_vectorImpl.insert(node, value);
    return true;
}

//===========================================================================
void EmptyImpl::insert(
    Node & node,
    unsigned const * first,
    unsigned const * last
) {
    if (first != last) {
        insert(node, *first++);
        if (first != last)
            s_vectorImpl.insert(node, first, last);
    }
}

//===========================================================================
bool EmptyImpl::erase(Node & node, unsigned value) {
    assert(relBase(value, node.depth) == node.base);
    return false;
}

//===========================================================================
size_t EmptyImpl::size(Node const & node) const {
    return 0;
}

//===========================================================================
bool EmptyImpl::findFirst(
    Node const ** onode,
    unsigned * ovalue,
    Node const & node,
    unsigned first
) const {
    return false;
}

//===========================================================================
bool EmptyImpl::lastContiguous(
    Node const ** onode,
    unsigned * ovalue,
    Node const & node,
    unsigned first
) const {
    return true;
}


/****************************************************************************
*
*   FullImpl
*
***/

//===========================================================================
void FullImpl::init(Node & node, bool full) {
    assert(node.type == kFull);
    assert(full);
    node.numBytes = 0;
    node.numValues = 0;
    node.values = nullptr;
}

//===========================================================================
void FullImpl::init(Node & node, Node const & from) {
    assert(node.type == kFull);
    assert(from.type == kFull);
    node = from;
}

//===========================================================================
void FullImpl::destroy(Node & node)
{}

//===========================================================================
bool FullImpl::insert(Node & node, unsigned value) {
    assert(relBase(value, node.depth) == node.base);
    return false;
}

//===========================================================================
void FullImpl::insert(
    Node & node,
    unsigned const * first,
    unsigned const * last
)
{}

//===========================================================================
bool FullImpl::erase(Node & node, unsigned value) {
    assert(relBase(value, node.depth) == node.base);
    destroy(node);
    if (node.depth == maxDepth()) {
        node.type = kBitmap;
        s_bitmapImpl.init(node, true);
        return s_bitmapImpl.erase(node, value);
    } else {
        node.type = kMeta;
        s_metaImpl.init(node, true);
        return s_metaImpl.erase(node, value);
    }
}

//===========================================================================
size_t FullImpl::size(Node const & node) const {
    return valueMask(node.depth) + 1;
}

//===========================================================================
bool FullImpl::findFirst(
    Node const ** onode,
    unsigned * ovalue,
    Node const & node,
    unsigned first
) const {
    assert(relBase(first, node.depth) == node.base);
    *onode = &node;
    *ovalue = first;
    return true;
}

//===========================================================================
bool FullImpl::lastContiguous(
    Node const ** onode,
    unsigned * ovalue,
    Node const & node,
    unsigned first
) const {
    assert(relBase(first, node.depth) == node.base);
    *onode = &node;
    *ovalue = absBase(node) + valueMask(node.depth);
    return false;
}


/****************************************************************************
*
*   VectorImpl
*
***/

//===========================================================================
void VectorImpl::init(Node & node, bool full) {
    assert(node.type == kVector);
    assert(!full);
    node.numBytes = kDataSize;
    node.numValues = 0;
    node.values = (unsigned *) malloc(node.numBytes);
}

//===========================================================================
void VectorImpl::init(Node & node, Node const & from) {
    assert(node.type == kVector);
    assert(from.type == kVector);
    node = from;
    node.values = (unsigned *) malloc(node.numBytes);
    assert(node.values);
    memcpy(node.values, from.values, node.numBytes);
}

//===========================================================================
void VectorImpl::destroy(Node & node) {
    assert(node.values);
    free(node.values);
}

//===========================================================================
bool VectorImpl::insert(Node & node, unsigned value) {
    assert(relBase(value, node.depth) == node.base);
    auto last = node.values + node.numValues;
    auto ptr = lower_bound(node.values, last, value);
    if (ptr == last) {
        if (node.numValues == maxValues()) {
            convertAndInsert(node, value);
        } else {
            *ptr = value;
            node.numValues += 1;
        }
        return true;
    }
    if (*ptr == value)
        return false;
    if (node.numValues == maxValues()) {
        convertAndInsert(node, value);
    } else {
        memmove(ptr + 1, ptr, sizeof *ptr * (last - ptr));
        *ptr = value;
        node.numValues += 1;
    }
    return true;
}

//===========================================================================
void VectorImpl::insert(
    Node & node,
    unsigned const * first,
    unsigned const * last
) {
    while (first != last) {
        if (node.type != kVector)
            return impl(node)->insert(node, first, last);
        insert(node, *first++);
    }
}

//===========================================================================
bool VectorImpl::erase(Node & node, unsigned value) {
    assert(relBase(value, node.depth) == node.base);
    auto last = node.values + node.numValues;
    auto ptr = lower_bound(node.values, last, value);
    if (ptr == last || *ptr != value)
        return false;

    if (--node.numValues) {
        // Still has values, shift remaining ones down
        memmove(ptr, ptr + 1, sizeof *ptr * (last - ptr - 1));
    } else {
        // No more values, convert to empty node.
        destroy(node);
        node.type = kEmpty;
        s_emptyImpl.init(node, false);
    }
    return true;
}

//===========================================================================
size_t VectorImpl::size(Node const & node) const {
    return node.numValues;
}

//===========================================================================
bool VectorImpl::findFirst(
    Node const ** onode,
    unsigned * ovalue,
    Node const & node,
    unsigned first
) const {
    assert(relBase(first, node.depth) == node.base);
    auto last = node.values + node.numValues;
    auto ptr = lower_bound(node.values, last, first);
    if (ptr != last) {
        *onode = &node;
        *ovalue = *ptr;
        return true;
    }
    return false;
}

//===========================================================================
bool VectorImpl::lastContiguous(
    Node const ** onode,
    unsigned * ovalue,
    Node const & node,
    unsigned first
) const {
    assert(relBase(first, node.depth) == node.base);
    auto last = node.values + node.numValues;
    auto ptr = lower_bound(node.values, last, first);
    auto val = first;
    while (ptr != last) {
        if (*ptr != val) {
            if (val != first) {
                *onode = &node;
                *ovalue = val - 1;
            }
            return true;
        }
        val += 1;
        ptr += 1;
    }
    *onode = &node;
    if (val && val <= absBase(node) + valueMask(node.depth)) {
        *ovalue = val - 1;
        return true;
    }
    // [first, end of node] is full
    *ovalue = absBase(node) + valueMask(node.depth);
    return false;
}

//===========================================================================
void VectorImpl::convertAndInsert(Node & node, unsigned value) {
    Node tmp{node};
    auto ptr = tmp.values;
    auto last = ptr + tmp.numValues;
    if (tmp.depth == maxDepth()) {
        node.type = kBitmap;
        s_bitmapImpl.init(node, false);
        s_bitmapImpl.insert(node, ptr, last);
        s_bitmapImpl.insert(node, value);
    } else {
        node.type = kMeta;
        s_metaImpl.init(node, false);
        s_metaImpl.insert(node, ptr, last);
        s_metaImpl.insert(node, value);
    }
    destroy(tmp);
}


/****************************************************************************
*
*   BitmapImpl
*
***/

//===========================================================================
void BitmapImpl::init(Node & node, bool full) {
    assert(node.type == kBitmap);
    node.numBytes = kDataSize;
    node.values = (unsigned *) malloc(node.numBytes);
    assert(node.values);
    if (full) {
        node.numValues = kDataSize / sizeof uint64_t;
        memset(node.values, 0xff, kDataSize);
    } else {
        node.numValues = 0;
        memset(node.values, 0, kDataSize);
    }
}

//===========================================================================
void BitmapImpl::init(Node & node, Node const & from) {
    assert(node.type == kBitmap);
    assert(from.type == kBitmap);
    node = from;
    node.values = (unsigned *) malloc(node.numBytes);
    assert(node.values);
    memcpy(node.values, from.values, node.numBytes);
}

//===========================================================================
void BitmapImpl::destroy(Node & node) {
    assert(node.values);
    free(node.values);
}

//===========================================================================
bool BitmapImpl::insert(Node & node, unsigned value) {
    assert(node.type == kBitmap);
    assert(relBase(value, node.depth) == node.base);
    auto base = (uint64_t *) node.values;
    value = relValue(value, node.depth);
    BitView bits{base, numInt64s()};
    auto tmp = bits.word(value);
    if (!tmp) {
        bits.set(value);
        node.numValues += 1;
        return true;
    }
    if (bits.testAndSet(value))
        return false;

    if (tmp != 0xffff'ffff'ffff'ffff || node.numValues != numInt64s())
        return true;
    for (unsigned i = 0; i < numInt64s(); ++i) {
        if (base[i] != 0xffff'ffff'ffff'ffff)
            return true;
    }
    destroy(node);
    node.type = kFull;
    s_fullImpl.init(node, true);
    return true;
}

//===========================================================================
void BitmapImpl::insert(
    Node & node,
    unsigned const * first,
    unsigned const * last
) {
    for (; first != last; ++first)
        insert(node, *first);
}

//===========================================================================
bool BitmapImpl::erase(Node & node, unsigned value) {
    assert(relBase(value, node.depth) == node.base);
    auto base = (uint64_t *) node.values;
    value = relValue(value, node.depth);
    BitView bits{base, numInt64s()};
    if (!bits.testAndReset(value))
        return false;
    if (bits.word(value) || --node.numValues)
        return true;

    // convert to empty
    destroy(node);
    node.type = kEmpty;
    s_emptyImpl.init(node, false);
    return true;
}

//===========================================================================
size_t BitmapImpl::size(Node const & node) const {
    auto base = (uint64_t *) node.values;
    BitView bits{base, numInt64s()};
    return bits.count();
}

//===========================================================================
bool BitmapImpl::findFirst(
    Node const ** onode,
    unsigned * ovalue,
    Node const & node,
    unsigned first
) const {
    assert(relBase(first, node.depth) == node.base);
    auto base = (uint64_t *) node.values;
    auto rel = relValue(first, node.depth);
    assert(rel < numBits());
    BitView bits{base, numInt64s()};
    if (auto pos = bits.find(rel); pos != bits.npos) {
        *onode = &node;
        *ovalue = (unsigned) pos + absBase(node);
        return true;
    }
    return false;
}

//===========================================================================
bool BitmapImpl::lastContiguous(
    Node const ** onode,
    unsigned * ovalue,
    Node const & node,
    unsigned first
) const {
    assert(relBase(first, node.depth) == node.base);
    auto base = (uint64_t *) node.values;
    auto rel = relValue(first, node.depth);
    assert(rel < numBits());
    BitView bits{base, numInt64s()};
    if (auto pos = bits.findZero(rel); pos != bits.npos) {
        if (auto val = (unsigned) pos + absBase(node); val != first) {
            *onode = &node;
            *ovalue = val - 1;
        }
        return true;
    }
    *onode = &node;
    *ovalue = absBase(node) + valueMask(node.depth);
    return false;
}


/****************************************************************************
*
*   MetaImpl
*
***/

//===========================================================================
void MetaImpl::init(Node & node, bool full) {
    assert(node.type == kMeta);
    node.numValues = numNodes(node.depth + 1);
    node.numBytes = (node.numValues + 1) * sizeof *node.nodes;
    node.nodes = (Node *) malloc(node.numBytes);
    assert(node.nodes);
    auto nptr = node.nodes;
    auto nlast = node.nodes + node.numValues;
    Node def;
    def.type = full ? kFull : kEmpty;
    def.depth = node.depth + 1;
    def.base = node.base;
    def.numBytes = 0;
    def.numValues = 0;
    def.values = nullptr;
    auto domain = absSize(def) >> 8;
    for (; nptr != nlast; ++nptr, def.base += domain)
        *nptr = def;

    // Internally the array of nodes contains a trailing "node" at the end that
    // is a pointer to the parent node.
    *nlast = {};
    nlast->type = kMetaEnd;
    nlast->nodes = &node;
}

//===========================================================================
void MetaImpl::init(Node & node, Node const & from) {
    assert(node.type == kMeta);
    assert(from.type == kMeta);
    node = from;
    node.nodes = (Node *) malloc(node.numBytes);
    assert(node.nodes);
    auto nptr = node.nodes;
    auto nlast = node.nodes + node.numValues;
    auto fptr = from.nodes;
    for (; nptr != nlast; ++nptr, ++fptr) {
        nptr->type = fptr->type;
        impl(*nptr)->init(*nptr, *fptr);
    }
    *nlast = *fptr;
    nlast->nodes = &node;
}

//===========================================================================
void MetaImpl::destroy(Node & node) {
    assert(node.nodes);
    auto * ptr = node.nodes;
    auto * last = ptr + node.numValues;
    for (; ptr != last; ++ptr)
        impl(*ptr)->destroy(*ptr);
    free(node.nodes);
}

//===========================================================================
unsigned MetaImpl::nodePos(Node const & node, unsigned value) const {
    assert(relBase(value, node.depth) == node.base);
    auto pos = relValue(value, node.depth) / (valueMask(node.depth + 1) + 1);
    assert(pos < node.numValues);
    return pos;
}

//===========================================================================
bool MetaImpl::insert(Node & node, unsigned value) {
    auto pos = nodePos(node, value);
    auto & rnode = node.nodes[pos];
    if (!impl(rnode)->insert(rnode, value))
        return false;
    if (rnode.type != kFull)
        return true;
    for (unsigned i = 0; i < node.numValues; ++i) {
        if (node.nodes[i].type != kFull)
            return true;
    }
    // convert to full
    destroy(node);
    node.type = kFull;
    s_fullImpl.init(node, true);
    return true;
}

//===========================================================================
void MetaImpl::insert(
    Node & node,
    unsigned const * first,
    unsigned const * last
) {
    for (; first != last; ++first)
        insert(node, *first);
}

//===========================================================================
bool MetaImpl::erase(Node & node, unsigned value) {
    auto pos = nodePos(node, value);
    auto & rnode = node.nodes[pos];
    if (!impl(rnode)->erase(rnode, value))
        return false;
    if (rnode.type != kEmpty)
        return true;
    for (unsigned i = 0; i < node.numValues; ++i) {
        if (node.nodes[i].type != kEmpty)
            return true;
    }
    // convert to empty
    destroy(node);
    node.type = kEmpty;
    s_emptyImpl.init(node, false);
    return true;
}

//===========================================================================
size_t MetaImpl::size(Node const & node) const {
    size_t num = 0;
    auto * ptr = node.nodes;
    auto * last = ptr + node.numValues;
    for (; ptr != last; ++ptr)
        num += impl(*ptr)->size(*ptr);
    return num;
}

//===========================================================================
bool MetaImpl::findFirst(
    Node const ** onode,
    unsigned * ovalue,
    Node const & node,
    unsigned first
) const {
    auto pos = nodePos(node, first);
    auto ptr = (Node const *) node.nodes + pos;
    auto last = node.nodes + node.numValues;
    for (;;) {
        if (impl(*ptr)->findFirst(onode, ovalue, *ptr, first))
            return true;
        if (++ptr == last)
            return false;
        first = absBase(*ptr);
    }
}

//===========================================================================
bool MetaImpl::lastContiguous(
    Node const ** onode,
    unsigned * ovalue,
    Node const & node,
    unsigned first
) const {
    auto pos = nodePos(node, first);
    auto ptr = (Node const *) node.nodes + pos;
    auto last = node.nodes + node.numValues;
    for (;;) {
        if (impl(*ptr)->lastContiguous(onode, ovalue, *ptr, first))
            return true;
        *onode = ptr;
        *ovalue = absBase(*ptr) + valueMask(ptr->depth);
        if (++ptr == last)
            return false;
        first = absBase(*ptr);
    }
}


/****************************************************************************
*
*   compare
*
*   Return values:
*      -2: left < right, unless left has following non-empty nodes
*      -1: left < right
*       0: left == right
*       1: left > right
*       2: left > right, unless right has following non-empty nodes
*
***/

static int compare(Node const & left, Node const & right);

//===========================================================================
static int cmpError(Node const & left, Node const & right) {
    logMsgFatal() << "compare: incompatible node types, " << left.type
        << ", " << right.type;
    return 0;
}

//===========================================================================
static int cmpLessIf(Node const & left, Node const & right) { return -2; }
static int cmpMoreIf(Node const & left, Node const & right) { return 2; }
static int cmpEqual(Node const & left, Node const & right) { return 0; }

//===========================================================================
static int cmpVecIf(Node const & left, Node const & right) {
    auto minMax = absBase(right) + right.numValues - 1;
    if (minMax == right.values[right.numValues - 1]) {
        return 2;
    } else {
        return -1;
    }
}

//===========================================================================
static int cmpBitIf(Node const & left, Node const & right) {
    auto base = (uint64_t *) right.values;
    BitView bits{base, BitmapImpl::numInt64s()};
    auto i = bits.findZero();
    if (i && bits.find(i) == BitView::npos) {
        return 2;
    } else {
        return -1;
    }
}

//===========================================================================
static int cmpMetaIf(Node const & left, Node const & right) {
    auto i = UnsignedSet::Iterator{&right};
    auto ri = UnsignedSet::RangeIterator{i};
    if (ri->first == absBase(right) && !++ri) {
        return 2;
    } else {
        return -1;
    }
}

//===========================================================================
static int cmpRVecIf(Node const & left, Node const & right) {
    return -cmpVecIf(right, left);
}

//===========================================================================
static int cmpRBitIf(Node const & left, Node const & right) {
    return -cmpBitIf(right, left);
}

//===========================================================================
static int cmpRMetaIf(Node const & left, Node const & right) {
    return -cmpMetaIf(right, left);
}

//===========================================================================
static int cmpIter(Node const & left, Node const & right) {
    auto li = UnsignedSet::Iterator(&left);
    auto ri = UnsignedSet::Iterator(&right);
    for (; li && ri; ++li, ++ri) {
        if (*li != *ri)
            return *li > *ri ? 1 : -1;
    }
    return 2 * ((bool) ri - (bool) li);
}

//===========================================================================
static int cmpVec(Node const & left, Node const & right) {
    auto li = left.values;
    auto le = li + left.numValues;
    auto ri = right.values;
    auto re = ri + right.numValues;
    for (; li != le && ri != re; ++li, ++ri) {
        if (*li != *ri)
            return *li > *ri ? 1 : -1;
    }
    return 2 * ((ri == re) - (li == le));
}

//===========================================================================
static int cmpBit(uint64_t left, uint64_t right) {
    if (left == right)
        return 0;
    auto a = reverseBits(left);
    auto b = reverseBits(right);
    constexpr uint64_t mask = numeric_limits<uint64_t>::max();
    if (a < b) {
        return a == (b & (mask << trailingZeroBits(a))) ? -2 : 1;
    } else {
        return b == (a & (mask << trailingZeroBits(b))) ? 2 : -1;
    }
}

//===========================================================================
static int cmpBit(Node const & left, Node const & right) {
    auto li = (uint64_t *) left.values;
    auto le = li + kDataSize / sizeof *li;
    auto ri = (uint64_t *) right.values;
    auto re = ri + kDataSize / sizeof *ri;
    for (; li != le && ri != re; ++li, ++ri) {
        if (int rc = cmpBit(*li, *ri)) {
            if (rc == -2) {
                while (++li != le) {
                    if (*li)
                        return 1;
                }
            } else if (rc == 2) {
                while (++ri != re) {
                    if (*ri)
                        return -1;
                }
            }
            return rc;
        }
    }
    return 2 * ((ri == re) - (li == le));
}

//===========================================================================
static int cmpMeta(Node const & left, Node const & right) {
    auto li = left.nodes;
    auto le = li + left.numValues;
    auto ri = right.nodes;
    auto re = ri + right.numValues;
    for (; li != le && ri != re; ++li, ++ri) {
        if (int rc = compare(*li, *ri)) {
            if (rc == -2) {
                while (++li != le) {
                    if (li->type != kEmpty)
                        return 1;
                }
            } else if (rc == 2) {
                while (++ri != re) {
                    if (ri->type != kEmpty)
                        return -1;
                }
            }
            return rc;
        }
    }
    return 2 * ((ri == re) - (li == le));
}

//===========================================================================
static int compare(Node const & left, Node const & right) {
    using CompareFn = int(Node const & left, Node const & right);
    static CompareFn * const functs[][kNodeTypes] = {
    // LEFT                         RIGHT
    //             empty      full        vector     bitmap     meta
    /* empty  */ { cmpEqual,  cmpLessIf,  cmpLessIf, cmpLessIf, cmpLessIf },
    /* full   */ { cmpMoreIf, cmpEqual,   cmpVecIf,  cmpBitIf,  cmpMetaIf },
    /* vector */ { cmpMoreIf, cmpRVecIf,  cmpVec,    cmpIter,   cmpIter   },
    /* bitmap */ { cmpMoreIf, cmpRBitIf,  cmpIter,   cmpBit,    cmpError  },
    /* meta   */ { cmpMoreIf, cmpRMetaIf, cmpIter,   cmpError,  cmpMeta   },
    };
    return functs[left.type][right.type](left, right);
}


/****************************************************************************
*
*   insert(Node &, Node const &)
*
***/

static void insert(Node & left, Node const & right);

//===========================================================================
static void insError(Node & left, Node const & right) {
    logMsgFatal() << "insert: incompatible node types, " << left.type
        << ", " << right.type;
}

//===========================================================================
static void insSkip(Node & left, Node const & right)
{}

//===========================================================================
static void insFull(Node & left, Node const & right) {
    impl(left)->destroy(left);
    left.type = kFull;
    s_fullImpl.init(left, true);
}

//===========================================================================
static void insCopy(Node & left, Node const & right) {
    impl(left)->destroy(left);
    left.type = right.type;
    impl(left)->init(left, right);
}

//===========================================================================
static void insIter(Node & left, Node const & right) {
    auto ri = UnsignedSet::Iterator(&right);
    for (; ri; ++ri)
        impl(left)->insert(left, *ri);
}

//===========================================================================
static void insRIter(Node & left, Node const & right) {
    Node tmp;
    insCopy(tmp, right);
    swap(left, tmp);
    insIter(left, move(tmp));
    impl(tmp)->destroy(tmp);
}

//===========================================================================
static void insVec(Node & left, Node const & right) {
    assert(right.type == kVector);
    auto ri = right.values;
    auto re = ri + right.numValues;
    s_vectorImpl.insert(left, ri, re);
}

//===========================================================================
static void insBitmap(Node & left, Node const & right) {
    auto li = (uint64_t *) left.values;
    auto le = li + kDataSize / sizeof *li;
    auto ri = (uint64_t *) right.values;
    left.numValues = 0;
    for (; li != le; ++li, ++ri) {
        if (*li |= *ri)
            left.numValues += 1;
    }
    if (left.numValues == BitmapImpl::numInt64s())
        insFull(left, right);
}

//===========================================================================
static void insMeta(Node & left, Node const & right) {
    auto li = left.nodes;
    auto le = li + left.numValues;
    auto ri = right.nodes;
    for (; li != le; ++li, ++ri) {
        insert(*li, *ri);
        if (li->type != kFull)
            goto NOT_FULL;
    }
    insFull(left, right);
    return;

    for (; li != le; ++li, ++ri) {
        insert(*li, *ri);
NOT_FULL: ;
    }
}

//===========================================================================
static void insert(Node & left, Node const & right) {
    using InsertFn = void(Node & left, Node const & right);
    static InsertFn * const functs[][kNodeTypes] = {
    // LEFT                         RIGHT
    //             empty    full     vector   bitmap     meta
    /* empty  */ { insSkip, insFull, insCopy, insCopy,   insCopy  },
    /* full   */ { insSkip, insSkip, insSkip, insSkip,   insSkip  },
    /* vector */ { insSkip, insFull, insVec,  insRIter,  insRIter },
    /* bitmap */ { insSkip, insFull, insIter, insBitmap, insError },
    /* meta   */ { insSkip, insFull, insIter, insError,  insMeta  },
    };
    functs[left.type][right.type](left, right);
}


/****************************************************************************
*
*   insert(Node &, Node &&)
*
***/

static void insert(Node & left, Node && right);

//===========================================================================
static void insError(Node & left, Node && right) {
    insError(left, right);
}

//===========================================================================
static void insSkip(Node & left, Node && right)
{}

//===========================================================================
static void insFull(Node & left, Node && right) {
    insFull(left, right);
}

//===========================================================================
static void insMove(Node & left, Node && right) {
    swap(left, right);
}

//===========================================================================
static void insIter(Node & left, Node && right) {
    insIter(left, right);
}

//===========================================================================
static void insRIter(Node & left, Node && right) {
    swap(left, right);
    insIter(left, move(right));
}

//===========================================================================
static void insVec(Node & left, Node && right) {
    insVec(left, right);
}

//===========================================================================
static void insBitmap(Node & left, Node && right) {
    insBitmap(left, right);
}

//===========================================================================
static void insMeta(Node & left, Node && right) {
    auto li = left.nodes;
    auto le = li + left.numValues;
    auto ri = right.nodes;
    for (; li != le; ++li, ++ri) {
        insert(*li, move(*ri));
        if (li->type != kFull)
            goto NOT_FULL;
    }
    insFull(left, move(right));
    return;

    for (; li != le; ++li, ++ri) {
        insert(*li, move(*ri));
NOT_FULL: ;
    }
}

//===========================================================================
static void insert(Node & left, Node && right) {
    using InsertFn = void(Node & left, Node && right);
    static InsertFn * const functs[][kNodeTypes] = {
    // LEFT                         RIGHT
    //             empty    full     vector   bitmap     meta
    /* empty  */ { insSkip, insFull, insMove, insMove,   insMove  },
    /* full   */ { insSkip, insSkip, insSkip, insSkip,   insSkip  },
    /* vector */ { insSkip, insFull, insVec,  insRIter,  insRIter },
    /* bitmap */ { insSkip, insFull, insIter, insBitmap, insError },
    /* meta   */ { insSkip, insFull, insIter, insError,  insMeta  },
    };
    functs[left.type][right.type](left, move(right));
}


/****************************************************************************
*
*   erase(Node &, Node const &)
*
***/

static void erase(Node & left, Node const & right);

//===========================================================================
static void eraError(Node & left, Node const & right) {
    logMsgFatal() << "erase: incompatible node types, " << left.type
        << ", " << right.type;
}

//===========================================================================
static void eraSkip(Node & left, Node const & right)
{}

//===========================================================================
static void eraEmpty(Node & left, Node const & right) {
    impl(left)->destroy(left);
    left.type = kEmpty;
    s_emptyImpl.init(left, false);
}

//===========================================================================
static void eraChange(Node & left, Node const & right) {
    auto ri = UnsignedSet::Iterator(&right);
    assert(ri);

    // convert from full to either bitmap or meta
    impl(left)->erase(left, *ri);

    erase(left, right);
}

//===========================================================================
static void eraFind(Node & left, Node const & right) {
    // Go through values of left vector and skip (aka remove) the ones that
    // are found in right node (values to be erased).
    assert(left.type == kVector);
    auto li = left.values;
    auto out = li;
    auto le = li + left.numValues;
    auto ptr = impl(right);
    Node const * node = nullptr;
    unsigned value = 0;
    for (; li != le; ++li) {
        if (*li < value) {
            *out++ = *li;
            continue;
        }
        if (!ptr->findFirst(&node, &value, right, *li)) {
            do {
                *out++ = *li++;
            } while (li != le);
            break;
        }
        if (value != *li)
            *out++ = *li;
    }
    left.numValues = uint16_t(out - left.values);
    if (!left.numValues)
        eraEmpty(left, right);
}

//===========================================================================
static void eraIter(Node & left, Node const & right) {
    assert(right.type == kVector);
    auto ri = right.values;
    auto re = ri + right.numValues;
    for (; ri != re; ++ri)
        impl(left)->erase(left, *ri);
}

//===========================================================================
static void eraVec(Node & left, Node const & right) {
    auto le = set_difference(
        left.values, left.values + left.numValues,
        right.values, right.values + right.numValues,
        left.values
    );
    left.numValues = uint16_t(le - left.values);
    if (!left.numValues)
        eraEmpty(left, right);
}

//===========================================================================
static void eraBitmap(Node & left, Node const & right) {
    auto li = (uint64_t *) left.values;
    auto le = li + kDataSize / sizeof *li;
    auto ri = (uint64_t *) right.values;
    left.numValues = 0;
    for (; li != le; ++li, ++ri) {
        if (*li &= ~*ri)
            left.numValues += 1;
    }
    if (!left.numValues)
        eraEmpty(left, right);
}

//===========================================================================
static void eraMeta(Node & left, Node const & right) {
    assert(left.numValues == right.numValues);
    auto li = left.nodes;
    auto le = li + left.numValues;
    auto ri = right.nodes;
    for (; li != le; ++li, ++ri) {
        erase(*li, *ri);
        if (li->type != kEmpty)
            goto NOT_EMPTY;
    }
    eraEmpty(left, right);
    return;

    for (; li != le; ++li, ++ri) {
        erase(*li, *ri);
NOT_EMPTY: ;
    }
}

//===========================================================================
static void erase(Node & left, Node const & right) {
    using EraseFn = void(Node & left, Node const & right);
    static EraseFn * const functs[][kNodeTypes] = {
    // LEFT                         RIGHT
    //             empty    full      vector     bitmap     meta
    /* empty  */ { eraSkip, eraSkip,  eraSkip,   eraSkip,   eraSkip   },
    /* full   */ { eraSkip, eraEmpty, eraIter,   eraChange, eraChange },
    /* vector */ { eraSkip, eraEmpty, eraVec,    eraFind,   eraFind   },
    /* bitmap */ { eraSkip, eraEmpty, eraIter,   eraBitmap, eraError  },
    /* meta   */ { eraSkip, eraEmpty, eraIter,   eraError,  eraMeta   },
    };
    functs[left.type][right.type](left, right);
}


/****************************************************************************
*
*   intersect(Node &, Node const &)
*
***/

static void intersect(Node & left, Node const & right);

//===========================================================================
static void isecError(Node & left, Node const & right) {
    logMsgFatal() << "intersect: incompatible node types, " << left.type
        << ", " << right.type;
}

//===========================================================================
static void isecSkip(Node & left, Node const & right)
{}

//===========================================================================
static void isecEmpty(Node & left, Node const & right) {
    eraEmpty(left, right);
}

//===========================================================================
static void isecCopy(Node & left, Node const & right) {
    insCopy(left, right);
}

//===========================================================================
static void isecFind(Node & left, Node const & right) {
    // Go through values of left vector and remove the ones that aren't
    // found in right node.
    assert(left.type == kVector);
    auto li = left.values;
    auto out = li;
    auto le = li + left.numValues;
    auto ptr = impl(right);
    Node const * node = nullptr;
    unsigned value = 0;
    for (; li != le; ++li) {
        if (*li < value)
            continue;
        if (!ptr->findFirst(&node, &value, right, *li))
            break;
        if (value == *li)
            *out++ = *li;
    }
    left.numValues = uint16_t(out - left.values);
    if (!left.numValues)
        isecEmpty(left, right);
}

//===========================================================================
static void isecRFind(Node & left, Node const & right) {
    Node tmp;
    isecCopy(tmp, right);
    swap(left, tmp);
    isecFind(left, move(tmp));
    impl(tmp)->destroy(tmp);
}

//===========================================================================
static void isecVec(Node & left, Node const & right) {
    auto le = set_intersection(
        left.values, left.values + left.numValues,
        right.values, right.values + right.numValues,
        left.values
    );
    left.numValues = uint16_t(le - left.values);
    if (!left.numValues)
        isecEmpty(left, right);
}

//===========================================================================
static void isecBitmap(Node & left, Node const & right) {
    auto li = (uint64_t *) left.values;
    auto le = li + kDataSize / sizeof *li;
    auto ri = (uint64_t *) right.values;
    left.numValues = 0;
    for (; li != le; ++li, ++ri) {
        if (*li &= *ri)
            left.numValues += 1;
    }
    if (!left.numValues)
        isecEmpty(left, right);
}

//===========================================================================
static void isecMeta(Node & left, Node const & right) {
    auto li = left.nodes;
    auto le = li + left.numValues;
    auto ri = right.nodes;
    for (; li != le; ++li, ++ri) {
        intersect(*li, *ri);
        if (li->type != kEmpty)
            goto NOT_EMPTY;
    }
    isecEmpty(left, right);
    return;

    for (; li != le; ++li, ++ri) {
        intersect(*li, *ri);
NOT_EMPTY: ;
    }
}

//===========================================================================
static void intersect(Node & left, Node const & right) {
    using IsectFn = void(Node & left, Node const & right);
    static IsectFn * const functs[][kNodeTypes] = {
    // LEFT                         RIGHT
    //             empty      full      vector     bitmap      meta
    /* empty  */ { isecSkip,  isecSkip, isecSkip,  isecSkip,   isecSkip },
    /* full   */ { isecEmpty, isecSkip, isecCopy,  isecCopy,   isecCopy  },
    /* vector */ { isecEmpty, isecSkip, isecVec,   isecFind,   isecFind  },
    /* bitmap */ { isecEmpty, isecSkip, isecRFind, isecBitmap, isecError },
    /* meta   */ { isecEmpty, isecSkip, isecRFind, isecError,  isecMeta  },
    };
    functs[left.type][right.type](left, right);
}


/****************************************************************************
*
*   intersect(Node &, Node &&)
*
***/

static void intersect(Node & left, Node && right);

//===========================================================================
static void isecError(Node & left, Node && right) {
    isecError(left, right);
}

//===========================================================================
static void isecSkip(Node & left, Node && right)
{}

//===========================================================================
static void isecEmpty(Node & left, Node && right) {
    isecEmpty(left, right);
}

//===========================================================================
static void isecMove(Node & left, Node && right) {
    swap(left, right);
}

//===========================================================================
static void isecFind(Node & left, Node && right) {
    isecFind(left, right);
}

//===========================================================================
static void isecRFind(Node & left, Node && right) {
    swap(left, right);
    isecFind(left, move(right));
}

//===========================================================================
static void isecVec(Node & left, Node && right) {
    isecVec(left, right);
}

//===========================================================================
static void isecBitmap(Node & left, Node && right) {
    isecBitmap(left, right);
}

//===========================================================================
static void isecMeta(Node & left, Node && right) {
    auto li = left.nodes;
    auto le = li + left.numValues;
    auto ri = right.nodes;
    for (; li != le; ++li, ++ri) {
        intersect(*li, move(*ri));
        if (li->type != kEmpty)
            goto NOT_EMPTY;
    }
    isecEmpty(left, move(right));
    return;

    for (; li != le; ++li, ++ri) {
        intersect(*li, move(*ri));
NOT_EMPTY: ;
    }
}

//===========================================================================
static void intersect(Node & left, Node && right) {
    using IsectFn = void(Node & left, Node && right);
    static IsectFn * const functs[][kNodeTypes] = {
    // LEFT                         RIGHT
    //             empty      full      vector     bitmap      meta
    /* empty  */ { isecSkip,  isecSkip, isecEmpty, isecEmpty,  isecEmpty },
    /* full   */ { isecEmpty, isecSkip, isecMove,  isecMove,   isecMove  },
    /* vector */ { isecEmpty, isecSkip, isecVec,   isecFind,   isecFind  },
    /* bitmap */ { isecEmpty, isecSkip, isecRFind, isecBitmap, isecError },
    /* meta   */ { isecEmpty, isecSkip, isecRFind, isecError,  isecMeta  },
    };
    functs[left.type][right.type](left, move(right));
}


/****************************************************************************
*
*   UnsignedSet::Node
*
***/

//===========================================================================
UnsignedSet::Node::Node()
    : type{kEmpty}
    , depth{0}
    , base{0}
    , numBytes{0}
    , numValues{0}
    , values{nullptr}
{}


/****************************************************************************
*
*   UnsignedSet
*
***/

//===========================================================================
UnsignedSet::UnsignedSet()
{}

//===========================================================================
UnsignedSet::UnsignedSet(UnsignedSet && from) noexcept {
    swap(from);
}

//===========================================================================
UnsignedSet::UnsignedSet(UnsignedSet const & from) {
    insert(from);
}

//===========================================================================
UnsignedSet::UnsignedSet(string_view from) {
    insert(from);
}

//===========================================================================
UnsignedSet::~UnsignedSet() {
    clear();
}

//===========================================================================
UnsignedSet & UnsignedSet::operator=(UnsignedSet && from) noexcept {
    clear();
    swap(from);
    return *this;
}

//===========================================================================
UnsignedSet & UnsignedSet::operator=(UnsignedSet const & from) {
    clear();
    insert(from);
    return *this;
}

//===========================================================================
UnsignedSet::iterator UnsignedSet::begin() const {
    return {&m_node};
}

//===========================================================================
UnsignedSet::iterator UnsignedSet::end() const {
    return {};
}

//===========================================================================
UnsignedSet::RangeRange UnsignedSet::ranges() const {
    return {begin()};
}

//===========================================================================
bool UnsignedSet::empty() const {
    return m_node.type == kEmpty;
}

//===========================================================================
size_t UnsignedSet::size() const {
    return impl(m_node)->size(m_node);
}

//===========================================================================
void UnsignedSet::clear() {
    impl(m_node)->destroy(m_node);
    m_node = {};
}

//===========================================================================
void UnsignedSet::fill() {
    clear();
    m_node.type = kFull;
    s_fullImpl.init(m_node, true);
}

//===========================================================================
void UnsignedSet::assign(unsigned value) {
    clear();
    insert(value);
}

//===========================================================================
void UnsignedSet::assign(UnsignedSet && from) {
    clear();
    insert(move(from));
}

//===========================================================================
void UnsignedSet::assign(UnsignedSet const & from) {
    clear();
    insert(from);
}

//===========================================================================
void UnsignedSet::assign(string_view src) {
    clear();
    insert(src);
}

//===========================================================================
bool UnsignedSet::insert(unsigned value) {
    return impl(m_node)->insert(m_node, value);
}

//===========================================================================
void UnsignedSet::insert(UnsignedSet && other) {
    ::insert(m_node, move(other.m_node));
}

//===========================================================================
void UnsignedSet::insert(UnsignedSet const & other) {
    ::insert(m_node, other.m_node);
}

//===========================================================================
void UnsignedSet::insert(string_view src) {
    char * eptr;
    auto first = strToUint(src, &eptr);
    if (!first && (src.data() == eptr || eptr[-1] != '0'))
        return;
    for (;;) {
        if (*eptr == '-') {
            auto second = strToUint(eptr + 1, &eptr);
            if (second < first)
                break;
            insert(first, second);
        } else {
            insert(first);
        }
        first = strToUint(eptr, &eptr);
        if (!first)
            break;
    }
}

//===========================================================================
void UnsignedSet::insert(unsigned low, unsigned high) {
    // TODO: make this efficient
    for (auto i = low; i <= high; ++i)
        insert(i);
}

//===========================================================================
bool UnsignedSet::erase(unsigned value) {
    return impl(m_node)->erase(m_node, value);
}

//===========================================================================
void UnsignedSet::erase(UnsignedSet::iterator where) {
    assert(where);
    erase(*where);
}

//===========================================================================
void UnsignedSet::erase(UnsignedSet const & other) {
    ::erase(m_node, other.m_node);
}

//===========================================================================
unsigned UnsignedSet::pop_front() {
    auto i = begin();
    auto val = *i;
    erase(i);
    return val;
}

//===========================================================================
void UnsignedSet::intersect(UnsignedSet && other) {
    ::intersect(m_node, move(other.m_node));
}

//===========================================================================
void UnsignedSet::intersect(UnsignedSet const & other) {
    ::intersect(m_node, other.m_node);
}

//===========================================================================
void UnsignedSet::swap(UnsignedSet & other) {
    ::swap(m_node, other.m_node);
}

//===========================================================================
int UnsignedSet::compare(UnsignedSet const & right) const {
    return ::compare(m_node, right.m_node);
}

//===========================================================================
bool UnsignedSet::includes(UnsignedSet const & other) const {
    assert(!"not implemented");
    return false;
}

//===========================================================================
bool UnsignedSet::intersects(UnsignedSet const & other) const {
    assert(!"not implemented");
    return false;
}

//===========================================================================
bool UnsignedSet::operator==(UnsignedSet const & right) const {
    return compare(right) == 0;
}

//===========================================================================
bool UnsignedSet::operator!=(UnsignedSet const & right) const {
    return compare(right) != 0;
}

//===========================================================================
bool UnsignedSet::operator<(UnsignedSet const & right) const {
    return compare(right) < 0;
}

//===========================================================================
bool UnsignedSet::operator>(UnsignedSet const & right) const {
    return compare(right) > 0;
}

//===========================================================================
bool UnsignedSet::operator<=(UnsignedSet const & right) const {
    return compare(right) <= 0;
}

//===========================================================================
bool UnsignedSet::operator>=(UnsignedSet const & right) const {
    return compare(right) >= 0;
}

//===========================================================================
size_t UnsignedSet::count(unsigned val) const {
    return (bool) find(val);
}

//===========================================================================
auto UnsignedSet::find(unsigned val) const -> iterator {
    auto first = lowerBound(val);
    return first && *first == val ? first : iterator{};
}

//===========================================================================
auto UnsignedSet::equalRange(unsigned val) const -> pair<iterator, iterator> {
    auto first = lowerBound(val);
    auto last = first;
    if (last && *last == val)
        ++last;
    return {first, last};
}

//===========================================================================
auto UnsignedSet::lowerBound(unsigned val) const -> iterator {
    return {&m_node, val, m_node.depth};
}

//===========================================================================
auto UnsignedSet::upperBound(unsigned val) const -> iterator {
    val += 1;
    return val ? lowerBound(val) : iterator{};
}

//===========================================================================
auto UnsignedSet::lastContiguous(iterator where) const -> iterator {
    return where.lastContiguous();
}

//===========================================================================
// Private
//===========================================================================
void UnsignedSet::iInsert(unsigned const * first, unsigned const * last) {
    impl(m_node)->insert(m_node, first, last);
}


/****************************************************************************
*
*   UnsignedSet::Iterator
*
***/

//===========================================================================
UnsignedSet::Iterator::Iterator(Node const * node)
    : Iterator{node, absBase(*node), node->depth}
{}

//===========================================================================
UnsignedSet::Iterator::Iterator(
    Node const * node,
    value_type value,
    unsigned minDepth
)
    : m_node{node}
    , m_value{value}
    , m_minDepth{minDepth}
{
    if (!m_node) {
        assert(!m_value && !m_minDepth);
    } else {
        if (!impl(*m_node)->findFirst(&m_node, &m_value, *m_node, m_value))
            *this = {};
    }
}

//===========================================================================
bool UnsignedSet::Iterator::operator== (Iterator const & right) const {
    return m_value == right.m_value && m_node == right.m_node;
}

//===========================================================================
bool UnsignedSet::Iterator::operator!= (Iterator const & right) const {
    return !(*this == right);
}

//===========================================================================
UnsignedSet::Iterator & UnsignedSet::Iterator::operator++() {
    if (m_value < absBase(*m_node) + valueMask(m_node->depth)) {
        m_value += 1;
        if (impl(*m_node)->findFirst(&m_node, &m_value, *m_node, m_value))
            return *this;
    }

    for (;;) {
        if (m_node->depth == m_minDepth) {
            *this = {};
            break;
        }
        m_node += 1;
        if (m_node->type == kMetaEnd) {
            m_node = m_node->nodes;
            continue;
        }
        m_value = absBase(*m_node);
        if (impl(*m_node)->findFirst(&m_node, &m_value, *m_node, m_value))
            break;
    }
    return *this;
}

//===========================================================================
UnsignedSet::Iterator UnsignedSet::Iterator::lastContiguous() const {
    auto node = m_node;
    auto value = m_value;
    if (impl(*node)->lastContiguous(&node, &value, *node, value))
        return {node, value, m_minDepth};

    auto ptr = node;
    for (;;) {
        if (!ptr->depth)
            break;
        ptr += 1;
        if (ptr->type == kMetaEnd) {
            ptr = ptr->nodes;
            assert(ptr->type != kMetaEnd);
            continue;
        }
        if (impl(*ptr)->lastContiguous(&node, &value, *ptr, absBase(*ptr)))
            break;
    }
    return {node, value, m_minDepth};
}


/****************************************************************************
*
*   UnsignedSet::RangeIterator
*
***/

//===========================================================================
UnsignedSet::RangeIterator::RangeIterator(Iterator where)
    : m_iter(where)
{
    if (m_iter)
        ++*this;
}

//===========================================================================
bool UnsignedSet::RangeIterator::operator== (
    RangeIterator const & right
) const {
    return m_value == right.m_value;
}

//===========================================================================
bool UnsignedSet::RangeIterator::operator!= (
    RangeIterator const & right
) const {
    return !(*this == right);
}

//===========================================================================
UnsignedSet::RangeIterator & UnsignedSet::RangeIterator::operator++() {
    if (!m_iter) {
        m_value = kEndValue;
    } else {
        m_value.first = *m_iter;
        m_iter = m_iter.lastContiguous();
        m_value.second = *m_iter;
        ++m_iter;
    }
    return *this;
}


/****************************************************************************
*
*   Free functions
*
***/

//===========================================================================
ostream & Dim::operator<<(ostream & os, UnsignedSet const & right) {
    if (auto v = right.ranges().begin()) {
        for (;;) {
            os << v->first;
            if (v->first != v->second)
                os << '-' << v->second;
            if (!++v)
                break;
            os << ' ';
        }
    }
    return os;
}
