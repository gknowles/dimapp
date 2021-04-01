// Copyright Glen Knowles 2017 - 2021.
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
    hammingWeight(pow2Ceil(kDataSize / sizeof Node + 1) / 2 - 1);
const unsigned kMaxDepth =
    (kBitWidth - kLeafBits + kStepBits - 1) / kStepBits;
static_assert(UnsignedSet::kBaseBits + kLeafBits >= kBitWidth);

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

constexpr unsigned relBase(unsigned value, unsigned depth) {
    return (value & ~valueMask(depth)) >> 8;
}
constexpr unsigned relValue(unsigned value, unsigned depth) {
    return value & valueMask(depth);
}

constexpr uint16_t numNodes(unsigned depth) {
    uint16_t ret = 0;
    if (depth) {
        ret = (uint16_t) 1
            << hammingWeight(valueMask(depth) ^ valueMask(depth - 1));
    }
    return ret;
}
constexpr unsigned nodePos(unsigned value, unsigned depth) {
    return relValue(value, depth) / (valueMask(depth + 1) + 1);
}

constexpr unsigned absBase(unsigned value, unsigned depth) {
    return value & ~valueMask(depth);
}
constexpr unsigned absFinal(unsigned value, unsigned depth) {
    return absBase(value, depth) + valueMask(depth);
}

constexpr unsigned absBase(const Node & node) {
    return node.base << 8;
}
constexpr unsigned absFinal(const Node & node) {
    return absBase(node) + valueMask(node.depth);
}
constexpr unsigned absSize(const Node & node) {
    return valueMask(node.depth) + 1;
}

enum UnsignedSet::Node::Type : int {
    kEmpty,         // contains no values
    kFull,          // contains all values in node's domain
    kVector,        // has vector of values
    kBitmap,        // has bitmap covering all of node's possible values
    kMeta,          // vector of nodes
    kNodeTypes,
    kMetaParent,    // link to parent meta node
};
static_assert(Node::kMetaParent < 1 << UnsignedSet::kTypeBits);

namespace {

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
    virtual bool erase(Node & node, unsigned value) = 0;

    virtual size_t size(const Node & node) const = 0;

    // Find value equal to or greater than "base", returns false if no such
    // value exists.
    virtual bool findFirst(
        const Node ** onode,
        unsigned * ovalue,
        const Node & node,
        unsigned base
    ) const = 0;
    // Find value equal to or less than "base", return false is no such value.
    virtual bool findLast(
        const Node ** onode,
        unsigned * ovalue,
        const Node & node,
        unsigned base
    ) const = 0;

    // [base] is empty - return true
    // [start of node, base] is full - update onode & ovalue, return false
    // otherwise - update onode & ovalue, return true
    virtual bool firstContiguous(
        const Node ** onode,
        unsigned * ovalue,
        const Node & node,
        unsigned base
    ) const = 0;
    // [base] is empty - return true
    // [base, end of node] is full - update onode & ovalue, return false
    // otherwise - update onode & ovalue, return true
    virtual bool lastContiguous(
        const Node ** onode,
        unsigned * ovalue,
        const Node & node,
        unsigned base
    ) const = 0;
};

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
    bool erase(Node & node, unsigned value) override;

    size_t size(const Node & node) const override;
    bool findFirst(
        const Node ** onode,
        unsigned * ovalue,
        const Node & node,
        unsigned base
    ) const override;
    bool findLast(
        const Node ** onode,
        unsigned * ovalue,
        const Node & node,
        unsigned base
    ) const override;
    bool firstContiguous(
        const Node ** onode,
        unsigned * ovalue,
        const Node & node,
        unsigned base
    ) const override;
    bool lastContiguous(
        const Node ** onode,
        unsigned * ovalue,
        const Node & node,
        unsigned base
    ) const override;
};

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
    bool erase(Node & node, unsigned value) override;

    size_t size(const Node & node) const override;
    bool findFirst(
        const Node ** onode,
        unsigned * ovalue,
        const Node & node,
        unsigned base
    ) const override;
    bool findLast(
        const Node ** onode,
        unsigned * ovalue,
        const Node & node,
        unsigned base
    ) const override;
    bool firstContiguous(
        const Node ** onode,
        unsigned * ovalue,
        const Node & node,
        unsigned base
    ) const override;
    bool lastContiguous(
        const Node ** onode,
        unsigned * ovalue,
        const Node & node,
        unsigned base
    ) const override;
};

struct VectorImpl final : IImplBase {
    static constexpr size_t maxValues() {
        return kDataSize / sizeof *Node::values;
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
    bool erase(Node & node, unsigned value) override;

    size_t size(const Node & node) const override;
    bool findFirst(
        const Node ** onode,
        unsigned * ovalue,
        const Node & node,
        unsigned base
    ) const override;
    bool findLast(
        const Node ** onode,
        unsigned * ovalue,
        const Node & node,
        unsigned base
    ) const override;
    bool firstContiguous(
        const Node ** onode,
        unsigned * ovalue,
        const Node & node,
        unsigned base
    ) const override;
    bool lastContiguous(
        const Node ** onode,
        unsigned * ovalue,
        const Node & node,
        unsigned base
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
    void init(Node & node, const Node & from) override;
    void destroy(Node & node) override;
    bool insert(Node & node, unsigned value) override;
    void insert(
        Node & node,
        const unsigned * first,
        const unsigned * last
    ) override;
    bool erase(Node & node, unsigned value) override;

    size_t size(const Node & node) const override;
    bool findFirst(
        const Node ** onode,
        unsigned * ovalue,
        const Node & node,
        unsigned base
    ) const override;
    bool findLast(
        const Node ** onode,
        unsigned * ovalue,
        const Node & node,
        unsigned base
    ) const override;
    bool firstContiguous(
        const Node ** onode,
        unsigned * ovalue,
        const Node & node,
        unsigned base
    ) const override;
    bool lastContiguous(
        const Node ** onode,
        unsigned * ovalue,
        const Node & node,
        unsigned base
    ) const override;
};

struct MetaImpl final : IImplBase {
    static constexpr size_t maxNodes() {
        return kDataSize / sizeof *Node::nodes;
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
    bool erase(Node & node, unsigned value) override;

    size_t size(const Node & node) const override;
    bool findFirst(
        const Node ** onode,
        unsigned * ovalue,
        const Node & node,
        unsigned base
    ) const override;
    bool findLast(
        const Node ** onode,
        unsigned * ovalue,
        const Node & node,
        unsigned base
    ) const override;
    bool firstContiguous(
        const Node ** onode,
        unsigned * ovalue,
        const Node & node,
        unsigned base
    ) const override;
    bool lastContiguous(
        const Node ** onode,
        unsigned * ovalue,
        const Node & node,
        unsigned base
    ) const override;

private:
    unsigned nodePos(const Node & node, unsigned value) const;
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
static_assert(size(s_impls) == Node::kNodeTypes);

//===========================================================================
static IImplBase * impl(const Node & node) {
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
    assert(node.type == Node::kEmpty);
    assert(!full);
    node.numBytes = 0;
    node.numValues = 0;
    node.values = nullptr;
}

//===========================================================================
void EmptyImpl::init(Node & node, const Node & from) {
    assert(node.type == Node::kEmpty);
    assert(from.type == Node::kEmpty);
    node = from;
}

//===========================================================================
void EmptyImpl::destroy(Node & node)
{}

//===========================================================================
bool EmptyImpl::insert(Node & node, unsigned value) {
    assert(node.type == Node::kEmpty);
    assert(relBase(value, node.depth) == node.base);
    destroy(node);
    node.type = Node::kVector;
    s_vectorImpl.init(node, false);
    s_vectorImpl.insert(node, value);
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
            s_vectorImpl.insert(node, first, last);
    }
}

//===========================================================================
bool EmptyImpl::erase(Node & node, unsigned value) {
    assert(relBase(value, node.depth) == node.base);
    return false;
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
    unsigned base
) const {
    return false;
}

//===========================================================================
bool EmptyImpl::findLast(
    const Node ** onode,
    unsigned * ovalue,
    const Node & node,
    unsigned base
) const {
    return false;
}

//===========================================================================
bool EmptyImpl::firstContiguous(
    const Node ** onode,
    unsigned * ovalue,
    const Node & node,
    unsigned base
) const {
    return true;
}

//===========================================================================
bool EmptyImpl::lastContiguous(
    const Node ** onode,
    unsigned * ovalue,
    const Node & node,
    unsigned base
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
    assert(node.type == Node::kFull);
    assert(full);
    node.numBytes = 0;
    node.numValues = 0;
    node.values = nullptr;
}

//===========================================================================
void FullImpl::init(Node & node, const Node & from) {
    assert(node.type == Node::kFull);
    assert(from.type == Node::kFull);
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
bool FullImpl::erase(Node & node, unsigned value) {
    assert(relBase(value, node.depth) == node.base);
    destroy(node);
    if (node.depth == maxDepth()) {
        node.type = Node::kBitmap;
        s_bitmapImpl.init(node, true);
        return s_bitmapImpl.erase(node, value);
    } else {
        node.type = Node::kMeta;
        s_metaImpl.init(node, true);
        return s_metaImpl.erase(node, value);
    }
}

//===========================================================================
size_t FullImpl::size(const Node & node) const {
    return absSize(node);
}

//===========================================================================
bool FullImpl::findFirst(
    const Node ** onode,
    unsigned * ovalue,
    const Node & node,
    unsigned base
) const {
    assert(relBase(base, node.depth) == node.base);
    *onode = &node;
    *ovalue = base;
    return true;
}

//===========================================================================
bool FullImpl::findLast(
    const Node ** onode,
    unsigned * ovalue,
    const Node & node,
    unsigned base
) const {
    assert(relBase(base, node.depth) == node.base);
    *onode = &node;
    *ovalue = base;
    return true;
}

//===========================================================================
bool FullImpl::firstContiguous(
    const Node ** onode,
    unsigned * ovalue,
    const Node & node,
    unsigned base
) const {
    assert(relBase(base, node.depth) == node.base);
    *onode = &node;
    *ovalue = absBase(node);
    return false;
}

//===========================================================================
bool FullImpl::lastContiguous(
    const Node ** onode,
    unsigned * ovalue,
    const Node & node,
    unsigned base
) const {
    assert(relBase(base, node.depth) == node.base);
    *onode = &node;
    *ovalue = absFinal(node);
    return false;
}


/****************************************************************************
*
*   VectorImpl
*
***/

//===========================================================================
void VectorImpl::init(Node & node, bool full) {
    assert(node.type == Node::kVector);
    assert(!full);
    node.numBytes = kDataSize;
    node.numValues = 0;
    node.values = (unsigned *) malloc(node.numBytes);
}

//===========================================================================
void VectorImpl::init(Node & node, const Node & from) {
    assert(node.type == Node::kVector);
    assert(from.type == Node::kVector);
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
    const unsigned * first,
    const unsigned * last
) {
    while (first != last) {
        if (node.type != Node::kVector)
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
        node.type = Node::kEmpty;
        s_emptyImpl.init(node, false);
    }
    return true;
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
    unsigned base
) const {
    assert(relBase(base, node.depth) == node.base);
    auto last = node.values + node.numValues;
    auto ptr = lower_bound(node.values, last, base);
    if (ptr != last) {
        *onode = &node;
        *ovalue = *ptr;
        return true;
    }
    return false;
}

//===========================================================================
bool VectorImpl::findLast(
    const Node ** onode,
    unsigned * ovalue,
    const Node & node,
    unsigned base
) const {
    assert(relBase(base, node.depth) == node.base);
    if (base < *node.values) {
        return false;
    } else if (base == *node.values) {
        *ovalue = base;
    } else {
        // base > *node.values
        auto last = node.values + node.numValues;
        auto ptr = lower_bound(node.values + 1, last, base);
        *ovalue = ptr[-1];
    }
    *onode = &node;
    return true;
}

//===========================================================================
bool VectorImpl::firstContiguous(
    const Node ** onode,
    unsigned * ovalue,
    const Node & node,
    unsigned base
) const {
    assert(relBase(base, node.depth) == node.base);
    auto last = node.values + node.numValues;
    auto ptr = lower_bound(node.values, last, base);
    auto val = base;
    if (ptr == last || *ptr != val) {
        // Base not found, return that search is done, but leave onode and
        // ovalue unchanged.
        return true;
    }
    *onode = &node;
    while (ptr != node.values) {
        ptr -= 1;
        val -= 1;
        if (*ptr != val) {
            *ovalue = val + 1;
            return true;
        }
    }
    *ovalue = val;
    if (val > absBase(node))
        return true;

    // May extend into preceding node.
    return false;
}

//===========================================================================
bool VectorImpl::lastContiguous(
    const Node ** onode,
    unsigned * ovalue,
    const Node & node,
    unsigned base
) const {
    assert(relBase(base, node.depth) == node.base);
    auto last = node.values + node.numValues;
    auto ptr = lower_bound(node.values, last, base);
    auto val = base;
    if (ptr == last || *ptr != val) {
        // Base not found, return that search is done, but leave onode and
        // ovalue unchanged.
        return true;
    }
    *onode = &node;
    while (++ptr != last) {
        val += 1;
        if (*ptr != val) {
            *ovalue = val - 1;
            return true;
        }
    }
    *ovalue = val;
    if (val < absFinal(node))
        return true;

    // May extend into following node.
    return false;
}

//===========================================================================
void VectorImpl::convertAndInsert(Node & node, unsigned value) {
    Node tmp{node};
    auto ptr = tmp.values;
    auto last = ptr + tmp.numValues;
    if (tmp.depth == maxDepth()) {
        node.type = Node::kBitmap;
        s_bitmapImpl.init(node, false);
        s_bitmapImpl.insert(node, ptr, last);
        s_bitmapImpl.insert(node, value);
    } else {
        node.type = Node::kMeta;
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
    assert(node.type == Node::kBitmap);
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
void BitmapImpl::init(Node & node, const Node & from) {
    assert(node.type == Node::kBitmap);
    assert(from.type == Node::kBitmap);
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
    assert(node.type == Node::kBitmap);
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
    node.type = Node::kFull;
    s_fullImpl.init(node, true);
    return true;
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
    node.type = Node::kEmpty;
    s_emptyImpl.init(node, false);
    return true;
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
    unsigned key
) const {
    assert(relBase(key, node.depth) == node.base);
    auto base = (uint64_t *) node.values;
    auto rel = relValue(key, node.depth);
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
bool BitmapImpl::findLast(
    const Node ** onode,
    unsigned * ovalue,
    const Node & node,
    unsigned key
) const {
    assert(relBase(key, node.depth) == node.base);
    auto base = (uint64_t *) node.values;
    auto rel = relValue(key, node.depth);
    assert(rel < numBits());
    BitView bits{base, numInt64s()};
    if (auto pos = bits.rfind(rel); pos != bits.npos) {
        *onode = &node;
        *ovalue = (unsigned) pos + absBase(node);
        return true;
    }
    return false;
}

//===========================================================================
bool BitmapImpl::firstContiguous(
    const Node ** onode,
    unsigned * ovalue,
    const Node & node,
    unsigned key
) const {
    assert(relBase(key, node.depth) == node.base);
    auto base = (uint64_t *) node.values;
    auto rel = relValue(key, node.depth);
    assert(rel < numBits());
    BitView bits{base, numInt64s()};
    if (auto pos = bits.rfindZero(rel); pos != bits.npos) {
        if (auto val = (unsigned) pos + absBase(node); val != key) {
            *onode = &node;
            *ovalue = val + 1;
        }
        return true;
    }
    *onode = &node;
    *ovalue = absBase(node);
    return false;
}

//===========================================================================
bool BitmapImpl::lastContiguous(
    const Node ** onode,
    unsigned * ovalue,
    const Node & node,
    unsigned key
) const {
    assert(relBase(key, node.depth) == node.base);
    auto base = (uint64_t *) node.values;
    auto rel = relValue(key, node.depth);
    assert(rel < numBits());
    BitView bits{base, numInt64s()};
    if (auto pos = bits.findZero(rel); pos != bits.npos) {
        if (auto val = (unsigned) pos + absBase(node); val != key) {
            *onode = &node;
            *ovalue = val - 1;
        }
        return true;
    }
    *onode = &node;
    *ovalue = absFinal(node);
    return false;
}


/****************************************************************************
*
*   MetaImpl
*
***/

//===========================================================================
void MetaImpl::init(Node & node, bool full) {
    assert(node.type == Node::kMeta);
    node.numValues = numNodes(node.depth + 1);
    node.numBytes = (node.numValues + 1) * sizeof *node.nodes;
    node.nodes = (Node *) malloc(node.numBytes);
    assert(node.nodes);
    auto nptr = node.nodes;
    auto nlast = node.nodes + node.numValues;
    Node def;
    def.type = full ? Node::kFull : Node::kEmpty;
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
    nlast->type = Node::kMetaParent;
    nlast->nodes = &node;
}

//===========================================================================
void MetaImpl::init(Node & node, const Node & from) {
    assert(node.type == Node::kMeta);
    assert(from.type == Node::kMeta);
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
unsigned MetaImpl::nodePos(const Node & node, unsigned value) const {
    assert(relBase(value, node.depth) == node.base);
    auto pos = ::nodePos(value, node.depth);
    assert(pos < node.numValues);
    return pos;
}

//===========================================================================
bool MetaImpl::insert(Node & node, unsigned value) {
    auto pos = nodePos(node, value);
    auto & rnode = node.nodes[pos];
    if (!impl(rnode)->insert(rnode, value))
        return false;
    if (rnode.type != Node::kFull)
        return true;
    for (unsigned i = 0; i < node.numValues; ++i) {
        if (node.nodes[i].type != Node::kFull)
            return true;
    }
    // convert to full
    destroy(node);
    node.type = Node::kFull;
    s_fullImpl.init(node, true);
    return true;
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
bool MetaImpl::erase(Node & node, unsigned value) {
    auto pos = nodePos(node, value);
    auto & rnode = node.nodes[pos];
    if (!impl(rnode)->erase(rnode, value))
        return false;
    if (rnode.type != Node::kEmpty)
        return true;
    for (unsigned i = 0; i < node.numValues; ++i) {
        if (node.nodes[i].type != Node::kEmpty)
            return true;
    }
    // convert to empty
    destroy(node);
    node.type = Node::kEmpty;
    s_emptyImpl.init(node, false);
    return true;
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
    unsigned key
) const {
    auto pos = nodePos(node, key);
    auto ptr = node.nodes + pos;
    auto last = node.nodes + node.numValues;
    for (;;) {
        if (impl(*ptr)->findFirst(onode, ovalue, *ptr, key))
            return true;
        if (++ptr == last)
            return false;
        key = absFinal(*ptr);
    }
}

//===========================================================================
bool MetaImpl::findLast(
    const Node ** onode,
    unsigned * ovalue,
    const Node & node,
    unsigned key
) const {
    auto pos = nodePos(node, key);
    auto ptr = node.nodes + pos;
    for (;;) {
        if (impl(*ptr)->findLast(onode, ovalue, *ptr, key))
            return true;
        if (ptr == node.nodes)
            return false;
        ptr -= 1;
        key = absBase(*ptr);
    }
}

//===========================================================================
bool MetaImpl::firstContiguous(
    const Node ** onode,
    unsigned * ovalue,
    const Node & node,
    unsigned key
) const {
    auto pos = nodePos(node, key);
    auto ptr = (const Node *) node.nodes + pos;
    auto last = node.nodes + node.numValues;
    for (;;) {
        if (impl(*ptr)->firstContiguous(onode, ovalue, *ptr, key))
            return true;
        *onode = ptr;
        *ovalue = absBase(*ptr);
        if (++ptr == last)
            return false;
        key = absFinal(*ptr);
    }
}

//===========================================================================
bool MetaImpl::lastContiguous(
    const Node ** onode,
    unsigned * ovalue,
    const Node & node,
    unsigned key
) const {
    auto pos = nodePos(node, key);
    auto ptr = (const Node *) node.nodes + pos;
    auto last = node.nodes + node.numValues;
    for (;;) {
        if (impl(*ptr)->lastContiguous(onode, ovalue, *ptr, key))
            return true;
        *onode = ptr;
        *ovalue = absFinal(*ptr);
        if (++ptr == last)
            return false;
        key = absBase(*ptr);
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

static int compare(const Node & left, const Node & right);

//===========================================================================
static int cmpError(const Node & left, const Node & right) {
    logMsgFatal() << "compare: incompatible node types, " << left.type
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
    auto i = UnsignedSet::iterator::makeFirst(&right);
    auto ri = UnsignedSet::range_iterator{i};
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
    auto li = UnsignedSet::iterator::makeFirst(&left);
    auto ri = UnsignedSet::iterator::makeFirst(&right);
    for (;; ++li, ++ri) {
        if (!li)
            return !ri ? 0 : -2;
        if (!ri)
            return 2;
        if (*li != *ri)
            return *li > *ri ? 1 : -1;
    }
}

//===========================================================================
static int cmpVec(const Node & left, const Node & right) {
    auto li = left.values;
    auto le = li + left.numValues;
    auto ri = right.values;
    auto re = ri + right.numValues;
    for (;; ++li, ++ri) {
        if (li == le)
            return ri == re ? 0 : -2;
        if (ri == re)
            return 2;
        if (*li != *ri)
            return *li > *ri ? 1 : -1;
    }
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
static int cmpBit(const Node & left, const Node & right) {
    auto li = (uint64_t *) left.values;
    auto le = li + kDataSize / sizeof *li;
    auto ri = (uint64_t *) right.values;
    auto re = ri + kDataSize / sizeof *ri;
    for (;; ++li, ++ri) {
        if (li == le)
            return ri == re ? 0 : -2;
        if (ri == re)
            return 2;
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
}

//===========================================================================
static int cmpMeta(const Node & left, const Node & right) {
    auto li = left.nodes;
    auto le = li + left.numValues;
    auto ri = right.nodes;
    auto re = ri + right.numValues;
    for (;; ++li, ++ri) {
        if (li == le)
            return ri == re ? 0 : -2;
        if (ri == re)
            return 2;
        if (int rc = compare(*li, *ri)) {
            if (rc == -2) {
                while (++li != le) {
                    if (li->type != Node::kEmpty)
                        return 1;
                }
            } else if (rc == 2) {
                while (++ri != re) {
                    if (ri->type != Node::kEmpty)
                        return -1;
                }
            }
            return rc;
        }
    }
}

//===========================================================================
static int compare(const Node & left, const Node & right) {
    using CompareFn = int(const Node & left, const Node & right);
    constexpr static CompareFn * functs[][Node::kNodeTypes] = {
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
*   contains(const Node &, const Node &)
*
***/

static bool contains(const Node & left, const Node & right);

//===========================================================================
static bool conError(const Node & left, const Node & right) {
    logMsgFatal() << "contains: incompatible node types, " << left.type
        << ", " << right.type;
    return 0;
}

//===========================================================================
static bool conTrue(const Node & left, const Node & right) { return true; }
static bool conFalse(const Node & left, const Node & right) { return false; }

//===========================================================================
static bool conRVec(const Node & left, const Node & right) {
    auto ri = right.values;
    auto re = ri + right.numValues;
    const Node * onode;
    unsigned ovalue;

    for (;;) {
        if (!impl(left)->findFirst(&onode, &ovalue, left, *ri))
            return false;
        if (*ri != ovalue || ++ri == re)
            return true;
    }
}

//===========================================================================
static bool conLVec(const Node & left, const Node & right) {
    auto li = left.values;
    auto le = li + left.numValues;
    const Node * onode;
    unsigned ovalue;

    impl(right)->findFirst(&onode, &ovalue, right, absBase(right));
    for (;;) {
        li = lower_bound(li, le, ovalue);
        if (li == le || *li != ovalue)
            return false;
        if (!impl(right)->findFirst(&onode, &ovalue, right, ovalue + 1))
            return true;
    }
}

//===========================================================================
static bool conVec(const Node & left, const Node & right) {
    if (left.numValues < right.numValues)
        return false;

    auto li = left.values;
    auto le = li + left.numValues;
    auto ri = right.values;
    auto re = ri + right.numValues;

    for (;;) {
        li = lower_bound(li, le, *ri);
        if (li == le || *li != *ri)
            return false;
        if (++ri == re)
            return true;
    }
}

//===========================================================================
static bool conBit(const Node & left, const Node & right) {
    auto li = (uint64_t *) left.values;
    auto le = li + kDataSize / sizeof *li;
    auto ri = (uint64_t *) right.values;

    for (;;) {
        if (*ri != (*li & *ri))
            return false;
        if (++li == le)
            return true;
        ++ri;
    }
}

//===========================================================================
static bool conMeta(const Node & left, const Node & right) {
    auto li = left.nodes;
    auto le = li + left.numValues;
    auto ri = right.nodes;
    for (; li != le; ++li, ++ri) {
        if (!contains(*li, *ri))
            return false;
    }
    return true;
}

//===========================================================================
static bool contains(const Node & left, const Node & right) {
    using Fn = bool(const Node & left, const Node & right);
    static Fn * const functs[][Node::kNodeTypes] = {
    // LEFT                         RIGHT
    //             empty    full      vector    bitmap    meta
    /* empty  */ { conTrue, conFalse, conFalse, conFalse, conFalse },
    /* full   */ { conTrue, conTrue,  conTrue,  conTrue,  conTrue  },
    /* vector */ { conTrue, conFalse, conVec,   conLVec,  conLVec  },
    /* bitmap */ { conTrue, conFalse, conRVec,  conBit,   conError },
    /* meta   */ { conTrue, conFalse, conRVec,  conError, conMeta  },
    };
    assert(!"not tested");
    return functs[left.type][right.type](left, right);
}


/****************************************************************************
*
*   intersects(const Node &, const Node &)
*
***/

static bool intersects(const Node & left, const Node & right);

//===========================================================================
static bool isecError(const Node & left, const Node & right) {
    logMsgFatal() << "intersects: incompatible node types, " << left.type
        << ", " << right.type;
    return false;
}

//===========================================================================
static bool isecTrue(const Node & left, const Node & right) { return true; }
static bool isecFalse(const Node & left, const Node & right) { return false; }

//===========================================================================
static bool isecRVec(const Node & left, const Node & right) {
    auto ri = right.values;
    auto re = ri + right.numValues;
    const Node * onode;
    unsigned ovalue;

    for (;;) {
        if (impl(left)->findFirst(&onode, &ovalue, left, *ri)
            && *ri == ovalue
        ) {
            return true;
        }
        if (++ri == re)
            return false;
    }
}

//===========================================================================
static bool isecLVec(const Node & left, const Node & right) {
    return isecRVec(right, left);
}

//===========================================================================
static bool isecVec(const Node & left, const Node & right) {
    auto li = left.values;
    auto le = li + left.numValues;
    auto ri = right.values;
    auto re = ri + right.numValues;

    for (;;) {
        if (*li == *ri)
            return true;
        while (*li < *ri) {
            if (++li == le)
                return false;
        }
        if (*li == *ri)
            return true;
        for (;;) {
            if (++ri == re)
                return false;
            if (*ri >= *li)
                break;
        }
    }
}

//===========================================================================
static bool isecBit(const Node & left, const Node & right) {
    auto li = (uint64_t *) left.values;
    auto le = li + kDataSize / sizeof *li;
    auto ri = (uint64_t *) right.values;

    for (;;) {
        if (*li & *ri)
            return true;
        if (++li == le)
            return false;
        ++ri;
    }
}

//===========================================================================
static bool isecMeta(const Node & left, const Node & right) {
    auto li = left.nodes;
    auto le = li + left.numValues;
    auto ri = right.nodes;
    for (; li != le; ++li, ++ri) {
        if (intersects(*li, *ri))
            return true;
    }
    return false;
}

//===========================================================================
static bool intersects(const Node & left, const Node & right) {
    using Fn = bool(const Node & left, const Node & right);
    static Fn * const functs[][Node::kNodeTypes] = {
    // LEFT                         RIGHT
    //             empty      full       vector     bitmap     meta
    /* empty  */ { isecFalse, isecFalse, isecFalse, isecFalse, isecFalse },
    /* full   */ { isecFalse, isecTrue,  isecTrue,  isecTrue,  isecTrue  },
    /* vector */ { isecFalse, isecTrue,  isecVec,   isecLVec,  isecLVec  },
    /* bitmap */ { isecFalse, isecTrue,  isecRVec,  isecBit,   isecError },
    /* meta   */ { isecFalse, isecTrue,  isecRVec,  isecError, isecMeta  },
    };
    assert(!"not tested");
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
    logMsgFatal() << "insert: incompatible node types, " << left.type
        << ", " << right.type;
}

//===========================================================================
static void insSkip(Node & left, const Node & right)
{}

//===========================================================================
static void insFull(Node & left, const Node & right) {
    impl(left)->destroy(left);
    left.type = Node::kFull;
    s_fullImpl.init(left, true);
}

//===========================================================================
static void insCopy(Node & left, const Node & right) {
    impl(left)->destroy(left);
    left.type = right.type;
    impl(left)->init(left, right);
}

//===========================================================================
static void insIter(Node & left, const Node & right) {
    auto ri = UnsignedSet::iterator::makeFirst(&right);
    for (; ri; ++ri)
        impl(left)->insert(left, *ri);
}

//===========================================================================
static void insRIter(Node & left, const Node & right) {
    Node tmp;
    insCopy(tmp, right);
    swap(left, tmp);
    insIter(left, move(tmp));
    impl(tmp)->destroy(tmp);
}

//===========================================================================
static void insVec(Node & left, const Node & right) {
    assert(right.type == Node::kVector);
    auto ri = right.values;
    auto re = ri + right.numValues;
    s_vectorImpl.insert(left, ri, re);
}

//===========================================================================
static void insBit(Node & left, const Node & right) {
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
static void insMeta(Node & left, const Node & right) {
    auto li = left.nodes;
    auto le = li + left.numValues;
    auto ri = right.nodes;
    for (; li != le; ++li, ++ri) {
        insert(*li, *ri);
        if (li->type != Node::kFull)
            goto NOT_FULL;
    }
    // Convert to full node.
    insFull(left, right);
    return;

NOT_FULL:
    ++li, ++ri;
    for (; li != le; ++li, ++ri)
        insert(*li, *ri);
}

//===========================================================================
static void insert(Node & left, const Node & right) {
    using InsertFn = void(Node & left, const Node & right);
    static InsertFn * const functs[][Node::kNodeTypes] = {
    // LEFT                         RIGHT
    //             empty    full     vector   bitmap     meta
    /* empty  */ { insSkip, insFull, insCopy, insCopy,   insCopy  },
    /* full   */ { insSkip, insSkip, insSkip, insSkip,   insSkip  },
    /* vector */ { insSkip, insFull, insVec,  insRIter,  insRIter },
    /* bitmap */ { insSkip, insFull, insIter, insBit,    insError },
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
static void insBit(Node & left, Node && right) {
    insBit(left, right);
}

//===========================================================================
static void insMeta(Node & left, Node && right) {
    auto li = left.nodes;
    auto le = li + left.numValues;
    auto ri = right.nodes;
    for (; li != le; ++li, ++ri) {
        insert(*li, move(*ri));
        if (li->type != Node::kFull)
            goto NOT_FULL;
    }
    insFull(left, move(right));
    return;

NOT_FULL:
    ++li, ++ri;
    for (; li != le; ++li, ++ri)
        insert(*li, move(*ri));
}

//===========================================================================
static void insert(Node & left, Node && right) {
    using InsertFn = void(Node & left, Node && right);
    static InsertFn * const functs[][Node::kNodeTypes] = {
    // LEFT                         RIGHT
    //             empty    full     vector   bitmap    meta
    /* empty  */ { insSkip, insFull, insMove, insMove,  insMove  },
    /* full   */ { insSkip, insSkip, insSkip, insSkip,  insSkip  },
    /* vector */ { insSkip, insFull, insVec,  insRIter, insRIter },
    /* bitmap */ { insSkip, insFull, insIter, insBit,   insError },
    /* meta   */ { insSkip, insFull, insIter, insError, insMeta  },
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
    logMsgFatal() << "erase: incompatible node types, " << left.type
        << ", " << right.type;
}

//===========================================================================
static void eraSkip(Node & left, const Node & right)
{}

//===========================================================================
static void eraEmpty(Node & left, const Node & right) {
    impl(left)->destroy(left);
    left.type = Node::kEmpty;
    s_emptyImpl.init(left, false);
}

//===========================================================================
static void eraChange(Node & left, const Node & right) {
    auto ri = UnsignedSet::iterator::makeFirst(&right);
    assert(ri);

    // convert from full to either bitmap or meta
    impl(left)->erase(left, *ri);

    erase(left, right);
}

//===========================================================================
static void eraFind(Node & left, const Node & right) {
    // Go through values of left vector and skip (aka remove) the ones that
    // are found in right node (values to be erased).
    assert(left.type == Node::kVector);
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
            for (;;) {
                *out++ = *li++;
                if (li == le)
                    break;
            }
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
static void eraIter(Node & left, const Node & right) {
    assert(right.type == Node::kVector);
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
    if (!left.numValues)
        eraEmpty(left, right);
}

//===========================================================================
static void eraBit(Node & left, const Node & right) {
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
static void eraMeta(Node & left, const Node & right) {
    assert(left.numValues == right.numValues);
    auto li = left.nodes;
    auto le = li + left.numValues;
    auto ri = right.nodes;
    for (; li != le; ++li, ++ri) {
        erase(*li, *ri);
        if (li->type != Node::kEmpty)
            goto NOT_EMPTY;
    }
    eraEmpty(left, right);
    return;

NOT_EMPTY:
    ++li, ++ri;
    for (; li != le; ++li, ++ri)
        erase(*li, *ri);
}

//===========================================================================
static void erase(Node & left, const Node & right) {
    using EraseFn = void(Node & left, const Node & right);
    static EraseFn * const functs[][Node::kNodeTypes] = {
    // LEFT                         RIGHT
    //             empty    full      vector     bitmap     meta
    /* empty  */ { eraSkip, eraSkip,  eraSkip,   eraSkip,   eraSkip   },
    /* full   */ { eraSkip, eraEmpty, eraIter,   eraChange, eraChange },
    /* vector */ { eraSkip, eraEmpty, eraVec,    eraFind,   eraFind   },
    /* bitmap */ { eraSkip, eraEmpty, eraIter,   eraBit,    eraError  },
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
    logMsgFatal() << "intersect: incompatible node types, " << left.type
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
    assert(left.type == Node::kVector);
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
    if (!left.numValues)
        isecEmpty(left, right);
}

//===========================================================================
static void isecRFind(Node & left, const Node & right) {
    Node tmp;
    isecCopy(tmp, right);
    swap(left, tmp);
    isecFind(left, move(tmp));
    impl(tmp)->destroy(tmp);
}

//===========================================================================
static void isecVec(Node & left, const Node & right) {
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
static void isecBit(Node & left, const Node & right) {
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
static void isecMeta(Node & left, const Node & right) {
    auto li = left.nodes;
    auto le = li + left.numValues;
    auto ri = right.nodes;
    for (; li != le; ++li, ++ri) {
        intersect(*li, *ri);
        if (li->type != Node::kEmpty)
            goto NOT_EMPTY;
    }
    isecEmpty(left, right);
    return;

NOT_EMPTY:
    ++li, ++ri;
    for (; li != le; ++li, ++ri)
        intersect(*li, *ri);
}

//===========================================================================
static void intersect(Node & left, const Node & right) {
    using IsectFn = void(Node & left, const Node & right);
    static IsectFn * const functs[][Node::kNodeTypes] = {
    // LEFT                         RIGHT
    //             empty      full      vector     bitmap     meta
    /* empty  */ { isecSkip,  isecSkip, isecSkip,  isecSkip,  isecSkip },
    /* full   */ { isecEmpty, isecSkip, isecCopy,  isecCopy,  isecCopy  },
    /* vector */ { isecEmpty, isecSkip, isecVec,   isecFind,  isecFind  },
    /* bitmap */ { isecEmpty, isecSkip, isecRFind, isecBit,   isecError },
    /* meta   */ { isecEmpty, isecSkip, isecRFind, isecError, isecMeta  },
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
static void isecBit(Node & left, Node && right) {
    isecBit(left, right);
}

//===========================================================================
static void isecMeta(Node & left, Node && right) {
    auto li = left.nodes;
    auto le = li + left.numValues;
    auto ri = right.nodes;
    for (; li != le; ++li, ++ri) {
        intersect(*li, move(*ri));
        if (li->type != Node::kEmpty)
            goto NOT_EMPTY;
    }
    isecEmpty(left, move(right));
    return;

NOT_EMPTY:
    ++li, ++ri;
    for (; li != le; ++li, ++ri)
        intersect(*li, move(*ri));
}

//===========================================================================
static void intersect(Node & left, Node && right) {
    using IsectFn = void(Node & left, Node && right);
    static IsectFn * const functs[][Node::kNodeTypes] = {
    // LEFT                         RIGHT
    //             empty      full      vector     bitmap     meta
    /* empty  */ { isecSkip,  isecSkip, isecEmpty, isecEmpty, isecEmpty },
    /* full   */ { isecEmpty, isecSkip, isecMove,  isecMove,  isecMove  },
    /* vector */ { isecEmpty, isecSkip, isecVec,   isecFind,  isecFind  },
    /* bitmap */ { isecEmpty, isecSkip, isecRFind, isecBit,   isecError },
    /* meta   */ { isecEmpty, isecSkip, isecRFind, isecError, isecMeta  },
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
UnsignedSet::UnsignedSet(const UnsignedSet & from) {
    insert(from);
}

//===========================================================================
UnsignedSet::UnsignedSet(string_view from) {
    insert(from);
}

//===========================================================================
UnsignedSet::UnsignedSet(unsigned low, unsigned high) {
    insert(low, high);
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
UnsignedSet & UnsignedSet::operator=(const UnsignedSet & from) {
    clear();
    insert(from);
    return *this;
}

//===========================================================================
UnsignedSet::iterator UnsignedSet::begin() const {
    return iterator::makeFirst(&m_node);
}

//===========================================================================
UnsignedSet::iterator UnsignedSet::end() const {
    return iterator::makeEnd(&m_node);
}

//===========================================================================
UnsignedSet::reverse_iterator UnsignedSet::rbegin() const {
    return reverse_iterator(end());
}

//===========================================================================
UnsignedSet::reverse_iterator UnsignedSet::rend() const {
    return reverse_iterator(begin());
}

//===========================================================================
UnsignedSet::RangeRange UnsignedSet::ranges() const {
    return RangeRange(begin());
}

//===========================================================================
bool UnsignedSet::empty() const {
    return m_node.type == Node::kEmpty;
}

//===========================================================================
size_t UnsignedSet::size() const {
    return impl(m_node)->size(m_node);
}

//===========================================================================
size_t UnsignedSet::max_size() const {
    return numeric_limits<unsigned>::max();
}

//===========================================================================
void UnsignedSet::clear() {
    impl(m_node)->destroy(m_node);
    m_node = {};
}

//===========================================================================
void UnsignedSet::fill() {
    clear();
    m_node.type = Node::kFull;
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
void UnsignedSet::assign(const UnsignedSet & from) {
    clear();
    insert(from);
}

//===========================================================================
void UnsignedSet::assign(string_view src) {
    clear();
    insert(src);
}

//===========================================================================
void UnsignedSet::assign(unsigned low, unsigned high) {
    clear();
    insert(low, high);
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
bool UnsignedSet::erase(unsigned value) {
    return impl(m_node)->erase(m_node, value);
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
void UnsignedSet::erase(unsigned low, unsigned high) {
    // TODO: make this efficient
    for (auto i = low; i <= high; ++i)
        erase(i);
}

//===========================================================================
unsigned UnsignedSet::pop_front() {
    auto val = front();
    erase(val);
    return val;
}

//===========================================================================
unsigned UnsignedSet::pop_back() {
    auto val = back();
    erase(val);
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
strong_ordering UnsignedSet::operator<=>(const UnsignedSet & other) const {
    return compare(other);
}

//===========================================================================
strong_ordering UnsignedSet::compare(const UnsignedSet & right) const {
    if (auto cmp = ::compare(m_node, right.m_node)) {
        return cmp < 0 ? strong_ordering::less : strong_ordering::greater;
    }
    return strong_ordering::equal;
}

//===========================================================================
bool UnsignedSet::contains(const UnsignedSet & other) const {
    return ::contains(m_node, other.m_node);
}

//===========================================================================
bool UnsignedSet::intersects(const UnsignedSet & other) const {
    return ::intersects(m_node, other.m_node);
}

//===========================================================================
bool UnsignedSet::operator==(const UnsignedSet & right) const {
    return compare(right) == 0;
}

//===========================================================================
unsigned UnsignedSet::front() const {
    auto i = begin();
    return *i;
}

//===========================================================================
unsigned UnsignedSet::back() const {
    auto i = rbegin();
    return *i;
}

//===========================================================================
size_t UnsignedSet::count(unsigned val) const {
    return contains(val);
}

//===========================================================================
size_t UnsignedSet::count(unsigned low, unsigned high) const {
    assert(low <= high);
    assert(!"Not implemented");
    auto lower = lowerBound(low);
    if (!lower || *lower > high)
        return 0;
    if (*lower == high)
        return 1;

    return 0;
}

//===========================================================================
bool UnsignedSet::contains(unsigned val) const {
    return (bool) find(val);
}

//===========================================================================
auto UnsignedSet::find(unsigned val) const -> iterator {
    auto first = lowerBound(val);
    return first && *first == val ? first : end();
}

//===========================================================================
auto UnsignedSet::findLessEqual(unsigned val) const -> iterator {
    return iterator::makeLast(&m_node, val);
}

//===========================================================================
auto UnsignedSet::lowerBound(unsigned val) const -> iterator {
    return iterator::makeFirst(&m_node, val);
}

//===========================================================================
auto UnsignedSet::upperBound(unsigned val) const -> iterator {
    val += 1;
    return val ? lowerBound(val) : end();
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
auto UnsignedSet::firstContiguous(iterator where) const -> iterator {
    return where.firstContiguous();
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
*   UnsignedSet::Iter
*
***/

//===========================================================================
// static
UnsignedSet::Iter UnsignedSet::Iter::makeEnd(const Node * node) {
    while (node->depth) {
        auto pos = nodePos(absBase(*node), node->depth - 1);
        node += numNodes(node->depth) - pos;
        node = node->nodes;
    }
    return Iter(node);
}

//===========================================================================
// static
UnsignedSet::Iter UnsignedSet::Iter::makeFirst(const Node * node) {
    return makeFirst(node, absBase(*node));
}

//===========================================================================
// static
UnsignedSet::Iter UnsignedSet::Iter::makeFirst(
    const Node * node,
    value_type value
) {
    if (impl(*node)->findFirst(&node, &value, *node, value))
        return {node, value};
    return {node};
}

//===========================================================================
// static
UnsignedSet::Iter UnsignedSet::Iter::makeLast(const Node * node) {
    return makeLast(node, absFinal(*node));
}

//===========================================================================
// static
UnsignedSet::Iter UnsignedSet::Iter::makeLast(
    const Node * node,
    value_type value
) {
    if (impl(*node)->findLast(&node, &value, *node, value))
        return {node, value};
    return {node};
}

//===========================================================================
UnsignedSet::Iter::Iter(const Node * node)
    : m_node(node)
{
    assert(node->depth == 0 && node->base == 0);
}

//===========================================================================
UnsignedSet::Iter::Iter(const Node * node, value_type value)
    : m_node(node)
    , m_value(value)
    , m_endmark(false)
{}

//===========================================================================
bool UnsignedSet::Iter::operator== (const Iter & right) const {
    return m_value == right.m_value
        && m_endmark == right.m_endmark
        && (m_node == right.m_node || !m_node || !right.m_node);
}

//===========================================================================
UnsignedSet::Iter & UnsignedSet::Iter::operator++() {
    if (!*this) {
        if (m_node) {
            auto from = absBase(*m_node);
            if (impl(*m_node)->findFirst(&m_node, &m_value, *m_node, from))
                m_endmark = false;
        }
        return *this;
    }
    if (m_value < absFinal(*m_node)) {
        m_value += 1;
        if (impl(*m_node)->findFirst(&m_node, &m_value, *m_node, m_value))
            return *this;
    }

CHECK_DEPTH:
    if (m_node->depth == 0) {
        *this = end();
        return *this;
    }

CHECK_END_OF_DEPTH:
    if (absFinal(*m_node) == absFinal(m_value, m_node->depth - 1)) {
        m_node += 1;
        assert(m_node->type == Node::kMetaParent);
        m_node = m_node->nodes;
        m_value = absBase(*m_node);
        goto CHECK_DEPTH;
    }

    m_node += 1;
    m_value = absBase(*m_node);
    if (impl(*m_node)->findFirst(&m_node, &m_value, *m_node, m_value))
        return *this;
    goto CHECK_END_OF_DEPTH;
}

//===========================================================================
UnsignedSet::Iter & UnsignedSet::Iter::operator--() {
    if (!*this) {
        if (m_node) {
            auto from = absFinal(*m_node);
            if (impl(*m_node)->findLast(&m_node, &m_value, *m_node, from))
                m_endmark = false;
        }
        return *this;
    }
    if (m_value > absBase(*m_node)) {
        m_value -= 1;
        if (impl(*m_node)->findLast(&m_node, &m_value, *m_node, m_value))
            return *this;
    }

CHECK_DEPTH:
    if (m_node->depth == 0) {
        *this = end();
        return *this;
    }

CHECK_END_OF_DEPTH:
    if (absBase(*m_node) == absBase(m_value, m_node->depth - 1)) {
        m_node += numNodes(m_node->depth);
        assert(m_node->type == Node::kMetaParent);
        m_node = m_node->nodes;
        m_value = absFinal(*m_node);
        goto CHECK_DEPTH;
    }

    m_node -= 1;
    m_value = absFinal(*m_node);
    if (impl(*m_node)->findLast(&m_node, &m_value, *m_node, m_value))
        return *this;
    goto CHECK_END_OF_DEPTH;
}

//===========================================================================
UnsignedSet::Iter UnsignedSet::Iter::end() const {
    return iterator::makeEnd(m_node);
}

//===========================================================================
UnsignedSet::Iter UnsignedSet::Iter::firstContiguous() const {
    auto node = m_node;
    auto value = m_value;
    if (impl(*node)->firstContiguous(&node, &value, *node, value))
        return {node, value};

    // Use a temporary to scout out the next node (which may require traversing
    // the tree to get there) since if it starts with a discontinuity we'll end
    // the search without advancing.
    auto ptr = node;

CHECK_DEPTH:
    if (ptr->depth == 0)
        return {node, value};

CHECK_END_OF_DEPTH:
    if (absBase(*ptr) == absBase(value, ptr->depth - 1)) {
        ptr += numNodes(ptr->depth);
        assert(ptr->type == Node::kMetaParent);
        ptr = ptr->nodes;
        goto CHECK_DEPTH;
    }

    ptr -= 1;
    if (impl(*ptr)->firstContiguous(&node, &value, *ptr, absFinal(*ptr)))
        return {node, value};
    goto CHECK_END_OF_DEPTH;
}

//===========================================================================
UnsignedSet::Iter UnsignedSet::Iter::lastContiguous() const {
    auto node = m_node;
    auto value = m_value;
    if (impl(*node)->lastContiguous(&node, &value, *node, value))
        return {node, value};

    // Use a temporary to scout out the next node (which may require traversing
    // the tree to find) since if it starts with a discontinuity we end the
    // search at the current node.
    auto ptr = node;

CHECK_DEPTH:
    if (ptr->depth == 0)
        return {node, value};

CHECK_END_OF_DEPTH:
    if (absFinal(*ptr) == absFinal(value, ptr->depth - 1)) {
        ptr += 1;
        assert(ptr->type == Node::kMetaParent);
        ptr = ptr->nodes;
        goto CHECK_DEPTH;
    }

    ptr += 1;
    if (impl(*ptr)->lastContiguous(&node, &value, *ptr, absBase(*ptr)))
        return {node, value};
    goto CHECK_END_OF_DEPTH;
}


/****************************************************************************
*
*   UnsignedSet::RangeIterator
*
***/

//===========================================================================
UnsignedSet::RangeIter::RangeIter(iterator where)
    : m_low(where)
    , m_high(where)
{
    if (m_low) {
        m_high = m_low.lastContiguous();
        m_value = { *m_low, *m_high };
    }
}

//===========================================================================
bool UnsignedSet::RangeIter::operator== (
    const RangeIter & other
) const {
    return m_low == other.m_low;
}

//===========================================================================
UnsignedSet::RangeIter & UnsignedSet::RangeIter::operator++() {
    m_low = m_high;
    if (++m_low) {
        m_high = m_low.lastContiguous();
        m_value = { *m_low, *m_high };
    } else {
        // Set high to also be at the end.
        m_high = m_low;
    }
    return *this;
}

//===========================================================================
UnsignedSet::RangeIter & UnsignedSet::RangeIter::operator--() {
    m_high = m_low;
    if (--m_high) {
        m_low = m_high.firstContiguous();
        m_value = { *m_low, *m_high };
    } else {
        // Set low to also be at the end.
        m_low = m_high;
    }
    return *this;
}


/****************************************************************************
*
*   Free functions
*
***/

//===========================================================================
namespace Dim {
ostream & operator<<(ostream & os, const UnsignedSet & right);
}
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
