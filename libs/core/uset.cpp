// Copyright Glen Knowles 2017.
// Distributed under the Boost Software License, Version 1.0.
//
// uset.cpp - dim core
#include "pch.h"
#pragma hdrstop

using namespace std;
using namespace Dim;


/****************************************************************************
*
*   Declarations
*
***/

const unsigned kDataSize = 1024;

const unsigned kBitWidth = 32;
constexpr unsigned kValueMask[] = {
    0xffff'ffff,
     0x7ff'ffff,
       0xf'ffff,
         0x1fff,
};
static_assert(kValueMask[size(kValueMask) - 1] + 1 == kDataSize * 8);
constexpr unsigned kNumNodes[] = {
    0,
    1 << hammingWeight(kValueMask[1] ^ kValueMask[0]),
    1 << hammingWeight(kValueMask[2] ^ kValueMask[1]),
    1 << hammingWeight(kValueMask[3] ^ kValueMask[2]),
};

namespace {
enum NodeType {
    kEmpty,     // contains no values
    kFull,      // contains all values in node's domain
    kVector,    // has vector of values
    kBitmap,    // has bitmap of values
    kMeta,      // vector of nodes
    kMetaEnd,   // marks end of node vector
};

struct IImplBase {
    using Node = UnsignedSet::Node;
    using value_type = unsigned;

    constexpr static unsigned relBase(unsigned value, unsigned depth) {
        assert(depth < ::size(kValueMask));
        return value & ~kValueMask[depth];
    }
    constexpr static unsigned relValue(unsigned value, unsigned depth) {
        assert(depth < ::size(kValueMask));
        return value & kValueMask[depth];
    }

    // Requires that type, base, and depth are set, but otherwise treats
    // the node as uninitialized.
    virtual void init(Node & node, bool full) = 0;

    virtual void destroy(Node & node) = 0;
    virtual bool insert(Node & node, unsigned value) = 0;
    virtual void insert(
        Node & node, 
        const unsigned * first, 
        const unsigned * last
    ) = 0;
    virtual void erase(Node & node, unsigned value) = 0;

    virtual size_t size(const Node & node) const = 0;
    virtual bool lowerBound(
        unsigned * out, 
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

static IImplBase * impl(const UnsignedSet::Node & node);


/****************************************************************************
*
*   EmptyImpl
*
***/

namespace {
struct EmptyImpl : IImplBase {
    void init(Node & node, bool full) override;
    void destroy(Node & node) override;
    bool insert(Node & node, unsigned value) override;
    void insert(
        Node & node, 
        const unsigned * first, 
        const unsigned * last
    ) override;
    void erase(Node & node, unsigned value) override;

    size_t size(const Node & node) const override;
    bool lowerBound(
        unsigned * out, 
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
bool EmptyImpl::lowerBound(
    unsigned * out, 
    const Node & node, 
    unsigned first
) const {
    return false;
}


/****************************************************************************
*
*   FullImpl
*
***/

namespace {
struct FullImpl : IImplBase {
    void init(Node & node, bool full) override;
    void destroy(Node & node) override;
    bool insert(Node & node, unsigned value) override;
    void insert(
        Node & node, 
        const unsigned * first, 
        const unsigned * last
    ) override;
    void erase(Node & node, unsigned value) override;

    size_t size(const Node & node) const override;
    bool lowerBound(
        unsigned * out, 
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
    assert(node.depth < ::size(kValueMask));
    return kValueMask[node.depth] + 1;
}

//===========================================================================
bool FullImpl::lowerBound(
    unsigned * out, 
    const Node & node, 
    unsigned first
) const {
    if (first < size(node)) {
        *out = first;
        return true;
    }
    return false;
}

//===========================================================================
void FullImpl::convert(Node & node) {
    if (node.depth == ::size(kValueMask) - 1) {
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
struct VectorImpl : IImplBase {
    static constexpr size_t maxValues() { 
        return kDataSize / sizeof(*Node::values); 
    }

    void init(Node & node, bool full) override;
    void destroy(Node & node) override;
    bool insert(Node & node, unsigned value) override;
    void insert(
        Node & node, 
        const unsigned * first, 
        const unsigned * last
    ) override;
    void erase(Node & node, unsigned value) override;

    size_t size(const Node & node) const override;
    bool lowerBound(
        unsigned * out, 
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
        memmove(ptr + 1, ptr, last - ptr);
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
            memmove(ptr, ptr + 1, last - ptr - 1);
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
bool VectorImpl::lowerBound(
    unsigned * out, 
    const Node & node,
    unsigned first
) const {
    assert(relBase(first, node.depth) == node.base);
    auto last = node.values + node.numValues;
    auto ptr = lower_bound(node.values, last, first);
    if (ptr != last) {
        *out = *ptr;
        return true;
    }
    return false;
}

//===========================================================================
void VectorImpl::convert(Node & node) {
    auto ptr = node.values;
    auto last = ptr + node.numValues;
    if (node.depth == ::size(kValueMask) - 1) {
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
struct BitmapImpl : IImplBase {
    static constexpr size_t numInts() { return kDataSize / sizeof(uint64_t); }
    static constexpr size_t numBits() { return 64 * numInts(); }

    void init(Node & node, bool full) override;
    void destroy(Node & node) override;
    bool insert(Node & node, unsigned value) override;
    void insert(
        Node & node, 
        const unsigned * first, 
        const unsigned * last
    ) override;
    void erase(Node & node, unsigned value) override;

    size_t size(const Node & node) const override;
    bool lowerBound(
        unsigned * out, 
        const Node & node, 
        unsigned first
    ) const override;
};
} // namespace
static BitmapImpl s_bitmapImpl;

//===========================================================================
void BitmapImpl::init(Node & node, bool full) {
    node.type = kBitmap;
    node.numBytes = kDataSize;
    node.numValues = kDataSize / sizeof(uint64_t);
    node.values = (unsigned *) malloc(kDataSize);
    memset(node.values, full ? 0xff : 0, kDataSize);
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
    size_t num = 0;
    auto ptr = (uint64_t *) node.values;
    auto last = ptr + numInts();
    for (; ptr != last; ++ptr) 
        num += hammingWeight(*ptr);
    return num;
}

//===========================================================================
bool BitmapImpl::lowerBound(
    unsigned * out, 
    const Node & node,
    unsigned first
) const {
    assert(relBase(first, node.depth) == node.base);
    auto base = (uint64_t *) node.values;
    auto rel = relValue(first + 1, node.depth);
    assert(rel < numBits());
    int pos;
    if (!findBit(&pos, base, numInts(), rel)) 
        return false;
    *out = pos + (node.base << 8);
    return true;
}


/****************************************************************************
*
*   MetaImpl
*
***/

namespace {
struct MetaImpl : IImplBase {
    static constexpr size_t maxNodes() { 
        return kDataSize / sizeof(*Node::nodes); 
    }

    void init(Node & node, bool full) override;
    void destroy(Node & node) override;
    bool insert(Node & node, unsigned value) override;
    void insert(
        Node & node, 
        const unsigned * first, 
        const unsigned * last
    ) override;
    void erase(Node & node, unsigned value) override;

    size_t size(const Node & node) const override;
    bool lowerBound(
        unsigned * out, 
        const Node & node, 
        unsigned first
    ) const override;
};
} // namespace
static MetaImpl s_metaImpl;

//===========================================================================
void MetaImpl::init(Node & node, bool full) {
    node.type = kMeta;
    node.numValues = kNumNodes[node.depth + 1];
    node.numBytes = (node.numValues + 1) * sizeof(*node.nodes);
    node.nodes = (Node *) malloc(node.numBytes);
    auto nptr = node.nodes;
    auto nlast = node.nodes + node.numValues;
    auto base = node.base << 8;
    auto depth = node.depth + 1;
    auto domain = kValueMask[depth] + 1;
    for (; nptr != nlast; ++nptr, base += domain) {
        nptr->base = base >> 8;
        nptr->depth = depth;
        nptr->type = full ? kFull : kEmpty;
        nptr->numBytes = 0;
        nptr->numValues = 0;
        nptr->values = nullptr;
    }

    // Internally arrays of nodes contain a trailing "node" at the end that
    // is really just a pointer back to the parent node.
    *nlast = {};
    nlast->type = kMetaEnd;
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
bool MetaImpl::insert(Node & node, unsigned value) {
    assert(relBase(value, node.depth) == node.base);
    auto pos = relValue(value, node.depth) / node.numValues;
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
    assert(relBase(value, node.depth) == node.base);
    auto pos = relValue(value, node.depth) / node.numValues;
    auto & rnode = node.nodes[pos];
    return impl(rnode)->erase(rnode, value);
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
bool MetaImpl::lowerBound(
    unsigned * out, 
    const Node & node,
    unsigned first
) const {
    assert(relBase(first, node.depth) == node.base);
    auto pos = relValue(first, node.depth) / node.numValues;
    auto ptr = node.nodes + pos;
    auto last = node.nodes + node.numValues;
    for (; ptr != last; ++ptr) {
        if (impl(*ptr)->lowerBound(out, *ptr, first))
            return true;
    }
    return false;
}


/****************************************************************************
*
*   Helpers
*
***/

//===========================================================================
static IImplBase * impl(const UnsignedSet::Node & node) {
    switch (node.type) {
    case kEmpty: return &s_emptyImpl;
    case kFull: return &s_fullImpl;
    case kVector: return &s_vectorImpl;
    case kBitmap: break;
    case kMeta: return &s_metaImpl;
    }

    logMsgCrash() << "invalid node type: " << node.type;
    return nullptr;
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
bool UnsignedSet::operator==(const UnsignedSet & right) const {
    return false;
}

//===========================================================================
UnsignedSet::Iterator UnsignedSet::begin() {
    return {&m_node};
}

//===========================================================================
UnsignedSet::Iterator UnsignedSet::end() {
    return {};
}

//===========================================================================
UnsignedSet::NodeRange UnsignedSet::nodes() {
    return {&m_node};
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
void UnsignedSet::insert(unsigned value) {
    impl(m_node)->insert(m_node, value);
}

//===========================================================================
void UnsignedSet::erase(unsigned value) {
    impl(m_node)->erase(m_node, value);
}

//===========================================================================
void UnsignedSet::swap(UnsignedSet & other) {
    ::swap(m_node, other.m_node);
}

//===========================================================================
void UnsignedSet::iInsert(const unsigned * first, const unsigned * last) {
    impl(m_node)->insert(m_node, first, last);
}


/****************************************************************************
*
*   UnsignedSet::NodeIterator
*
***/

//===========================================================================
UnsignedSet::NodeIterator::NodeIterator(Node * node)
    : m_node(node)
{
    if (m_node && m_node->type == kEmpty)
        ++*this;
}

//===========================================================================
bool UnsignedSet::NodeIterator::operator!= (
    const NodeIterator & right
) const {
    return m_node != right.m_node;
}

//===========================================================================
UnsignedSet::NodeIterator & UnsignedSet::NodeIterator::operator++() {
    assert(m_node->type != kMetaEnd);
    for (;;) {
        if (!m_node->depth) {
            m_node = nullptr;
        } else {
            do {
                m_node += 1;
            } while (m_node->type == kEmpty);
            if (m_node->type == kMetaEnd) {
                m_node = m_node->nodes;
                continue;
            }
        }
        return *this;
    }
}


/****************************************************************************
*
*   UnsignedSet::Iterator
*
***/

//===========================================================================
UnsignedSet::Iterator::Iterator(Node * node) 
    : m_node(node)
{
    if (m_node)
        impl(*m_node)->lowerBound(&m_value, *m_node, 0);
}

//===========================================================================
bool UnsignedSet::Iterator::operator!= (const Iterator & right) const {
    return m_value != right.m_value || m_node != right.m_node;
}

//===========================================================================
UnsignedSet::Iterator & UnsignedSet::Iterator::operator++() {
    if (impl(*m_node)->lowerBound(&m_value, *m_node, m_value + 1))
        return *this;

    for (;;) {
        ++m_node;
        if (!m_node || impl(*m_node)->lowerBound(&m_value, *m_node, 0)) {
            m_value = 0;
            return *this;
        }
    }
}
