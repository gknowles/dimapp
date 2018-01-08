// Copyright Glen Knowles 2017.
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

const unsigned kDataSize = 512;
static_assert(hammingWeight(kDataSize) == 1);
static_assert(kDataSize >= 256);

const unsigned kBitWidth = 32;

const unsigned kLeafBits = hammingWeight(8 * kDataSize - 1);
const unsigned kStepBits =
    hammingWeight(pow2Ceil(kDataSize / sizeof(Node) + 1) / 2 - 1);
const unsigned kMaxDepth =
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
constexpr unsigned absBase(const Node & node) {
    return node.base << 8;
}
constexpr unsigned absSize(const Node & node) {
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
    virtual void init(Node & node, const Node & from) = 0;

    virtual void destroy(Node & node) = 0;
    virtual bool insert(Node & node, unsigned value) = 0;
    virtual void insert(
        Node & node,
        const unsigned * first,
        const unsigned * last
    ) = 0;
    virtual void erase(Node & node, unsigned value) = 0;

    virtual size_t size(const Node & node) const = 0;
    virtual bool findFirst(
        const Node ** onode,
        unsigned * ovalue,
        const Node & node,
        unsigned first
    ) const = 0;

    // [first] is empty - return true
    // [first, end of node] is full - update onode & ovalue, return false
    // otherwise - update onode & ovalue, return true
    virtual bool lastContiguous(
        const Node ** onode,
        unsigned * ovalue,
        const Node & node,
        unsigned first
    ) const = 0;
};

} // namespace


/****************************************************************************
*
*   Helpers
*
***/

static IImplBase * impl(const Node & node);


/****************************************************************************
*
*   EmptyImpl
*
***/

namespace {
struct EmptyImpl final : IImplBase {
    void init(Node & node, bool full) override;
    void init(Node & node, const Node & from) override;
    void destroy(Node & node) override;
    bool insert(Node & node, unsigned value) override;
    void insert(
        Node & node,
        const unsigned * first,
        const unsigned * last
    ) override;
    void erase(Node & node, unsigned value) override;

    size_t size(const Node & node) const override;
    bool findFirst(
        const Node ** onode,
        unsigned * ovalue,
        const Node & node,
        unsigned first
    ) const override;
    bool lastContiguous(
        const Node ** onode,
        unsigned * ovalue,
        const Node & node,
        unsigned first
    ) const override;
};
} // namespace
static EmptyImpl s_emptyImpl;

//===========================================================================
void EmptyImpl::init(Node & node, bool full) {
    assert(node.type == kEmpty);
    assert(!full);
    node.numBytes = 0;
    node.numValues = 0;
    node.values = nullptr;
}

//===========================================================================
void EmptyImpl::init(Node & node, const Node & from) {
    assert(node.type == kEmpty);
    assert(from.type == kEmpty);
    node = from;
}

//===========================================================================
void EmptyImpl::destroy(Node & node)
{}

//===========================================================================
bool EmptyImpl::insert(Node & node, unsigned value) {
    assert(relBase(value, node.depth) == node.base);
    node.type = kVector;
    auto ptr = impl(node);
    ptr->init(node, false);
    ptr->insert(node, value);
    return true;
}

//===========================================================================
void EmptyImpl::insert(
    Node & node,
    const unsigned * first,
    const unsigned * last
) {
    if (first != last) {
        insert(node, *first++);
        if (first != last)
            impl(node)->insert(node, first, last);
    }
}

//===========================================================================
void EmptyImpl::erase(Node & node, unsigned value) {
    assert(relBase(value, node.depth) == node.base);
}

//===========================================================================
size_t EmptyImpl::size(const Node & node) const {
    return 0;
}

//===========================================================================
bool EmptyImpl::findFirst(
    const Node ** onode,
    unsigned * ovalue,
    const Node & node,
    unsigned first
) const {
    return false;
}

//===========================================================================
bool EmptyImpl::lastContiguous(
    const Node ** onode,
    unsigned * ovalue,
    const Node & node,
    unsigned first
) const {
    return true;
}


/****************************************************************************
*
*   FullImpl
*
***/

namespace {
struct FullImpl final : IImplBase {
    void init(Node & node, bool full) override;
    void init(Node & node, const Node & from) override;
    void destroy(Node & node) override;
    bool insert(Node & node, unsigned value) override;
    void insert(
        Node & node,
        const unsigned * first,
        const unsigned * last
    ) override;
    void erase(Node & node, unsigned value) override;

    size_t size(const Node & node) const override;
    bool findFirst(
        const Node ** onode,
        unsigned * ovalue,
        const Node & node,
        unsigned first
    ) const override;
    bool lastContiguous(
        const Node ** onode,
        unsigned * ovalue,
        const Node & node,
        unsigned first
    ) const override;

private:
    void convert(Node & node);
};
} // namespace
static FullImpl s_fullImpl;

//===========================================================================
void FullImpl::init(Node & node, bool full) {
    assert(node.type == kFull);
    assert(full);
    node.numBytes = 0;
    node.numValues = 0;
    node.values = nullptr;
}

//===========================================================================
void FullImpl::init(Node & node, const Node & from) {
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
    const unsigned * first,
    const unsigned * last
)
{}

//===========================================================================
void FullImpl::erase(Node & node, unsigned value) {
    assert(relBase(value, node.depth) == node.base);
    convert(node);
    impl(node)->erase(node, value);
}

//===========================================================================
size_t FullImpl::size(const Node & node) const {
    return valueMask(node.depth) + 1;
}

//===========================================================================
bool FullImpl::findFirst(
    const Node ** onode,
    unsigned * ovalue,
    const Node & node,
    unsigned first
) const {
    if (first < size(node)) {
        *onode = &node;
        *ovalue = first;
        return true;
    }
    return false;
}

//===========================================================================
bool FullImpl::lastContiguous(
    const Node ** onode,
    unsigned * ovalue,
    const Node & node,
    unsigned first
) const {
    *onode = &node;
    *ovalue = absBase(node) + valueMask(node.depth);
    return false;
}

//===========================================================================
void FullImpl::convert(Node & node) {
    if (node.depth == maxDepth()) {
        node.type = kBitmap;
    } else {
        node.type = kMeta;
    }
    impl(node)->init(node, true);
}


/****************************************************************************
*
*   VectorImpl
*
***/

namespace {
struct VectorImpl final : IImplBase {
    static constexpr size_t maxValues() {
        return kDataSize / sizeof(*Node::values);
    }

    void init(Node & node, bool full) override;
    void init(Node & node, const Node & from) override;
    void destroy(Node & node) override;
    bool insert(Node & node, unsigned value) override;
    void insert(
        Node & node,
        const unsigned * first,
        const unsigned * last
    ) override;
    void erase(Node & node, unsigned value) override;

    size_t size(const Node & node) const override;
    bool findFirst(
        const Node ** onode,
        unsigned * ovalue,
        const Node & node,
        unsigned first
    ) const override;
    bool lastContiguous(
        const Node ** onode,
        unsigned * ovalue,
        const Node & node,
        unsigned first
    ) const override;

private:
    void convert(Node & node);
};
} // namespace
static VectorImpl s_vectorImpl;

//===========================================================================
void VectorImpl::init(Node & node, bool full) {
    assert(node.type == kVector);
    assert(!full);
    node.numBytes = kDataSize;
    node.numValues = 0;
    node.values = (unsigned *) malloc(node.numBytes);
}

//===========================================================================
void VectorImpl::init(Node & node, const Node & from) {
    assert(node.type == kVector);
    assert(from.type == kVector);
    node = from;
    node.values = (unsigned *) malloc(node.numBytes);
    assert(node.values);
    memcpy(node.values, from.values, node.numBytes);
}

//===========================================================================
void VectorImpl::destroy(Node & node) {
    if (node.values)
        free(node.values);
}

//===========================================================================
bool VectorImpl::insert(Node & node, unsigned value) {
    assert(relBase(value, node.depth) == node.base);
    auto last = node.values + node.numValues;
    auto ptr = lower_bound(node.values, last, value);
    if (ptr == last) {
        if (node.numValues == maxValues()) {
            convert(node);
            impl(node)->insert(node, value);
        } else {
            *ptr = value;
            node.numValues += 1;
        }
        return true;
    }
    if (*ptr == value)
        return false;
    if (node.numValues == maxValues()) {
        convert(node);
        impl(node)->insert(node, value);
    } else {
        memmove(ptr + 1, ptr, sizeof(*ptr) * (last - ptr));
        *ptr = value;
        node.numValues += 1;
    }
    return true;
}

//===========================================================================
void VectorImpl::insert(
    Node & node,
    const unsigned * first,
    const unsigned * last
) {
    while (first != last) {
        if (node.type != kVector)
            return impl(node)->insert(node, first, last);
        insert(node, *first++);
    }
}

//===========================================================================
void VectorImpl::erase(Node & node, unsigned value) {
    assert(relBase(value, node.depth) == node.base);
    auto last = node.values + node.numValues;
    auto ptr = lower_bound(node.values, last, value);
    if (ptr != last && *ptr == value) {
        if (--node.numValues) {
            // Still has values, shift remaining ones down
            memmove(ptr, ptr + 1, sizeof(*ptr) * (last - ptr - 1));
        } else {
            // No more values, convert to empty node.
            destroy(node);
            node.type = kEmpty;
            impl(node)->init(node, false);
        }
    }
}

//===========================================================================
size_t VectorImpl::size(const Node & node) const {
    return node.numValues;
}

//===========================================================================
bool VectorImpl::findFirst(
    const Node ** onode,
    unsigned * ovalue,
    const Node & node,
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
    const Node ** onode,
    unsigned * ovalue,
    const Node & node,
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
void VectorImpl::convert(Node & node) {
    auto ptr = node.values;
    auto last = ptr + node.numValues;
    if (node.depth == maxDepth()) {
        node.type = kBitmap;
    } else {
        node.type = kMeta;
    }
    auto iptr = impl(node);
    iptr->init(node, false);
    iptr->insert(node, ptr, last);
    free(ptr);
}


/****************************************************************************
*
*   BitmapImpl
*
***/

namespace {
struct BitmapImpl final : IImplBase {
    static constexpr size_t numInt64s() {
        return kDataSize / sizeof(uint64_t);
    }
    static constexpr size_t numBits() { return 64 * numInt64s(); }

    void init(Node & node, bool full) override;
    void init(Node & node, const Node & from) override;
    void destroy(Node & node) override;
    bool insert(Node & node, unsigned value) override;
    void insert(
        Node & node,
        const unsigned * first,
        const unsigned * last
    ) override;
    void erase(Node & node, unsigned value) override;

    size_t size(const Node & node) const override;
    bool findFirst(
        const Node ** onode,
        unsigned * ovalue,
        const Node & node,
        unsigned first
    ) const override;
    bool lastContiguous(
        const Node ** onode,
        unsigned * ovalue,
        const Node & node,
        unsigned first
    ) const override;
};
} // namespace
static BitmapImpl s_bitmapImpl;

//===========================================================================
void BitmapImpl::init(Node & node, bool full) {
    assert(node.type == kBitmap);
    node.numBytes = kDataSize;
    node.values = (unsigned *) malloc(node.numBytes);
    assert(node.values);
    if (full) {
        node.numValues = kDataSize / sizeof(uint64_t);
        memset(node.values, 0xff, kDataSize);
    } else {
        node.numValues = 0;
        memset(node.values, 0, kDataSize);
    }
}

//===========================================================================
void BitmapImpl::init(Node & node, const Node & from) {
    assert(node.type == kBitmap);
    assert(from.type == kBitmap);
    node = from;
    node.values = (unsigned *) malloc(node.numBytes);
    assert(node.values);
    memcpy(node.values, from.values, node.numBytes);
}

//===========================================================================
void BitmapImpl::destroy(Node & node) {
    if (node.values)
        free(node.values);
}

//===========================================================================
bool BitmapImpl::insert(Node & node, unsigned value) {
    assert(relBase(value, node.depth) == node.base);
    auto base = (uint64_t *) node.values;
    value = relValue(value, node.depth);
    assert(value < numBits());
    auto ptr = base + value / 64;
    auto tmp = *ptr;
    *ptr |= 1ull << (value % 64);
    if (tmp != *ptr) {
        if (!tmp)
            node.numValues += 1;
        return true;
    }
    return false;
}

//===========================================================================
void BitmapImpl::insert(
    Node & node,
    const unsigned * first,
    const unsigned * last
) {
    for (; first != last; ++first)
        insert(node, *first);
}

//===========================================================================
void BitmapImpl::erase(Node & node, unsigned value) {
    assert(relBase(value, node.depth) == node.base);
    auto base = (uint64_t *) node.values;
    value = relValue(value, node.depth);
    assert(value < numBits());
    auto ptr = base + value / 64;
    auto tmp = *ptr;
    *ptr &= ~(1ull << (value % 64));
    if (tmp != *ptr && !*ptr) {
        if (!--node.numValues) {
            // convert to empty
            destroy(node);
            node.type = kEmpty;
            impl(node)->init(node, false);
        }
    }
}

//===========================================================================
size_t BitmapImpl::size(const Node & node) const {
    auto base = (uint64_t *) node.values;
    BitView bits{base, numInt64s()};
    return bits.count();
}

//===========================================================================
bool BitmapImpl::findFirst(
    const Node ** onode,
    unsigned * ovalue,
    const Node & node,
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
    const Node ** onode,
    unsigned * ovalue,
    const Node & node,
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

namespace {
struct MetaImpl final : IImplBase {
    static constexpr size_t maxNodes() {
        return kDataSize / sizeof(*Node::nodes);
    }

    void init(Node & node, bool full) override;
    void init(Node & node, const Node & from) override;
    void destroy(Node & node) override;
    bool insert(Node & node, unsigned value) override;
    void insert(
        Node & node,
        const unsigned * first,
        const unsigned * last
    ) override;
    void erase(Node & node, unsigned value) override;

    size_t size(const Node & node) const override;
    bool findFirst(
        const Node ** onode,
        unsigned * ovalue,
        const Node & node,
        unsigned first
    ) const override;
    bool lastContiguous(
        const Node ** onode,
        unsigned * ovalue,
        const Node & node,
        unsigned first
    ) const override;

private:
    unsigned nodePos(const Node & node, unsigned value) const;
};
} // namespace
static MetaImpl s_metaImpl;

//===========================================================================
void MetaImpl::init(Node & node, bool full) {
    assert(node.type == kMeta);
    node.numValues = numNodes(node.depth + 1);
    node.numBytes = (node.numValues + 1) * sizeof(*node.nodes);
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
    // is just a pointer to the parent node.
    *nlast = {};
    nlast->type = kMetaEnd;
    nlast->nodes = &node;
}

//===========================================================================
void MetaImpl::init(Node & node, const Node & from) {
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
    if (auto * ptr = node.nodes) {
        auto * last = ptr + node.numValues;
        for (; ptr != last; ++ptr)
            impl(*ptr)->destroy(*ptr);
        free(node.nodes);
    }
}

//===========================================================================
unsigned MetaImpl::nodePos(const Node & node, unsigned value) const {
    assert(relBase(value, node.depth) == node.base);
    auto pos = relValue(value, node.depth) / (valueMask(node.depth + 1) + 1);
    assert(pos < node.numValues);
    return pos;
}

//===========================================================================
bool MetaImpl::insert(Node & node, unsigned value) {
    auto pos = nodePos(node, value);
    auto & rnode = node.nodes[pos];
    return impl(rnode)->insert(rnode, value);
}

//===========================================================================
void MetaImpl::insert(
    Node & node,
    const unsigned * first,
    const unsigned * last
) {
    for (; first != last; ++first)
        insert(node, *first);
}

//===========================================================================
void MetaImpl::erase(Node & node, unsigned value) {
    auto pos = nodePos(node, value);
    auto & rnode = node.nodes[pos];
    if (rnode.type == kEmpty)
        return;
    impl(rnode)->erase(rnode, value);
    if (rnode.type != kEmpty)
        return;
    for (unsigned i = 0; i < node.numValues; ++i) {
        if (node.nodes[i].type != kEmpty)
            return;
    }
    // convert to empty
    destroy(node);
    node.type = kEmpty;
    impl(node)->init(node, false);
}

//===========================================================================
size_t MetaImpl::size(const Node & node) const {
    size_t num = 0;
    auto * ptr = node.nodes;
    auto * last = ptr + node.numValues;
    for (; ptr != last; ++ptr)
        num += impl(*ptr)->size(*ptr);
    return num;
}

//===========================================================================
bool MetaImpl::findFirst(
    const Node ** onode,
    unsigned * ovalue,
    const Node & node,
    unsigned first
) const {
    auto pos = nodePos(node, first);
    auto ptr = (const Node *) node.nodes + pos;
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
    const Node ** onode,
    unsigned * ovalue,
    const Node & node,
    unsigned first
) const {
    auto pos = nodePos(node, first);
    auto ptr = (const Node *) node.nodes + pos;
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
*   Helpers
*
***/

//===========================================================================
static IImplBase * impl(const Node & node) {
    switch (node.type) {
    case kEmpty: return &s_emptyImpl;
    case kFull: return &s_fullImpl;
    case kVector: return &s_vectorImpl;
    case kBitmap: return &s_bitmapImpl;
    case kMeta: return &s_metaImpl;
    }

    logMsgCrash() << "invalid node type: " << node.type;
    return nullptr;
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

static int compare(const Node & left, const Node & right);

//===========================================================================
static int cmpError(const Node & left, const Node & right) {
    logMsgCrash() << "compare: incompatible node types, " << left.type
        << ", " << right.type;
    return 0;
}

//===========================================================================
static int cmpLessIf(const Node & left, const Node & right) { return -2; }
static int cmpMoreIf(const Node & left, const Node & right) { return 2; }
static int cmpEqual(const Node & left, const Node & right) { return 0; }

//===========================================================================
static int cmpVecIf(const Node & left, const Node & right) {
    auto minMax = absBase(right) + right.numValues - 1;
    if (minMax == right.values[right.numValues - 1]) {
        return 2;
    } else {
        return -1;
    }
}

//===========================================================================
static int cmpBitIf(const Node & left, const Node & right) {
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
static int cmpMetaIf(const Node & left, const Node & right) {
    auto i = UnsignedSet::Iterator{&right};
    auto ri = UnsignedSet::RangeIterator{i};
    if (ri->first == absBase(right) && !++ri) {
        return 2;
    } else {
        return -1;
    }
}

//===========================================================================
static int cmpRVecIf(const Node & left, const Node & right) {
    return -cmpVecIf(right, left);
}

//===========================================================================
static int cmpRBitIf(const Node & left, const Node & right) {
    return -cmpBitIf(right, left);
}

//===========================================================================
static int cmpRMetaIf(const Node & left, const Node & right) {
    return -cmpMetaIf(right, left);
}

//===========================================================================
static int cmpIter(const Node & left, const Node & right) {
    auto li = UnsignedSet::Iterator(&left);
    auto ri = UnsignedSet::Iterator(&right);
    for (; li && ri; ++li, ++ri) {
        if (*li != *ri)
            return *li > *ri ? 1 : -1;
    }
    return 2 * ((bool) ri - (bool) li);
}

//===========================================================================
static int cmpVec(const Node & left, const Node & right) {
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
    uint64_t mask = numeric_limits<uint64_t>::max();
    if (a < b) {
        return a == (b & (mask << trailingZeroBits(a))) ? -2 : 1;
    } else {
        return b == (a & (mask << trailingZeroBits(b))) ? 2 : -1;
    }
}

//===========================================================================
static int cmpBit(const Node & left, const Node & right) {
    auto li = (uint64_t *) left.values;
    auto le = li + kDataSize / sizeof(*li);
    auto ri = (uint64_t *) right.values;
    auto re = ri + kDataSize / sizeof(*ri);
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
static int cmpMeta(const Node & left, const Node & right) {
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
// FIXME: node compare needs to consider whether or not all following nodes
//        are empty. For example: 1-2 < 1-3, but not if it's 1-2,100000
static int compare(const Node & left, const Node & right) {
    using CompareFn = int(const Node & left, const Node & right);
    CompareFn * functs[][kNodeTypes] = {
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
*   insert(Node &, const Node &)
*
***/

static void insert(Node & left, const Node & right);

//===========================================================================
static void insError(Node & left, const Node & right) {
    logMsgCrash() << "insert: incompatible node types, " << left.type
        << ", " << right.type;
}

//===========================================================================
static void insSkip(Node & left, const Node & right)
{}

//===========================================================================
static void insCopy(Node & left, const Node & right) {
    impl(left)->destroy(left);
    left.type = right.type;
    impl(left)->init(left, right);
}

//===========================================================================
static void insIter(Node & left, const Node & right) {
    auto ri = UnsignedSet::Iterator(&right);
    for (; ri; ++ri)
        impl(left)->insert(left, *ri);
}

//===========================================================================
static void insRIter(Node & left, const Node & right) {
    Node tmp;
    insCopy(tmp, right);
    swap(left, tmp);
    insIter(left, tmp);
}

//===========================================================================
static void insVec(Node & left, const Node & right) {
    assert(right.type == kVector);
    auto ri = right.values;
    auto re = ri + right.numValues;
    for (; ri != re; ++ri)
        impl(left)->insert(left, *ri);
}

//===========================================================================
static void insBitmap(Node & left, const Node & right) {
    auto li = (uint64_t *) left.values;
    auto le = li + kDataSize / sizeof(*li);
    auto ri = (uint64_t *) right.values;
    left.numValues = 0;
    for (; li != le; ++li, ++ri) {
        if (*li |= *ri)
            left.numValues += 1;
    }
}

//===========================================================================
static void insMeta(Node & left, const Node & right) {
    auto li = left.nodes;
    auto le = li + left.numValues;
    auto ri = right.nodes;
    auto re = ri + right.numValues;
    for (; li != le && ri != re; ++li, ++ri)
        insert(*li, *ri);
}

//===========================================================================
static void insert(Node & left, const Node & right) {
    using InsertFn = void(Node & left, const Node & right);
    InsertFn * functs[][kNodeTypes] = {
    // LEFT                         RIGHT
    //             empty    full     vector   bitmap     meta
    /* empty  */ { insSkip, insCopy, insCopy, insCopy,   insCopy  },
    /* full   */ { insSkip, insSkip, insSkip, insSkip,   insSkip  },
    /* vector */ { insSkip, insCopy, insVec,  insRIter,  insRIter },
    /* bitmap */ { insSkip, insCopy, insIter, insBitmap, insError },
    /* meta   */ { insSkip, insCopy, insIter, insError,  insMeta  },
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
    auto re = ri + right.numValues;
    for (; li != le && ri != re; ++li, ++ri)
        insert(*li, move(*ri));
}

//===========================================================================
static void insert(Node & left, Node && right) {
    using InsertFn = void(Node & left, Node && right);
    InsertFn * functs[][kNodeTypes] = {
    // LEFT                         RIGHT
    //             empty    full     vector   bitmap     meta
    /* empty  */ { insSkip, insMove, insMove, insMove,   insMove  },
    /* full   */ { insSkip, insSkip, insSkip, insSkip,   insSkip  },
    /* vector */ { insSkip, insMove, insVec,  insRIter,  insRIter },
    /* bitmap */ { insSkip, insMove, insIter, insBitmap, insError },
    /* meta   */ { insSkip, insMove, insIter, insError,  insMeta  },
    };
    functs[left.type][right.type](left, move(right));
}


/****************************************************************************
*
*   erase(Node &, const Node &)
*
***/

static void erase(Node & left, const Node & right);

//===========================================================================
static void eraError(Node & left, const Node & right) {
    logMsgCrash() << "erase: incompatible node types, " << left.type
        << ", " << right.type;
}

//===========================================================================
static void eraSkip(Node & left, const Node & right)
{}

//===========================================================================
static void eraEmpty(Node & left, const Node & right) {
    impl(left)->destroy(left);
    left.type = kEmpty;
    impl(left)->init(left, false);
}

//===========================================================================
static void eraChange(Node & left, const Node & right) {
    auto ri = UnsignedSet::Iterator(&right);
    assert(ri);

    // convert from full to either bitmap or meta
    impl(left)->erase(left, *ri);

    erase(left, right);
}

//===========================================================================
static void eraFind(Node & left, const Node & right) {
    // Go through values of left vector and skip (aka remove) the ones that
    // are found in right node (values to be erased).
    assert(left.type == kVector);
    auto li = left.values;
    auto out = li;
    auto le = li + left.numValues;
    auto ptr = impl(right);
    const Node * node = nullptr;
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
}

//===========================================================================
static void eraIter(Node & left, const Node & right) {
    assert(right.type == kVector);
    auto ri = right.values;
    auto re = ri + right.numValues;
    for (; ri != re; ++ri)
        impl(left)->erase(left, *ri);
}

//===========================================================================
static void eraVec(Node & left, const Node & right) {
    auto le = set_difference(
        left.values, left.values + left.numValues,
        right.values, right.values + right.numValues,
        left.values
    );
    left.numValues = uint16_t(le - left.values);
}

//===========================================================================
static void eraBitmap(Node & left, const Node & right) {
    auto li = (uint64_t *) left.values;
    auto le = li + kDataSize / sizeof(*li);
    auto ri = (uint64_t *) right.values;
    left.numValues = 0;
    for (; li != le; ++li, ++ri) {
        if (*li &= ~*ri)
            left.numValues += 1;
    }
}

//===========================================================================
static void eraMeta(Node & left, const Node & right) {
    auto li = left.nodes;
    auto le = li + left.numValues;
    auto ri = right.nodes;
    auto re = ri + right.numValues;
    for (; li != le && ri != re; ++li, ++ri)
        erase(*li, *ri);
}

//===========================================================================
static void erase(Node & left, const Node & right) {
    using EraseFn = void(Node & left, const Node & right);
    EraseFn * functs[][kNodeTypes] = {
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
*   intersect(Node &, const Node &)
*
***/

static void intersect(Node & left, const Node & right);

//===========================================================================
static void isecError(Node & left, const Node & right) {
    logMsgCrash() << "intersect: incompatible node types, " << left.type
        << ", " << right.type;
}

//===========================================================================
static void isecSkip(Node & left, const Node & right)
{}

//===========================================================================
static void isecEmpty(Node & left, const Node & right) {
    eraEmpty(left, right);
}

//===========================================================================
static void isecCopy(Node & left, const Node & right) {
    insCopy(left, right);
}

//===========================================================================
static void isecFind(Node & left, const Node & right) {
    // Go through values of left vector and remove the ones that aren't
    // found in right node.
    assert(left.type == kVector);
    auto li = left.values;
    auto out = li;
    auto le = li + left.numValues;
    auto ptr = impl(right);
    const Node * node = nullptr;
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
}

//===========================================================================
static void isecSFind(Node & left, const Node & right) {
    Node tmp;
    isecCopy(tmp, right);
    swap(left, tmp);
    isecFind(left, tmp);
}

//===========================================================================
static void isecVec(Node & left, const Node & right) {
    auto le = set_intersection(
        left.values, left.values + left.numValues,
        right.values, right.values + right.numValues,
        left.values
    );
    left.numValues = uint16_t(le - left.values);
}

//===========================================================================
static void isecBitmap(Node & left, const Node & right) {
    auto li = (uint64_t *) left.values;
    auto le = li + kDataSize / sizeof(*li);
    auto ri = (uint64_t *) right.values;
    left.numValues = 0;
    for (; li != le; ++li, ++ri) {
        if (*li &= *ri)
            left.numValues += 1;
    }
}

//===========================================================================
static void isecMeta(Node & left, const Node & right) {
    auto li = left.nodes;
    auto le = li + left.numValues;
    auto ri = right.nodes;
    auto re = ri + right.numValues;
    for (; li != le && ri != re; ++li, ++ri)
        intersect(*li, *ri);
}

//===========================================================================
static void intersect(Node & left, const Node & right) {
    using IsectFn = void(Node & left, const Node & right);
    IsectFn * functs[][kNodeTypes] = {
    // LEFT                         RIGHT
    //             empty      full      vector     bitmap      meta
    /* empty  */ { isecSkip,  isecSkip, isecSkip,  isecSkip,   isecSkip },
    /* full   */ { isecEmpty, isecSkip, isecCopy,  isecCopy,   isecCopy  },
    /* vector */ { isecEmpty, isecSkip, isecVec,   isecFind,   isecFind  },
    /* bitmap */ { isecEmpty, isecSkip, isecSFind, isecBitmap, isecError },
    /* meta   */ { isecEmpty, isecSkip, isecSFind, isecError,  isecMeta  },
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
    auto re = ri + right.numValues;
    for (; li != le && ri != re; ++li, ++ri)
        intersect(*li, move(*ri));
}

//===========================================================================
static void intersect(Node & left, Node && right) {
    using IsectFn = void(Node & left, Node && right);
    IsectFn * functs[][kNodeTypes] = {
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
UnsignedSet::UnsignedSet(UnsignedSet && from) {
    swap(from);
}

//===========================================================================
UnsignedSet::UnsignedSet(const UnsignedSet & from) {
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
UnsignedSet & UnsignedSet::operator=(UnsignedSet && from) {
    clear();
    swap(from);
    return *this;
}

//===========================================================================
UnsignedSet & UnsignedSet::operator=(const UnsignedSet & from) {
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
    impl(m_node)->init(m_node, true);
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
void UnsignedSet::assign(const UnsignedSet & from) {
    clear();
    insert(from);
}

//===========================================================================
void UnsignedSet::assign(std::string_view src) {
    clear();
    insert(src);
}

//===========================================================================
void UnsignedSet::insert(unsigned value) {
    impl(m_node)->insert(m_node, value);
}

//===========================================================================
void UnsignedSet::insert(UnsignedSet && other) {
    ::insert(m_node, move(other.m_node));
}

//===========================================================================
void UnsignedSet::insert(const UnsignedSet & other) {
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
void UnsignedSet::erase(unsigned value) {
    impl(m_node)->erase(m_node, value);
}

//===========================================================================
void UnsignedSet::erase(UnsignedSet::iterator where) {
    assert(where);
    erase(*where);
}

//===========================================================================
void UnsignedSet::erase(const UnsignedSet & other) {
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
void UnsignedSet::intersect(const UnsignedSet & other) {
    ::intersect(m_node, other.m_node);
}

//===========================================================================
void UnsignedSet::swap(UnsignedSet & other) {
    ::swap(m_node, other.m_node);
}

//===========================================================================
int UnsignedSet::compare(const UnsignedSet & right) const {
    return ::compare(m_node, right.m_node);
}

//===========================================================================
bool UnsignedSet::includes(const UnsignedSet & other) const {
    assert(!"not implemented");
    return false;
}

//===========================================================================
bool UnsignedSet::intersects(const UnsignedSet & other) const {
    assert(!"not implemented");
    return false;
}

//===========================================================================
bool UnsignedSet::operator==(const UnsignedSet & right) const {
    return compare(right) == 0;
}

//===========================================================================
bool UnsignedSet::operator!=(const UnsignedSet & right) const {
    return compare(right) != 0;
}

//===========================================================================
bool UnsignedSet::operator<(const UnsignedSet & right) const {
    return compare(right) < 0;
}

//===========================================================================
bool UnsignedSet::operator>(const UnsignedSet & right) const {
    return compare(right) > 0;
}

//===========================================================================
bool UnsignedSet::operator<=(const UnsignedSet & right) const {
    return compare(right) <= 0;
}

//===========================================================================
bool UnsignedSet::operator>=(const UnsignedSet & right) const {
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
void UnsignedSet::iInsert(const unsigned * first, const unsigned * last) {
    impl(m_node)->insert(m_node, first, last);
}


/****************************************************************************
*
*   UnsignedSet::Iterator
*
***/

//===========================================================================
UnsignedSet::Iterator::Iterator(const Node * node)
    : Iterator{node, absBase(*node), node->depth}
{}

//===========================================================================
UnsignedSet::Iterator::Iterator(
    const Node * node,
    value_type value,
    unsigned minDepth
)
    : m_node{node}
    , m_value{value}
    , m_minDepth{minDepth}
{
    if (!m_node) {
        assert(!m_value);
    } else {
        if (!impl(*m_node)->findFirst(&m_node, &m_value, *m_node, m_value))
            *this = {};
    }
}

//===========================================================================
bool UnsignedSet::Iterator::operator!= (const Iterator & right) const {
    return m_value != right.m_value || m_node != right.m_node;
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
bool UnsignedSet::RangeIterator::operator!= (
    const RangeIterator & right
) const {
    return m_value != right.m_value;
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
ostream & Dim::operator<<(ostream & os, const UnsignedSet & right) {
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
