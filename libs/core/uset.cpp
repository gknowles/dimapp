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

static constexpr unsigned maxDepth() {
    return kMaxDepth;
}

static constexpr unsigned valueMask(unsigned depth) {
    assert(depth <= kMaxDepth);
    unsigned bits = kBitWidth - (kLeafBits + kStepBits * (kMaxDepth - depth));
    return numeric_limits<unsigned>::max() >> bits;
}
static constexpr unsigned relBase(unsigned value, unsigned depth) {
    return (value & ~valueMask(depth)) >> 8;
}
static constexpr unsigned relValue(unsigned value, unsigned depth) {
    return value & valueMask(depth);
}

static constexpr uint16_t numNodes(unsigned depth) {
    uint16_t ret = 0;
    if (depth) {
        ret = (uint16_t) 1
            << hammingWeight(valueMask(depth) ^ valueMask(depth - 1));
    }
    return ret;
}
static constexpr unsigned nodePos(unsigned value, unsigned depth) {
    return relValue(value, depth) / (valueMask(depth + 1) + 1);
}

static constexpr unsigned absBase(unsigned value, unsigned depth) {
    return value & ~valueMask(depth);
}
static constexpr unsigned absFinal(unsigned value, unsigned depth) {
    return absBase(value, depth) + valueMask(depth);
}

static constexpr unsigned absBase(const Node & node) {
    return node.base << 8;
}
static constexpr unsigned absFinal(const Node & node) {
    return absBase(node) + valueMask(node.depth);
}
static constexpr unsigned absSize(const Node & node) {
    return valueMask(node.depth) + 1;
}

enum UnsignedSet::Node::Type : int {
    kEmpty,         // contains no values
    kFull,          // contains all values in node's domain
    kSmVector,      // small vector of values embedded in node struct
    kVector,        // vector of values
    kBitmap,        // bitmap covering all of node's possible values
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
    virtual bool insert(
        Node & node,
        const unsigned * first,
        const unsigned * last
    ) = 0;
    virtual bool insert(Node & node, unsigned start, size_t count) = 0;
    virtual bool erase(Node & node, unsigned start, size_t count) = 0;

    virtual size_t count(const Node & node) const = 0;
    virtual size_t count(
        const Node & node,
        unsigned start,
        size_t count
    ) const = 0;

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
    // [start of node, base] is full - output node & value, return false
    // otherwise - output node & value, return true
    virtual bool firstContiguous(
        const Node ** onode,
        unsigned * ovalue,
        const Node & node,
        unsigned base
    ) const = 0;
    // [base] is empty - return true
    // [base, end of node] is full - output node & value, return false
    // otherwise - output node & value, return true
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
    bool insert(
        Node & node,
        const unsigned * first,
        const unsigned * last
    ) override;
    bool insert(Node & node, unsigned start, size_t count) override;
    bool erase(Node & node, unsigned start, size_t count) override;

    size_t count(const Node & node) const override;
    size_t count(
        const Node & node,
        unsigned start,
        size_t count
    ) const override;
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
    bool insert(
        Node & node,
        const unsigned * first,
        const unsigned * last
    ) override;
    bool insert(Node & node, unsigned start, size_t count) override;
    bool erase(Node & node, unsigned start, size_t count) override;

    size_t count(const Node & node) const override;
    size_t count(
        const Node & node,
        unsigned start,
        size_t count
    ) const override;
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

struct SmVectorImpl final : IImplBase {
    static constexpr size_t maxValues() {
        return sizeof Node::localValues / sizeof *Node::localValues;
    }

    void init(Node& node, bool full) override;
    void init(Node& node, const Node& from) override;
    void destroy(Node& node) override;
    bool insert(
        Node& node,
        const unsigned* first,
        const unsigned* last
    ) override;
    bool insert(Node& node, unsigned start, size_t count) override;
    bool erase(Node& node, unsigned start, size_t count) override;

    size_t count(const Node & node) const override;
    size_t count(
        const Node & node,
        unsigned start,
        size_t count
    ) const override;
    bool findFirst(
        const Node** onode,
        unsigned* ovalue,
        const Node& node,
        unsigned base
    ) const override;
    bool findLast(
        const Node** onode,
        unsigned* ovalue,
        const Node& node,
        unsigned base
    ) const override;
    bool firstContiguous(
        const Node** onode,
        unsigned* ovalue,
        const Node& node,
        unsigned base
    ) const override;
    bool lastContiguous(
        const Node** onode,
        unsigned* ovalue,
        const Node& node,
        unsigned base
    ) const override;

private:
    void convert(Node& node);
};

struct VectorImpl final : IImplBase {
    static constexpr size_t maxValues() {
        return kDataSize / sizeof *Node::values;
    }

    void init(Node & node, bool full) override;
    void init(Node & node, const Node & from) override;
    void destroy(Node & node) override;
    bool insert(
        Node & node,
        const unsigned * first,
        const unsigned * last
    ) override;
    bool insert(Node & node, unsigned start, size_t count) override;
    bool erase(Node & node, unsigned start, size_t count) override;

    size_t count(const Node & node) const override;
    size_t count(
        const Node & node,
        unsigned start,
        size_t count
    ) const override;
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
    void convert(Node & node);
};

struct BitmapImpl final : IImplBase {
    static constexpr size_t numInt64s() {
        return kDataSize / sizeof uint64_t;
    }
    static constexpr size_t numBits() { return 64 * numInt64s(); }

    void init(Node & node, bool full) override;
    void init(Node & node, const Node & from) override;
    void destroy(Node & node) override;
    bool insert(
        Node & node,
        const unsigned * first,
        const unsigned * last
    ) override;
    bool insert(Node & node, unsigned start, size_t count) override;
    bool erase(Node & node, unsigned start, size_t count) override;

    size_t count(const Node & node) const override;
    size_t count(
        const Node & node,
        unsigned start,
        size_t count
    ) const override;
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
    bool insert(
        Node & node,
        const unsigned * first,
        const unsigned * last
    ) override;
    bool insert(Node & node, unsigned start, size_t count) override;
    bool erase(Node & node, unsigned start, size_t count) override;

    size_t count(const Node & node) const override;
    size_t count(
        const Node & node,
        unsigned start,
        size_t count
    ) const override;
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
    bool convertIf(Node & node, Node::Type type);
};

} // namespace


/****************************************************************************
*
*   Helpers
*
***/

static EmptyImpl s_emptyImpl;
static FullImpl s_fullImpl;
static SmVectorImpl s_smallVectorImpl;
static VectorImpl s_vectorImpl;
static BitmapImpl s_bitmapImpl;
static MetaImpl s_metaImpl;

static IImplBase * s_impls[] = {
    &s_emptyImpl,
    &s_fullImpl,
    &s_smallVectorImpl,
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
*   node.numValues - unused, always 0
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
bool EmptyImpl::insert(
    Node & node,
    const unsigned * first,
    const unsigned * last
) {
    if (first == last)
        return false;

    destroy(node);
    node.type = Node::kSmVector;
    s_smallVectorImpl.init(node, false);
    s_smallVectorImpl.insert(node, first, last);
    return true;
}

//===========================================================================
bool EmptyImpl::insert(Node & node, unsigned start, size_t count) {
    assert(node.type == Node::kEmpty);
    assert(count);
    assert(relBase(start, node.depth) == node.base);
    assert(count - 1 <= absFinal(node) - start);
    destroy(node);
    node.type = Node::kSmVector;
    s_smallVectorImpl.init(node, false);
    s_smallVectorImpl.insert(node, start, count);
    return true;
}

//===========================================================================
bool EmptyImpl::erase(Node & node, unsigned start, size_t count) {
    assert(count);
    assert(relBase(start, node.depth) == node.base);
    assert(count - 1 <= absFinal(node) - start);
    return false;
}

//===========================================================================
size_t EmptyImpl::count(const Node & node) const {
    return 0;
}

//===========================================================================
size_t EmptyImpl::count(
    const Node & node,
    unsigned start,
    size_t count
) const {
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
*   node.numValues - unused, always 0
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
bool FullImpl::insert(
    Node & node,
    const unsigned * first,
    const unsigned * last
) {
    return false;
}

//===========================================================================
bool FullImpl::insert(Node & node, unsigned start, size_t count) {
    assert(count);
    assert(relBase(start, node.depth) == node.base);
    assert(count - 1 <= absFinal(node) - start);
    return false;
}

//===========================================================================
bool FullImpl::erase(Node & node, unsigned start, size_t count) {
    assert(count);
    assert(relBase(start, node.depth) == node.base);
    assert(count - 1 <= absFinal(node) - start);
    destroy(node);
    if (node.depth == maxDepth()) {
        node.type = Node::kBitmap;
        s_bitmapImpl.init(node, true);
        return s_bitmapImpl.erase(node, start, count);
    } else {
        node.type = Node::kMeta;
        s_metaImpl.init(node, true);
        return s_metaImpl.erase(node, start, count);
    }
}

//===========================================================================
size_t FullImpl::count(const Node & node) const {
    return absSize(node);
}

//===========================================================================
size_t FullImpl::count(
    const Node & node,
    unsigned start,
    size_t count
) const {
    return count;
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
*   SmVectorImpl
*
*   node.numValues - size (not capacity!) of node.localValues array
*
***/

//===========================================================================
void SmVectorImpl::init(Node & node, bool full) {
    assert(node.type == Node::kSmVector);
    assert(!full);
    node.numBytes = 0;
    node.numValues = 0;
}

//===========================================================================
void SmVectorImpl::init(Node & node, const Node & from) {
    assert(node.type == Node::kSmVector);
    assert(from.type == Node::kSmVector);
    node = from;
}

//===========================================================================
void SmVectorImpl::destroy(Node & node)
{}

//===========================================================================
bool SmVectorImpl::insert(
    Node & node,
    const unsigned * first,
    const unsigned * last
) {
    bool changed = false;
    while (first != last) {
        if (node.type != Node::kSmVector)
            return impl(node)->insert(node, first, last) || changed;
        changed = insert(node, *first++, 1) || changed;
    }
    return changed;
}

//===========================================================================
bool SmVectorImpl::insert(Node & node, unsigned start, size_t count) {
    assert(count);
    assert(relBase(start, node.depth) == node.base);
    assert(count - 1 <= absFinal(node) - start);

    auto last = node.localValues + node.numValues;
    auto ptr = lower_bound(node.localValues, last, start);
    auto eptr = (ptr < last && *ptr < start + count)
        ? lower_bound(ptr, last, start + count)
        : ptr;
    size_t nBelow = ptr - node.localValues;
    size_t nOld = eptr - ptr;
    size_t nAbove = last - eptr;
    if (count == nOld)
        return false;
    auto num = nBelow + count + nAbove;
    if (num > maxValues()) {
        convert(node);
        return impl(node)->insert(node, start, count);
    }

    node.numValues = (uint16_t) num;
    if (nAbove)
        memmove(ptr + count, eptr, nAbove * sizeof *ptr);
    for (;;) {
        *ptr++ = start;
        if (!--count)
            return true;
        start += 1;
    }
}

//===========================================================================
bool SmVectorImpl::erase(Node & node, unsigned start, size_t count) {
    assert(count);
    assert(relBase(start, node.depth) == node.base);
    assert(count - 1 <= absFinal(node) - start);
    auto last = node.localValues + node.numValues;
    auto ptr = lower_bound(node.localValues, last, start);
    if (ptr == last || *ptr >= start + count)
        return false;
    auto eptr = lower_bound(ptr, last, start + count);
    if (ptr == eptr)
        return false;

    node.numValues -= (uint16_t) (eptr - ptr);
    if (node.numValues) {
        // Still has values, shift remaining ones down
        memmove(ptr, eptr, sizeof *ptr * (last - eptr));
    } else {
        // No more values, convert to empty node.
        destroy(node);
        node.type = Node::kEmpty;
        s_emptyImpl.init(node, false);
    }
    return true;
}

//===========================================================================
size_t SmVectorImpl::count(const Node & node) const {
    return node.numValues;
}

//===========================================================================
size_t SmVectorImpl::count(
    const Node & node,
    unsigned start,
    size_t count
) const {
    assert(count);
    assert(relBase(start, node.depth) == node.base);
    assert(count - 1 <= absFinal(node) - start);
    auto last = node.localValues + node.numValues;
    auto ptr = lower_bound(node.localValues, last, start);
    if (ptr == last || *ptr >= start + count)
        return 0;
    auto eptr = lower_bound(ptr, last, start + count);
    return eptr - ptr;
}

//===========================================================================
bool SmVectorImpl::findFirst(
    const Node ** onode,
    unsigned * ovalue,
    const Node & node,
    unsigned base
) const {
    assert(relBase(base, node.depth) == node.base);
    auto last = node.localValues + node.numValues;
    auto ptr = lower_bound(node.localValues, last, base);
    if (ptr != last) {
        *onode = &node;
        *ovalue = *ptr;
        return true;
    }
    return false;
}

//===========================================================================
bool SmVectorImpl::findLast(
    const Node ** onode,
    unsigned * ovalue,
    const Node & node,
    unsigned base
) const {
    assert(relBase(base, node.depth) == node.base);
    if (base < *node.localValues) {
        return false;
    } else if (base == *node.localValues) {
        *ovalue = base;
    } else {
        // base > *node.localValues
        auto last = node.localValues + node.numValues;
        auto ptr = lower_bound(node.localValues + 1, last, base);
        *ovalue = ptr[-1];
    }
    *onode = &node;
    return true;
}

//===========================================================================
bool SmVectorImpl::firstContiguous(
    const Node ** onode,
    unsigned * ovalue,
    const Node & node,
    unsigned base
) const {
    assert(relBase(base, node.depth) == node.base);
    auto last = node.localValues + node.numValues;
    auto ptr = lower_bound(node.localValues, last, base);
    auto val = base;
    if (ptr == last || *ptr != val) {
        // Base not found, return that search is done, but leave output node
        // and value unchanged.
        return true;
    }
    *onode = &node;
    while (ptr != node.localValues) {
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
bool SmVectorImpl::lastContiguous(
    const Node ** onode,
    unsigned * ovalue,
    const Node & node,
    unsigned base
) const {
    assert(relBase(base, node.depth) == node.base);
    auto last = node.localValues + node.numValues;
    auto ptr = lower_bound(node.localValues, last, base);
    auto val = base;
    if (ptr == last || *ptr != val) {
        // Base not found, return that search is done, but leave output node
        // and value unchanged.
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
void SmVectorImpl::convert(Node & node) {
    Node tmp{node};
    auto ptr = tmp.localValues;
    auto last = ptr + tmp.numValues;
    node.type = Node::kVector;
    s_vectorImpl.init(node, false);
    s_vectorImpl.insert(node, ptr, last);
    destroy(tmp);
}


/****************************************************************************
*
*   VectorImpl
*
*   node.numValues - size (not capacity!) of node.values array
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
    memcpy(node.values, from.values, node.numValues * sizeof *node.values);
}

//===========================================================================
void VectorImpl::destroy(Node & node) {
    assert(node.values);
    free(node.values);
}

//===========================================================================
bool VectorImpl::insert(
    Node & node,
    const unsigned * first,
    const unsigned * last
) {
    bool changed = false;
    while (first != last) {
        if (node.type != Node::kVector)
            return impl(node)->insert(node, first, last) || changed;
        changed = insert(node, *first++, 1) || changed;
    }
    return changed;
}

//===========================================================================
bool VectorImpl::insert(Node & node, unsigned start, size_t count) {
    assert(count);
    assert(relBase(start, node.depth) == node.base);
    assert(count - 1 <= absFinal(node) - start);
    auto last = node.values + node.numValues;
    auto ptr = lower_bound(node.values, last, start);
    auto eptr = (ptr < last && *ptr < start + count)
        ? lower_bound(ptr, last, start + count)
        : ptr;
    size_t nBelow = ptr - node.values;
    size_t nOld = eptr - ptr;
    size_t nAbove = last - eptr;
    if (count == nOld)
        return false;
    auto num = nBelow + count + nAbove;
    if (num > maxValues()) {
        convert(node);
        return impl(node)->insert(node, start, count);
    }

    node.numValues = (uint16_t) num;
    if (nAbove)
        memmove(ptr + count, eptr, nAbove * sizeof *ptr);
    for (;;) {
        *ptr++ = start;
        if (!--count)
            return true;
        start += 1;
    }
}

//===========================================================================
bool VectorImpl::erase(Node & node, unsigned start, size_t count) {
    assert(count);
    assert(relBase(start, node.depth) == node.base);
    assert(count - 1 <= absFinal(node) - start);
    auto last = node.values + node.numValues;
    auto ptr = lower_bound(node.values, last, start);
    if (ptr == last || *ptr >= start + count)
        return false;
    auto eptr = lower_bound(ptr, last, start + count);
    if (ptr == eptr)
        return false;

    node.numValues -= (uint16_t) (eptr - ptr);
    if (node.numValues) {
        // Still has values, shift remaining ones down
        memmove(ptr, eptr, sizeof *ptr * (last - eptr));
    } else {
        // No more values, convert to empty node.
        destroy(node);
        node.type = Node::kEmpty;
        s_emptyImpl.init(node, false);
    }
    return true;
}

//===========================================================================
size_t VectorImpl::count(const Node & node) const {
    return node.numValues;
}

//===========================================================================
size_t VectorImpl::count(
    const Node & node,
    unsigned start,
    size_t count
) const {
    assert(count);
    assert(relBase(start, node.depth) == node.base);
    assert(count - 1 <= absFinal(node) - start);
    auto last = node.values + node.numValues;
    auto ptr = lower_bound(node.values, last, start);
    if (ptr == last || *ptr >= start + count)
        return 0;
    auto eptr = lower_bound(ptr, last, start + count);
    return eptr - ptr;
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
        // Base not found, return that search is done, but leave output node
        // and value unchanged.
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
        // Base not found, return that search is done, but leave output node
        // and value unchanged.
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
void VectorImpl::convert(Node & node) {
    Node tmp {node};
    auto ptr = tmp.values;
    auto last = ptr + tmp.numValues;
    if (tmp.depth == maxDepth()) {
        node.type = Node::kBitmap;
        s_bitmapImpl.init(node, false);
        s_bitmapImpl.insert(node, ptr, last);
    } else {
        node.type = Node::kMeta;
        s_metaImpl.init(node, false);
        s_metaImpl.insert(node, ptr, last);
    }
    destroy(tmp);
}


/****************************************************************************
*
*   BitmapImpl
*
*   node.numValues - number of non-zero words in the bit view
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
bool BitmapImpl::insert(
    Node & node,
    const unsigned * first,
    const unsigned * last
) {
    bool changed = false;
    for (; first != last; ++first)
        changed = insert(node, *first, 1) || changed;
    return changed;
}

//===========================================================================
bool BitmapImpl::insert(Node & node, unsigned start, size_t count) {
    assert(node.type == Node::kBitmap);
    assert(count);
    assert(relBase(start, node.depth) == node.base);
    assert(count - 1 <= absFinal(node) - start);
    auto base = (uint64_t *) node.values;
    BitView bits(base, numInt64s());

    auto low = relValue(start, node.depth);
    auto high = low + count - 1;
    auto lower = bits.data(low);
    auto upper = bits.data(high);
    if (lower == upper) {
        auto tmp = *lower;
        bits.set(low, count);
        if (tmp == *lower)
            return false;
        if (!tmp)
            node.numValues += 1;
        return true;
    }

    bits.remove_suffix(numInt64s() - (upper - base + 1));
    auto zero = (unsigned) bits.findZero(low);
    if (zero > high)
        return false;

    lower = bits.data(zero);
    for (auto ptr = lower; ptr <= upper; ++ptr) {
        if (!*ptr)
            node.numValues += 1;
    }
    bits.set(zero, high - zero + 1);

    if (node.numValues < numInt64s())
        return true;
    bits = BitView(base, numInt64s());
    if (!bits.all())
        return true;

    destroy(node);
    node.type = Node::kFull;
    s_fullImpl.init(node, true);
    return true;
}

//===========================================================================
bool BitmapImpl::erase(Node & node, unsigned start, size_t count) {
    assert(count);
    assert(relBase(start, node.depth) == node.base);
    assert(count - 1 <= absFinal(node) - start);
    auto base = (uint64_t *) node.values;
    BitView bits(base, numInt64s());
    auto low = relValue(start, node.depth);
    auto high = low + count - 1;
    auto lower = bits.data(low);
    auto upper = bits.data(high);
    if (lower == upper) {
        auto tmp = *lower;
        bits.reset(low, count);
        if (tmp == *lower)
            return false;
        if (!*lower) {
            node.numValues -= 1;
            goto CHECK_IF_EMPTY;
        }
        return true;
    }

    bits = BitView(lower, upper - lower + 1);
    low %= bits.kWordBits;
    high = low + count - 1;
    low = (unsigned) bits.find(low);
    if (low > high)
        return false;

    count = high - low + 1;
    lower = bits.data(low);
    bits = BitView(lower, upper - lower + 1);
    low %= bits.kWordBits;
    for (auto ptr = lower; ptr <= upper; ++ptr) {
        if (*ptr)
            node.numValues -= 1;
    }
    bits.reset(low, count);
    for (auto ptr = lower; ptr <= upper; ++ptr) {
        if (*ptr)
            node.numValues += 1;
    }

CHECK_IF_EMPTY:
    if (!node.numValues) {
        destroy(node);
        node.type = Node::kEmpty;
        s_emptyImpl.init(node, false);
    }

    return true;
}

//===========================================================================
size_t BitmapImpl::count(const Node & node) const {
    auto base = (uint64_t *) node.values;
    BitView bits{base, numInt64s()};
    return bits.count();
}

//===========================================================================
size_t BitmapImpl::count(
    const Node & node,
    unsigned start,
    size_t count
) const {
    assert(count);
    assert(relBase(start, node.depth) == node.base);
    assert(count - 1 <= absFinal(node) - start);
    auto base = (uint64_t *) node.values;
    BitView bits{base, numInt64s()};
    auto low = relValue(start, node.depth);
    return bits.count(low, count);
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
*   node.numValues - length of node.nodes array
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
bool MetaImpl::insert(
    Node & node,
    const unsigned * first,
    const unsigned * last
) {
    bool changed = false;
    while (first != last) {
        auto final = absFinal(*first, node.depth + 1);
        auto ptr = node.nodes + nodePos(node, *first);
        auto mid = first + 1;
        while (mid != last && *mid > *first && *mid <= final)
            mid += 1;
        changed = impl(*ptr)->insert(*ptr, first, mid) || changed;
        first = mid;
    }
    return changed;
}

//===========================================================================
bool MetaImpl::insert(Node & node, unsigned start, size_t count) {
    assert(count);
    assert(relBase(start, node.depth) == node.base);
    assert(count - 1 <= absFinal(node) - start);
    auto ptr = node.nodes + nodePos(node, start);
    auto step = valueMask(node.depth + 1) + 1;
    auto maybeFull = true;
    auto changed = false;
    for (auto&& [st, cnt] : AlignedIntervalGen(start, count, step)) {
        if (cnt == step) {
            if (ptr->type != Node::kFull) {
                changed = true;
                impl(*ptr)->destroy(*ptr);
                ptr->type = Node::kFull;
                impl(*ptr)->init(*ptr, true);
            }
        } else if (impl(*ptr)->insert(*ptr, (value_type) st, cnt)) {
            changed = true;
            if (ptr->type != Node::kFull)
                maybeFull = false;
        }
        ptr += 1;
    }
    if (changed && maybeFull)
        convertIf(node, Node::kFull);
    return changed;
}

//===========================================================================
bool MetaImpl::erase(Node & node, unsigned start, size_t count) {
    assert(count);
    assert(relBase(start, node.depth) == node.base);
    assert(count - 1 <= absFinal(node) - start);
    auto ptr = node.nodes + nodePos(node, start);
    auto step = valueMask(node.depth + 1) + 1;
    auto maybeEmpty = true;
    auto changed = false;
    for (auto&& [st, cnt] : AlignedIntervalGen(start, count, step)) {
        if (cnt == step) {
            if (ptr->type != Node::kEmpty) {
                changed = true;
                impl(*ptr)->destroy(*ptr);
                ptr->type = Node::kEmpty;
                impl(*ptr)->init(*ptr, false);
            }
        } else if (impl(*ptr)->erase(*ptr, (value_type) st, cnt)) {
            changed = true;
            if (ptr->type != Node::kEmpty)
                maybeEmpty = false;
        }
        ptr += 1;
    }
    if (changed && maybeEmpty)
        convertIf(node, Node::kEmpty);
    return changed;
}

//===========================================================================
size_t MetaImpl::count(const Node & node) const {
    size_t num = 0;
    auto * ptr = node.nodes;
    auto * last = ptr + node.numValues;
    for (; ptr != last; ++ptr)
        num += impl(*ptr)->count(*ptr);
    return num;
}

//===========================================================================
size_t MetaImpl::count(
    const Node & node,
    unsigned start,
    size_t count
) const {
    assert(count);
    assert(relBase(start, node.depth) == node.base);
    assert(count - 1 <= absFinal(node) - start);
    auto pos = nodePos(node, start);
    auto finalPos = nodePos(node, (value_type) (start + count - 1));
    auto lower = node.nodes + pos;
    auto upper = node.nodes + finalPos;
    if (pos == finalPos) {
        auto ptr = node.nodes + pos;
        return impl(*ptr)->count(*ptr, start, count);
    }

    size_t num = 0;
    num += impl(*lower)->count(
        *lower,
        start,
        absFinal(*lower) - start + 1
    );
    for (auto ptr = lower + 1; ptr < upper; ++ptr) {
        num += impl(*ptr)->count(*ptr);
    }
    num += impl(*upper)->count(
        *upper,
        absBase(*upper),
        start + count - 1
    );
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

//===========================================================================
bool MetaImpl::convertIf(Node & node, Node::Type type) {
    assert(type == Node::kEmpty || type == Node::kFull);
    for (unsigned i = 0; i < node.numValues; ++i) {
        if (node.nodes[i].type != type)
            return false;
    }
    // convert
    destroy(node);
    node.type = type;
    impl(node)->init(node, type == Node::kFull);
    return true;
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
*   Assuming meta nodes contain ranges of 100 values:
*       | 1 2 | <=> | 1 3 |         compares 2 <=> 3, which is -1
*       | 1 2 | <=> | 1   | ??? |   compares 2 <=> ???, which is 2
*       | 1 2 | <=> | 1   |         compares 2 <=> none, which is 1
*       | 1 2 | <=> | 1   | 101 |   compares 2 <=> 101, which is -1
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
static int cmpArray(
    const unsigned * li,
    unsigned lcount,
    const unsigned * ri,
    unsigned rcount
) {
    auto le = li + lcount;
    auto re = ri + rcount;
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
static int cmpVecIf(const Node & left, const Node & right) {
    auto minMax = absBase(right) + right.numValues - 1;
    if (minMax == right.values[right.numValues - 1]) {
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
static int cmpSVecIf(const Node & left, const Node & right) {
    auto minMax = absBase(right) + right.numValues - 1;
    if (minMax == right.localValues[right.numValues - 1]) {
        return 2;
    } else {
        return -1;
    }
}

//===========================================================================
static int cmpRSVecIf(const Node & left, const Node & right) {
    return -cmpSVecIf(right, left);
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
// vector <=> vector
static int cmpVec(const Node & left, const Node & right) {
    return cmpArray(
        left.values,
        left.numValues,
        right.values,
        right.numValues
    );
}

//===========================================================================
// small vector <=> small vector
static int cmpSVec(const Node & left, const Node & right) {
    return cmpArray(
        left.localValues,
        left.numValues,
        right.localValues,
        right.numValues
    );
}

//===========================================================================
// small vector <=> vector
static int cmpLSVec(const Node & left, const Node & right) {
    return cmpArray(
        left.localValues,
        left.numValues,
        right.values,
        right.numValues
    );
}

//===========================================================================
// vector <=> small vector
static int cmpRSVec(const Node & left, const Node & right) {
    return cmpArray(
        left.values,
        left.numValues,
        right.localValues,
        right.numValues
    );
}

//===========================================================================
static int cmpBit(uint64_t left, uint64_t right) {
    if (left == right)
        return 0;   // equal
    uint64_t mask = numeric_limits<uint64_t>::max();
    if (left < right) {
        if (left != (right & (mask << trailingZeroBits(left)))) {
            // left bitmap > right bitmap. Which means the right side has gaps,
            // paradoxically making it greater lexicographically. For example,
            // { 1, 2, 3 } < { 1, 3 }, since at the second position 2 < 3.
            // Where the bitmap representations compare reversed, 111 > 101.
            return 1;
        } else {
            // left and right only differ where left has trailing zeros, so if
            // left has any following non-zero nodes it would be larger
            // lexicographically due to having the larger gap.
            return -2;
        }
    } else {
        if (right != (left & (mask << trailingZeroBits(right)))) {
            return -1;
        } else {
            return 2;
        }
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
        if (int rc = cmpBit(ntoh64(li), ntoh64(ri))) {
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
    using Fn = int(const Node & left, const Node & right);
    static Fn * functs[][Node::kNodeTypes] = {
// LEFT                                 RIGHT
//         empty      full        sm vec     vector     bitmap     meta
/*empty*/{ cmpEqual,  cmpLessIf,  cmpLessIf, cmpLessIf, cmpLessIf, cmpLessIf },
/*full */{ cmpMoreIf, cmpEqual,   cmpSVecIf, cmpVecIf,  cmpBitIf,  cmpMetaIf },
/*svec */{ cmpMoreIf, cmpRSVecIf, cmpSVec,   cmpLSVec,  cmpIter,   cmpIter   },
/*vec  */{ cmpMoreIf, cmpRVecIf,  cmpRSVec,  cmpVec,    cmpIter,   cmpIter   },
/*bit  */{ cmpMoreIf, cmpRBitIf,  cmpIter,   cmpIter,   cmpBit,    cmpError  },
/*meta */{ cmpMoreIf, cmpRMetaIf, cmpIter,   cmpIter,   cmpError,  cmpMeta   },
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
static bool conRArray(
    const Node & left,
    const Node & right,
    const UnsignedSet::value_type * ri
) {
    auto re = ri + right.numValues;
    const Node* onode;
    unsigned ovalue;

    for (;;) {
        if (!impl(left)->findFirst(&onode, &ovalue, left, *ri))
            return false;
        if (*ri != ovalue || ++ri == re)
            return true;
    }
}

//===========================================================================
static bool conLArray(
    const Node & left,
    const UnsignedSet::value_type * li,
    const Node & right
) {
    auto le = li + left.numValues;
    const Node* onode;
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
static bool conArray(
    const UnsignedSet::value_type * li,
    unsigned lcount,
    const UnsignedSet::value_type * ri,
    unsigned rcount
) {
    if (lcount < rcount)
        return false;

    auto le = li + lcount;
    auto re = ri + rcount;

    for (;;) {
        li = lower_bound(li, le, *ri);
        if (li == le || *li != *ri)
            return false;
        if (++ri == re)
            return true;
    }
}

//===========================================================================
static bool conRVec(const Node & left, const Node & right) {
    return conRArray(left, right, right.values);
}

//===========================================================================
static bool conLVec(const Node & left, const Node & right) {
    return conLArray(left, left.values, right);
}

//===========================================================================
static bool conVec(const Node & left, const Node & right) {
    return conArray(
        left.values,
        left.numValues,
        right.values,
        right.numValues
    );
}

//===========================================================================
static bool conRSVec(const Node& left, const Node& right) {
    return conRArray(left, right, right.localValues);
}

//===========================================================================
static bool conLSVec(const Node& left, const Node& right) {
    return conLArray(left, left.localValues, right);
}

//===========================================================================
static bool conSVec(const Node& left, const Node& right) {
    return conArray(
        left.localValues,
        left.numValues,
        right.localValues,
        right.numValues
    );
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
//         empty    full      sm vec    vector    bitmap     meta
/*empty*/{ conTrue, conFalse, conFalse, conFalse, conFalse, conFalse },
/*full */{ conTrue, conTrue,  conTrue,  conTrue,  conTrue,  conTrue  },
/*svec */{ conTrue, conFalse, conSVec,  conLSVec, conLSVec, conLSVec  },
/*vec  */{ conTrue, conFalse, conRSVec, conVec,   conLVec,  conLVec  },
/*bit  */{ conTrue, conFalse, conRSVec, conRVec,  conBit,   conError },
/*meta */{ conTrue, conFalse, conRSVec, conRVec,  conError, conMeta  },
    };
    assert(!"not tested, contains(const Node&, const Node&)");
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
static bool isecRArray(
    const Node & left,
    const Node & right,
    const unsigned * ri
) {
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
static bool isecArray(
    const unsigned * li,
    unsigned lcount,
    const unsigned * ri,
    unsigned rcount
) {
    auto le = li + lcount;
    auto re = ri + rcount;

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
static bool isecRVec(const Node& left, const Node& right) {
    return isecRArray(left, right, right.values);
}

//===========================================================================
static bool isecLVec(const Node& left, const Node& right) {
    return isecRArray(right, left, left.values);
}

//===========================================================================
static bool isecVec(const Node& left, const Node& right) {
    return isecArray(
        left.values,
        left.numValues,
        right.values,
        right.numValues
    );
}

//===========================================================================
static bool isecRSVec(const Node& left, const Node& right) {
    return isecRArray(left, right, right.localValues);
}

//===========================================================================
static bool isecLSVec(const Node& left, const Node& right) {
    return isecRArray(right, left, left.localValues);
}

//===========================================================================
static bool isecSVec(const Node& left, const Node& right) {
    return isecArray(
        left.localValues,
        left.numValues,
        right.localValues,
        right.numValues
    );
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
//         empty      full       sm vec     vector     bitmap     meta
/*empty*/{ isecFalse, isecFalse, isecFalse, isecFalse, isecFalse, isecFalse },
/*full */{ isecFalse, isecTrue,  isecTrue,  isecTrue,  isecTrue,  isecTrue  },
/*svec */{ isecFalse, isecTrue,  isecSVec,  isecLSVec, isecLSVec, isecLSVec },
/*vec  */{ isecFalse, isecTrue,  isecRSVec, isecVec,   isecLVec,  isecLVec  },
/*bit  */{ isecFalse, isecTrue,  isecRSVec, isecRVec,  isecBit,   isecError },
/*meta */{ isecFalse, isecTrue,  isecRSVec, isecRVec,  isecError, isecMeta  },
    };
    assert(!"not tested, intersects(const Node&, const Node&)");
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
static void insRSVec(Node & left, const Node & right) {
    assert(right.type == Node::kSmVector);
    auto ri = right.localValues;
    auto re = ri + right.numValues;
    impl(left)->insert(left, ri, re);
}

//===========================================================================
static void insLSVec(Node & left, const Node & right) {
    assert(left.type == Node::kSmVector);
    Node tmp;
    insCopy(tmp, right);
    swap(left, tmp);
    insRSVec(left, move(tmp));
    impl(tmp)->destroy(tmp);
}

//===========================================================================
static void insArray(
    Node & left,
    unsigned * li,
    size_t maxValues,
    const unsigned * ri,
    const unsigned * re
) {
    assert(left.type == Node::kVector);
    assert(left.numValues);
    assert(ri < re);
    auto rbase = ri;
    auto rcount = (uint16_t) min<size_t>(
        re - ri,
        maxValues - left.numValues
    );
    if (rcount) {
        auto le = li + left.numValues - 1;
        auto out = le + rcount;
        ri = re - rcount;
        re -= 1;
        for (;;) {
            auto cmp = *le <=> *re;
            if (cmp > 0) {
                *out-- = *le--;
            } else {
                *out-- = *re--;
                if (cmp == 0) {
                    le -= 1;
                } else if (le == out) {
                    left.numValues += rcount;
                    break;
                }
                if (re < ri) {
                    auto cnt = (li + left.numValues - 1) + rcount - out;
                    memmove(le + 1, out + 1, cnt * sizeof *ri);
                    left.numValues = (uint16_t) ((le + 1 - li) + cnt);
                    break;
                }
            }
        }
    }
    if (rbase != ri)
        impl(left)->insert(left, rbase, ri);
}

//===========================================================================
static void insVec(Node & left, const Node & right) {
    assert(left.type == Node::kVector);
    assert(right.type == Node::kVector);
    auto ri = right.values;
    auto re = ri + right.numValues;
    insArray(left, left.values, s_vectorImpl.maxValues(), ri, re);
}

//===========================================================================
static void insRVec(Node & left, const Node & right) {
    assert(right.type == Node::kVector);
    auto ri = right.values;
    auto re = ri + right.numValues;
    impl(left)->insert(left, ri, re);
}

//===========================================================================
static void insLVec(Node & left, const Node & right) {
    assert(left.type == Node::kVector);
    Node tmp;
    insCopy(tmp, right);
    swap(left, tmp);
    insRVec(left, move(tmp));
    impl(tmp)->destroy(tmp);
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
    using Fn = void(Node & left, const Node & right);
    static Fn * const functs[][Node::kNodeTypes] = {
// LEFT                         RIGHT
//         empty    full     sm vec    vector    bitmap    meta
/*empty*/{ insSkip, insFull, insCopy,  insCopy,  insCopy,  insCopy  },
/*full */{ insSkip, insSkip, insSkip,  insSkip,  insSkip,  insSkip  },
/*svec */{ insSkip, insFull, insRSVec, insLSVec, insLSVec, insLSVec },
/*vec  */{ insSkip, insFull, insRSVec, insVec,   insLVec,  insLVec  },
/*bit  */{ insSkip, insFull, insRSVec, insRVec,  insBit,   insError },
/*meta */{ insSkip, insFull, insRSVec, insRVec,  insError, insMeta  },
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
static void insRSVec(Node & left, Node && right) {
    insRSVec(left, right);
}

//===========================================================================
static void insLSVec(Node & left, Node && right) {
    swap(left, right);
    insRSVec(left, move(right));
}

//===========================================================================
static void insRVec(Node & left, Node && right) {
    insRVec(left, right);
}

//===========================================================================
static void insLVec(Node & left, Node && right) {
    swap(left, right);
    insRVec(left, move(right));
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
    using Fn = void(Node & left, Node && right);
    static Fn * const functs[][Node::kNodeTypes] = {
// LEFT                         RIGHT
//         empty    full     sm vec    vector    bitmap    meta
/*empty*/{ insSkip, insFull, insMove,  insMove,  insMove,  insMove  },
/*full */{ insSkip, insSkip, insSkip,  insSkip,  insSkip,  insSkip  },
/*svec */{ insSkip, insFull, insRSVec, insLSVec, insLSVec, insLSVec },
/*vec  */{ insSkip, insFull, insRSVec, insRVec,  insLVec,  insLVec  },
/*bit  */{ insSkip, insFull, insRSVec, insRVec,  insBit,   insError },
/*meta */{ insSkip, insFull, insRSVec, insRVec,  insError, insMeta  },
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
    impl(left)->erase(left, *ri, 1);

    erase(left, right);
}

//===========================================================================
static void eraArray(
    Node & left,
    unsigned * li,
    const Node & right,
    const unsigned * ri
) {
    auto le = set_difference(
        li, li + left.numValues,
        ri, ri + right.numValues,
        li
    );
    left.numValues = uint16_t(le - li);
    if (!left.numValues)
        eraEmpty(left, right);
}

//===========================================================================
static void eraLArray(
    Node & left,
    unsigned * li,
    const Node & right
) {
    // Go through values of left vector and skip (aka remove) the ones that
    // are found in right node (values to be erased).
    auto base = li;
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
    left.numValues = uint16_t(out - base);
    if (!left.numValues)
        eraEmpty(left, right);
}

//===========================================================================
static void eraRArray(
    Node & left,
    const Node & right,
    const unsigned * ri
) {
    auto re = ri + right.numValues;
    for (; ri != re; ++ri)
        impl(left)->erase(left, *ri, 1);
}

//===========================================================================
static void eraSVec(Node & left, const Node & right) {
    eraArray(left, left.localValues, right, right.localValues);
}

//===========================================================================
static void eraLSVec(Node & left, const Node & right) {
    assert(left.type == Node::kSmVector);
    eraLArray(left, left.localValues, right);
}

//===========================================================================
static void eraRSVec(Node & left, const Node & right) {
    assert(right.type == Node::kSmVector);
    eraRArray(left, right, right.localValues);
}

//===========================================================================
static void eraVec(Node & left, const Node & right) {
    eraArray(left, left.values, right, right.values);
}

//===========================================================================
static void eraLVec(Node & left, const Node & right) {
    assert(left.type == Node::kVector);
    eraLArray(left, left.values, right);
}

//===========================================================================
static void eraRVec(Node & left, const Node & right) {
    assert(right.type == Node::kVector);
    eraRArray(left, right, right.values);
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
    using Fn = void(Node & left, const Node & right);
    static Fn * const functs[][Node::kNodeTypes] = {
// LEFT                         RIGHT
//         empty    full      sm vec     vector     bitmap     meta
/*empty*/{ eraSkip, eraSkip,  eraSkip,   eraSkip,   eraSkip,   eraSkip   },
/*full */{ eraSkip, eraEmpty, eraChange, eraChange, eraChange, eraChange },
/*svec */{ eraSkip, eraEmpty, eraSVec,   eraLSVec,  eraLSVec,  eraLSVec  },
/*vec  */{ eraSkip, eraEmpty, eraRSVec,  eraVec,    eraLVec,   eraLVec   },
/*bit  */{ eraSkip, eraEmpty, eraRSVec,  eraRVec,   eraBit,    eraError  },
/*meta */{ eraSkip, eraEmpty, eraRSVec,  eraRVec,   eraError,  eraMeta   },
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
static void isecArray(
    Node & left,
    unsigned * li,
    const Node & right,
    const unsigned * ri
) {
    auto le = set_intersection(
        li, li + left.numValues,
        ri, ri + right.numValues,
        li
    );
    left.numValues = uint16_t(le - li);
    if (!left.numValues)
        isecEmpty(left, right);
}

//===========================================================================
static void isecLArray(
    Node & left,
    unsigned * li,
    const Node & right
) {
    // Go through values of left vector and remove the ones that aren't
    // found in right node.
    auto base = li;
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
    left.numValues = uint16_t(out - base);
    if (!left.numValues)
        isecEmpty(left, right);
}

//===========================================================================
static void isecSVec(Node & left, const Node & right) {
    assert(left.type == Node::kSmVector);
    isecArray(left, left.localValues, right, right.localValues);
}

//===========================================================================
static void isecLSVec(Node & left, const Node & right) {
    assert(left.type == Node::kSmVector);
    isecLArray(left, left.localValues, right);
}

//===========================================================================
static void isecRSVec(Node & left, const Node & right) {
    assert(right.type == Node::kSmVector);
    Node tmp;
    isecCopy(tmp, right);
    swap(left, tmp);
    isecLArray(left, left.localValues, tmp);
    impl(tmp)->destroy(tmp);
}

//===========================================================================
static void isecVec(Node & left, const Node & right) {
    assert(left.type == Node::kVector);
    isecArray(left, left.values, right, right.values);
}

//===========================================================================
static void isecLVec(Node & left, const Node & right) {
    assert(left.type == Node::kVector);
    isecLArray(left, left.values, right);
}

//===========================================================================
static void isecRVec(Node & left, const Node & right) {
    assert(right.type == Node::kVector);
    Node tmp;
    isecCopy(tmp, right);
    swap(left, tmp);
    isecLArray(left, left.values, tmp);
    impl(tmp)->destroy(tmp);
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
    using Fn = void(Node & left, const Node & right);
    static Fn * const functs[][Node::kNodeTypes] = {
// LEFT                         RIGHT
//         empty      full      sm vec     vector     bitmap     meta
/*empty*/{ isecSkip,  isecSkip, isecSkip,  isecSkip,  isecSkip,  isecSkip  },
/*full */{ isecEmpty, isecSkip, isecCopy,  isecCopy,  isecCopy,  isecCopy  },
/*svec */{ isecEmpty, isecSkip, isecSVec,  isecLSVec, isecLSVec, isecLSVec },
/*vec  */{ isecEmpty, isecSkip, isecRSVec, isecVec,   isecLVec,  isecLVec  },
/*bit  */{ isecEmpty, isecSkip, isecRSVec, isecRVec,  isecBit,   isecError },
/*meta */{ isecEmpty, isecSkip, isecRSVec, isecRVec,  isecError, isecMeta  },
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
static void isecSVec(Node & left, Node && right) {
    isecSVec(left, right);
}

//===========================================================================
static void isecLSVec(Node & left, Node && right) {
    isecLSVec(left, right);
}

//===========================================================================
static void isecRSVec(Node & left, Node && right) {
    swap(left, right);
    isecLSVec(left, right);
}

//===========================================================================
static void isecVec(Node & left, Node && right) {
    isecVec(left, right);
}

//===========================================================================
static void isecLVec(Node & left, Node && right) {
    isecLVec(left, right);
}

//===========================================================================
static void isecRVec(Node & left, Node && right) {
    swap(left, right);
    isecLVec(left, right);
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
    using Fn = void(Node & left, Node && right);
    static Fn * const functs[][Node::kNodeTypes] = {
// LEFT                         RIGHT
//         empty      full      sm vec     vector     bitmap     meta
/*empty*/{ isecSkip,  isecSkip, isecEmpty, isecEmpty, isecEmpty, isecEmpty },
/*full */{ isecEmpty, isecSkip, isecMove,  isecMove,  isecMove,  isecMove  },
/*svec */{ isecEmpty, isecSkip, isecSVec,  isecLSVec, isecLSVec, isecLSVec },
/*vec  */{ isecEmpty, isecSkip, isecRSVec, isecVec,   isecLVec,  isecLVec  },
/*bit  */{ isecEmpty, isecSkip, isecRSVec, isecRVec,  isecBit,   isecError },
/*meta */{ isecEmpty, isecSkip, isecRSVec, isecRVec,  isecError, isecMeta  },
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
UnsignedSet::UnsignedSet(std::initializer_list<value_type> il) {
    insert(il);
}

//===========================================================================
UnsignedSet::UnsignedSet(string_view from) {
    insert(from);
}

//===========================================================================
UnsignedSet::UnsignedSet(value_type start, size_t count) {
    insert(start, count);
}

//===========================================================================
UnsignedSet::~UnsignedSet() {
    clear();
}

//===========================================================================
UnsignedSet & UnsignedSet::operator=(UnsignedSet && from) noexcept {
    assign(move(from));
    return *this;
}

//===========================================================================
UnsignedSet & UnsignedSet::operator=(const UnsignedSet & from) {
    assign(from);
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
    return impl(m_node)->count(m_node);
}

//===========================================================================
size_t UnsignedSet::max_size() const {
    return numeric_limits<value_type>::max();
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
void UnsignedSet::assign(UnsignedSet && from) {
    clear();
    swap(from);
}

//===========================================================================
void UnsignedSet::assign(const UnsignedSet & from) {
    clear();
    insert(from);
}

//===========================================================================
void UnsignedSet::assign(value_type value) {
    clear();
    insert(value);
}

//===========================================================================
inline void UnsignedSet::assign(std::initializer_list<value_type> il) {
    clear();
    insert(il);
}

//===========================================================================
void UnsignedSet::assign(value_type start, size_t count) {
    clear();
    insert(start, count);
}

//===========================================================================
void UnsignedSet::assign(string_view src) {
    clear();
    insert(src);
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
bool UnsignedSet::insert(value_type value) {
    return impl(m_node)->insert(m_node, value, 1);
}

//===========================================================================
void UnsignedSet::insert(std::initializer_list<value_type> il) {
    iInsert(il.begin(), il.end());
}

//===========================================================================
void UnsignedSet::insert(value_type start, size_t count) {
    if (!count)
        return;
    if (count == dynamic_extent)
        count = valueMask(0) - start + 1;
    impl(m_node)->insert(m_node, start, count);
}

//===========================================================================
void UnsignedSet::insert(string_view src) {
    char * eptr;
    for (;;) {
        auto first = strToUint(src, &eptr);
        if (!first && (src.data() == eptr || eptr[-1] != '0'))
            return;

        if (*eptr == '-') {
            src.remove_prefix(eptr - src.data() + 1);
            auto second = strToUint(src, &eptr);
            if (second < first)
                break;
            insert(first, second - first + 1);
        } else {
            insert(first);
        }

        src.remove_prefix(eptr - src.data());
    }
}

//===========================================================================
bool UnsignedSet::erase(value_type value) {
    return impl(m_node)->erase(m_node, value, 1);
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
void UnsignedSet::erase(value_type start, size_t count) {
    if (!count)
        return;
    if (count == dynamic_extent)
        count = valueMask(0) - start + 1;
    impl(m_node)->erase(m_node, start, count);
}

//===========================================================================
UnsignedSet::value_type UnsignedSet::pop_front() {
    auto val = front();
    erase(val);
    return val;
}

//===========================================================================
UnsignedSet::value_type UnsignedSet::pop_back() {
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
strong_ordering UnsignedSet::compare(const UnsignedSet & right) const {
    if (auto cmp = ::compare(m_node, right.m_node)) {
        return cmp < 0 ? strong_ordering::less : strong_ordering::greater;
    }
    return strong_ordering::equal;
}

//===========================================================================
strong_ordering UnsignedSet::operator<=>(const UnsignedSet & other) const {
    return compare(other);
}

//===========================================================================
bool UnsignedSet::operator==(const UnsignedSet & right) const {
    return compare(right) == 0;
}

//===========================================================================
UnsignedSet::value_type UnsignedSet::front() const {
    assert(!empty());
    auto i = begin();
    return *i;
}

//===========================================================================
UnsignedSet::value_type UnsignedSet::back() const {
    assert(!empty());
    auto i = rbegin();
    return *i;
}

//===========================================================================
size_t UnsignedSet::count() const {
    return size();
}

//===========================================================================
size_t UnsignedSet::count(value_type val) const {
    return count(val, 1);
}

//===========================================================================
size_t UnsignedSet::count(value_type start, size_t count) const {
    return impl(m_node)->count(m_node, start, count);
}

//===========================================================================
auto UnsignedSet::find(value_type val) const -> iterator {
    auto first = lowerBound(val);
    return first && *first == val ? first : end();
}

//===========================================================================
bool UnsignedSet::contains(value_type val) const {
    return count(val);
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
auto UnsignedSet::findLessEqual(value_type val) const -> iterator {
    return iterator::makeLast(&m_node, val);
}

//===========================================================================
auto UnsignedSet::lowerBound(value_type val) const -> iterator {
    return iterator::makeFirst(&m_node, val);
}

//===========================================================================
auto UnsignedSet::upperBound(value_type val) const -> iterator {
    val += 1;
    return val ? lowerBound(val) : end();
}

//===========================================================================
auto UnsignedSet::equalRange(value_type val) const
    -> pair<iterator, iterator>
{
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
void UnsignedSet::iInsert(const value_type * first, const value_type * last) {
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
