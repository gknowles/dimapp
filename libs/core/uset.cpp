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

const unsigned kDataSize = 1024;

const unsigned kBitWidth = 32;
constexpr unsigned kValueMask[] = {
    0xffff'ffff,
     0x7ff'ffff,
       0xf'ffff,
         0x1fff,
};
static_assert(kValueMask[size(kValueMask) - 1] + 1 == kDataSize * 8);
constexpr uint16_t kNumNodes[] = {
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
    kNodeTypes,
    kMetaEnd,   // marks end of node vector
};

struct IImplBase {
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

static IImplBase * impl(const Node & node);


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
static IImplBase * impl(const Node & node) {
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
*   Compare
*
***/

static int compare(const Node & left, const Node & right);

using CompareFn = int(const Node & left, const Node & right);

//===========================================================================
static int cmpError(const Node & left, const Node & right) {
    logMsgCrash() << "compare: incompatible node types, " << left.type
        << ", " << right.type;
    return 0;
}

//===========================================================================
static int cmpLess(const Node & left, const Node & right) { return -1; }
static int cmpMore(const Node & left, const Node & right) { return 1; }
static int cmpEqual(const Node & left, const Node & right) { return 0; }

//===========================================================================
static int cmpIter(const Node & left, const Node & right) {
    auto li = UnsignedSet::Iterator(&left, 0);
    auto ri = UnsignedSet::Iterator(&right, 0);
    for (; li && ri; ++li, ++ri) {
        if (*li != *ri)
            return *li > *ri ? 1 : -1;
    }
    return (bool) li - (bool) ri;
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
    return (li == le) - (ri == re);
}

//===========================================================================
static int cmpBitmap(const Node & left, const Node & right) {
    auto li = (uint64_t *) left.values;
    auto le = li + kDataSize / sizeof(*li);
    auto ri = (uint64_t *) right.values;
    auto re = ri + kDataSize / sizeof(*ri);
    for (; li != le && ri != re; ++li, ++ri) {
        if (*li != *ri) 
            return reverseBits(*li) > reverseBits(*ri) ? 1 : -1;
    }
    return (li == le) - (ri == re);
}

//===========================================================================
static int cmpMeta(const Node & left, const Node & right) {
    auto li = left.nodes;
    auto le = li + left.numValues;
    auto ri = right.nodes;
    auto re = ri + right.numValues;
    for (; li != le && ri != re; ++li, ++ri) {
        if (int rc = compare(*li, *ri))
            return rc;
    }
    return (li == le) - (ri == re);
}

//===========================================================================
static int compare(const Node & left, const Node & right) {
    CompareFn * functs[][kNodeTypes] = {
        // empty    full      vector   bitmap     meta
        { cmpEqual, cmpLess,  cmpLess, cmpLess,   cmpLess  }, // empty
        { cmpMore,  cmpEqual, cmpMore, cmpMore,   cmpMore  }, // full
        { cmpMore,  cmpLess,  cmpVec,  cmpIter,   cmpIter  }, // vector
        { cmpMore,  cmpLess,  cmpIter, cmpBitmap, cmpError }, // bitmap
        { cmpMore,  cmpLess,  cmpIter, cmpError,  cmpMeta  }, // meta
    };
    return functs[left.type][right.type](left, right);
}


/****************************************************************************
*
*   Insert
*
***/

static void insert(Node & left, Node && right);

using InsertFn = void(Node & left, Node && right);

//===========================================================================
static void insError(Node & left, Node && right) {
    logMsgCrash() << "insert: incompatible node types, " << left.type
        << ", " << right.type;
}

//===========================================================================
static void insSkip(Node & left, Node && right) 
{}

//===========================================================================
static void insCopy(Node & left, Node && right) {
    swap(left, right);
}

//===========================================================================
static void insIter(Node & left, Node && right) {
    auto ri = UnsignedSet::Iterator(&right, 0);
    for (; ri; ++ri) 
        impl(left)->insert(left, *ri);
}

//===========================================================================
static void insCopyIter(Node & left, Node && right) {
    swap(left, right);
    insIter(left, move(right));
}

//===========================================================================
static void insVec(Node & left, Node && right) {
    auto ri = right.values;
    auto re = ri + right.numValues;
    for (; ri != re; ++ri) 
        impl(left)->insert(left, *ri);
}

//===========================================================================
static void insBitmap(Node & left, Node && right) {
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
    InsertFn * functs[][kNodeTypes] = {
        // empty   full     vector   bitmap       meta
        { insSkip, insCopy, insCopy, insCopy,     insCopy     }, // empty
        { insSkip, insSkip, insSkip, insSkip,     insSkip     }, // full
        { insSkip, insCopy, insVec,  insCopyIter, insCopyIter }, // vector
        { insSkip, insCopy, insIter, insBitmap,   insError    }, // bitmap
        { insSkip, insCopy, insIter, insError,    insMeta     }, // meta
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
auto UnsignedSet::begin() -> iterator {
    return {&m_node};
}

//===========================================================================
auto UnsignedSet::end() -> iterator {
    return {};
}

//===========================================================================
auto UnsignedSet::begin() const -> const_iterator {
    return const_cast<UnsignedSet*>(this)->begin();
}

//===========================================================================
auto UnsignedSet::end() const -> const_iterator {
    return const_cast<UnsignedSet*>(this)->end();
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
void UnsignedSet::insert(UnsignedSet && other) {
    ::insert(m_node, move(other.m_node));
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
int UnsignedSet::compare(const UnsignedSet & right) const {
    return ::compare(m_node, right.m_node);
}

//===========================================================================
bool UnsignedSet::intersects(const UnsignedSet & other) const {
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
auto UnsignedSet::find(unsigned val) -> iterator {
    auto first = iterator{&m_node, val};
    return first && *first == val ? first : iterator{};
}

//===========================================================================
auto UnsignedSet::find(unsigned val) const -> const_iterator {
    return const_cast<UnsignedSet*>(this)->find(val);
}

//===========================================================================
auto UnsignedSet::equalRange(unsigned val) -> std::pair<iterator, iterator> {
    auto first = lowerBound(val);
    auto last = first;
    if (last && *last == val)
        ++last;
    return {first, last};
}

//===========================================================================
auto UnsignedSet::equalRange(unsigned val) const 
    -> std::pair<const_iterator, const_iterator> 
{
    return const_cast<UnsignedSet*>(this)->equalRange(val);
}

//===========================================================================
auto UnsignedSet::lowerBound(unsigned val) -> iterator {
    return {&m_node, val};
}

//===========================================================================
auto UnsignedSet::lowerBound(unsigned val) const -> const_iterator {
    return const_cast<UnsignedSet*>(this)->lowerBound(val);
}

//===========================================================================
auto UnsignedSet::upperBound(unsigned val) -> iterator {
    val += 1;
    return val ? iterator{&m_node, val} : iterator{};
}

//===========================================================================
auto UnsignedSet::upperBound(unsigned val) const -> const_iterator {
    return const_cast<UnsignedSet*>(this)->upperBound(val);
}

//===========================================================================
// Private
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
UnsignedSet::NodeIterator::NodeIterator(const Node * node)
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
UnsignedSet::Iterator::Iterator(const Node * node, value_type value) 
    : m_node(node)
{
    if (m_node)
        impl(*m_node)->lowerBound(&m_value, *m_node, value);
}

//===========================================================================
bool UnsignedSet::Iterator::operator!= (const Iterator & right) const {
    return m_value != right.m_value || m_node != right.m_node;
}

//===========================================================================
UnsignedSet::Iterator & UnsignedSet::Iterator::operator++() {
    m_value += 1;
    if (m_value && impl(*m_node)->lowerBound(&m_value, *m_node, m_value))
        return *this;

    for (;;) {
        ++m_node;
        if (!m_node || impl(*m_node)->lowerBound(&m_value, *m_node, 0)) {
            m_value = 0;
            return *this;
        }
    }
}
