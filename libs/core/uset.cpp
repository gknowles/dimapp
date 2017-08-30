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
void VectorImpl::init(Node & node, const Node & from) {
    assert(node.type == kVector);
    assert(from.type == kVector);
    node = from;
    node.values = (unsigned *) malloc(node.numBytes);
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
    assert(node.type == kBitmap);
    node.numBytes = kDataSize;
    node.numValues = kDataSize / sizeof(uint64_t);
    node.values = (unsigned *) malloc(node.numBytes);
    memset(node.values, full ? 0xff : 0, kDataSize);
}

//===========================================================================
void BitmapImpl::init(Node & node, const Node & from) {
    assert(node.type == kBitmap);
    assert(from.type == kBitmap);
    node = from;
    node.values = (unsigned *) malloc(node.numBytes);
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
    assert(node.type == kMeta);
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
void MetaImpl::init(Node & node, const Node & from) {
    assert(node.type == kMeta);
    assert(from.type == kMeta);
    node = from;
    node.nodes = (Node *) malloc(node.numBytes);
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
***/

static int compare(const Node & left, const Node & right);

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
    using CompareFn = int(const Node & left, const Node & right);
    CompareFn * functs[][kNodeTypes] = {
    // LEFT                         RIGHT
    //             empty     full      vector   bitmap     meta
    /* empty  */ { cmpEqual, cmpLess,  cmpLess, cmpLess,   cmpLess  },
    /* full   */ { cmpMore,  cmpEqual, cmpMore, cmpMore,   cmpMore  },
    /* vector */ { cmpMore,  cmpLess,  cmpVec,  cmpIter,   cmpIter  },
    /* bitmap */ { cmpMore,  cmpLess,  cmpIter, cmpBitmap, cmpError },
    /* meta   */ { cmpMore,  cmpLess,  cmpIter, cmpError,  cmpMeta  },
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
    auto ri = UnsignedSet::Iterator(&right, 0);
    for (; ri; ++ri) 
        impl(left)->insert(left, *ri);
}

//===========================================================================
static void insCopyIter(Node & left, const Node & right) {
    Node tmp;
    insCopy(tmp, right);
    swap(left, tmp);
    insIter(left, tmp);
}

//===========================================================================
static void insVec(Node & left, const Node & right) {
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
    //             empty    full     vector   bitmap       meta
    /* empty  */ { insSkip, insCopy, insCopy, insCopy,     insCopy     }, 
    /* full   */ { insSkip, insSkip, insSkip, insSkip,     insSkip     }, 
    /* vector */ { insSkip, insCopy, insVec,  insCopyIter, insCopyIter }, 
    /* bitmap */ { insSkip, insCopy, insIter, insBitmap,   insError    }, 
    /* meta   */ { insSkip, insCopy, insIter, insError,    insMeta     }, 
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
static void insSIter(Node & left, Node && right) {
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
    /* vector */ { insSkip, insMove, insVec,  insSIter,  insSIter }, 
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
    auto ri = UnsignedSet::Iterator(&right, 0);
    assert(ri);
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
    unsigned val = 0;
    for (; li != le; ++li) {
        if (*li < val) {
            *out++ = *li;
            continue;
        }
        if (!ptr->lowerBound(&val, right, *li)) {
            do {
                *out++ = *li++;
            } while (li != le);
            break;
        }
        if (val != *li)
            *out++ = *li;
    }
    left.numValues = uint16_t(out - left.values);
}

//===========================================================================
static void eraIter(Node & left, const Node & right) {
    assert(right.type == kVector);
    auto ri = right.values;
    auto re = ri + right.numValues;
    while (ri != re)
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
    functs[left.type][right.type](left, move(right));
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
    impl(left)->destroy(left);
    left.type = kEmpty;
    impl(left)->init(left, false);
}

//===========================================================================
static void isecCopy(Node & left, const Node & right) {
    impl(left)->destroy(left);
    left.type = right.type;
    impl(left)->init(left, right);
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
    unsigned val = 0;
    for (; li != le; ++li) {
        if (*li < val) 
            continue;
        if (!ptr->lowerBound(&val, right, *li))
            break;
        if (val == *li)
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
static void isecSFind(Node & left, Node && right) {
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
    /* bitmap */ { isecEmpty, isecSkip, isecSFind, isecBitmap, isecError }, 
    /* meta   */ { isecEmpty, isecSkip, isecSFind, isecError,  isecMeta  }, 
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
UnsignedSet::NodeRange UnsignedSet::nodes() const {
    return {&m_node};
}

//===========================================================================
UnsignedSet::RangeRange UnsignedSet::ranges() const {
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
        first = strToUint(eptr + 1, &eptr);
        if (!first)
            break;
    }
}

//===========================================================================
void UnsignedSet::insert(unsigned first, unsigned second) {
    // TODO: make this efficient
    for (auto i = first; i <= second; ++i)
        insert(i);
}

//===========================================================================
void UnsignedSet::erase(unsigned value) {
    impl(m_node)->erase(m_node, value);
}

//===========================================================================
void UnsignedSet::erase(const UnsignedSet & other) {
    ::erase(m_node, other.m_node);
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
    assert(0 && "not implemented");
    return false;
}

//===========================================================================
bool UnsignedSet::intersects(const UnsignedSet & other) const {
    assert(0 && "not implemented");
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


/****************************************************************************
*
*   UnsignedSet::RangeIterator
*
***/

//===========================================================================
UnsignedSet::RangeIterator::RangeIterator(
    const Node * node, 
    UnsignedSet::value_type val
) 
    : m_iter(node, val)
{
    if (m_iter) 
        ++*this;
}

//===========================================================================
bool UnsignedSet::RangeIterator::operator!= (
    const RangeIterator & right
) const {
    return m_iter != right.m_iter;
}

//===========================================================================
UnsignedSet::RangeIterator & UnsignedSet::RangeIterator::operator++() {
    m_value.first = m_value.second = *m_iter;
    while (++m_iter && *m_iter == m_value.second + 1) 
        m_value.second += 1;
    return *this;
}


/****************************************************************************
*
*   Free functions
*
***/

//===========================================================================
ostream & operator<<(ostream & os, const UnsignedSet & right) {
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
