// Copyright Glen Knowles 2017 - 2022.
// Distributed under the Boost Software License, Version 1.0.
//
// uset.cpp - dim core
#include "pch.h"
#pragma hdrstop

using namespace std;
using namespace Dim;

template <typename T, typename A>
using Node = IntegralSet<T,A>::Node;


/****************************************************************************
*
*   Declarations
*
***/

namespace {
} // namespace

template <std::integral T, typename A>
class IntegralSet<T,A>::Impl {
public:
    enum NodeType : int {
        kEmpty,         // contains no values
        kFull,          // contains all values in node's domain
        kSmVector,      // small vector of values embedded in node struct
        kVector,        // vector of values
        kBitmap,        // bitmap covering all of node's possible values
        kMeta,          // vector of nodes
        kNodeTypes,
        kMetaParent,    // link to parent meta node
    };
    static_assert(kMetaParent < 1 << kTypeBits);

    constinit static const size_t kLeafBits = min((size_t) 12, kBitWidth);
    constinit static const size_t kDataSize = (1 << kLeafBits) / 8;

    constinit static const size_t kStepBits =
        popcount(bit_ceil(kDataSize / sizeof Node + 1) / 2 - 1);
    constinit static const size_t kMaxDepth =
        (kBitWidth - kLeafBits + kStepBits - 1) / kStepBits;
    static_assert(kBaseBits + kLeafBits >= kBitWidth);

    constexpr static storage_type valueMask(size_t depth) {
        assert(depth <= kMaxDepth);
        size_t bits = kBitWidth -
            (kLeafBits + kStepBits * (kMaxDepth - depth));
        return numeric_limits<storage_type>::max() >> bits;
    }
    constexpr static storage_type relBase(storage_type value, size_t depth) {
        return (value & ~valueMask(depth)) >> (kBitWidth - kBaseBits);
    }
    constexpr static storage_type relValue(storage_type value, size_t depth) {
        return value & valueMask(depth);
    }

    constexpr static size_t numNodes(size_t depth) {
        size_t ret = 0;
        if (depth) {
            ret = (size_t) 1
                << popcount(valueMask(depth) ^ valueMask(depth - 1));
        }
        return ret;
    }
    constexpr static size_t nodePos(storage_type value, size_t depth) {
        return relValue(value, depth) / (valueMask(depth + 1) + 1);
    }

    constexpr static storage_type absBase(storage_type value, size_t depth) {
        return value & ~valueMask(depth);
    }
    constexpr static storage_type absFinal(storage_type value, size_t depth) {
        return absBase(value, depth) + valueMask(depth);
    }

    constexpr static storage_type absBase(const Node & node) {
        return node.base << (kBitWidth - kBaseBits);
    }
    constexpr static storage_type absFinal(const Node & node) {
        return absBase(node) + valueMask(node.depth);
    }
    constexpr static storage_type absSize(const Node & node) {
        return valueMask(node.depth) + 1;
    }

    constexpr static size_t smvMaxValues() {
        return sizeof Node::localValues / sizeof *Node::localValues;
    }
    constexpr static size_t vecMaxValues() {
        return kDataSize / sizeof *Node::values;
    }
    constexpr static size_t bitNumInt64s() {
        return kDataSize / sizeof uint64_t;
    }
    constexpr static size_t bitNumBits() {
        return 64 * bitNumInt64s();
    }

    constexpr static value_type toValue(storage_type val) {
        if constexpr (is_signed_v<value_type>) {
            return (value_type) (val + numeric_limits<value_type>::min());
        } else {
            return (value_type) val;
        }
    }
    constexpr static storage_type toStorage(value_type val) {
        if constexpr (is_signed_v<value_type>) {
            return (storage_type) val
                + (storage_type) -numeric_limits<value_type>::min();
        } else {
            return (storage_type) val;
        }
    }

    // Allocator
    static void * allocate(A * alloc, size_t bytes);
    static void deallocate(A * alloc, void * ptr, size_t bytes);

    // misc
    static void swap(Node & left, Node & right);

    static bool yes(const Node & left, const Node & right);
    static bool yes(
        const Node ** onode,
        storage_type * ovalue,
        const Node & node,
        storage_type key
    );
    static bool no(const Node & left, const Node & right);
    static bool no(
        const Node ** onode,
        storage_type * ovalue,
        const Node & node,
        storage_type key
    );
    static bool no(
        A * alloc,
        Node * node,
        const storage_type * first,
        const storage_type * last
    );
    static bool no(A * alloc, Node * node, storage_type start, size_t len);

    static void skip(A * alloc, Node * node);
    static void skip(A * alloc, Node * left, const Node & right);
    static void skip(A * alloc, Node * left, Node && right);
    static void clear(A * alloc, Node * node);
    static void clear(A * alloc, Node * left, const Node & right);
    static void clear(A * alloc, Node * left, Node && right);
    static void fill(A * alloc, Node * node);
    static void fill(A * alloc, Node * left, const Node & right);
    static void fill(A * alloc, Node * left, Node && right);
    static void copy(A * alloc, Node * left, const Node & right);
    static void copy(A * alloc, Node * left, Node && right);

    static size_t nodePos(const Node & node, unsigned value);
    static void grow(A * alloc, Node * node, size_t newCount);
    static bool convertMetaIf(A * alloc, Node * node, NodeType type);

    // init
    static void init(A * alloc, Node * node, bool full);
    static void initEmpty(A * alloc, Node * node, bool full);
    static void initFull(A * alloc, Node * node, bool full);
    static void initSmv(A * alloc, Node * node, bool full);
    static void initVec(A * alloc, Node * node, bool full);
    static void initBit(A * alloc, Node * node, bool full);
    static void initMeta(A * alloc, Node * node, bool full);

    static void init(A * alloc, Node * node, const Node & from);
    static void initEmpty(A * alloc, Node * node, const Node & from);
    static void initFull(A * alloc, Node * node, const Node & from);
    static void initSmv(A * alloc, Node * node, const Node & from);
    static void initVec(A * alloc, Node * node, const Node & from);
    static void initBit(A * alloc, Node * node, const Node & from);
    static void initMeta(A * alloc, Node * node, const Node & from);

    // destroy
    static void destroy(A * alloc, Node * node);
    static void desFree(A * alloc, Node * node);
    static void desMeta(A * alloc, Node * node);

    // insert
    static bool insert(
        A * alloc,
        Node * node,
        const storage_type * first,
        const storage_type * last
    );
    static bool insEmpty(
        A * alloc,
        Node * node,
        const storage_type * first,
        const storage_type * last
    );
    static bool insSmv(
        A * alloc,
        Node * node,
        const storage_type * first,
        const storage_type * last
    );
    static bool insVec(
        A * alloc,
        Node * node,
        const storage_type * first,
        const storage_type * last
    );
    static bool insBit(
        A * alloc,
        Node * node,
        const storage_type * first,
        const storage_type * last
    );
    static bool insMeta(
        A * alloc,
        Node * node,
        const storage_type * first,
        const storage_type * last
    );

    static bool insert(A *, Node * node, storage_type start, size_t len);
    static bool insEmpty(A *, Node * node, storage_type start, size_t len);
    static bool insSmv(A *, Node * node, storage_type start, size_t len);
    static bool insVec(A *, Node * node, storage_type start, size_t len);
    static bool insBit(A *, Node * node, storage_type start, size_t len);
    static bool insMeta(A *, Node * node, storage_type start, size_t len);

    static void insert(A * alloc, Node * left, const Node & right);
    static void insError(A * alloc, Node * left, const Node & right);
    static void insRSmv(A * alloc, Node * left, const Node & right);
    static void insLSmv(A * alloc, Node * left, const Node & right);
    static void insVec(A * alloc, Node * left, const Node & right);
    static void insRVec(A * alloc, Node * left, const Node & right);
    static void insLVec(A * alloc, Node * left, const Node & right);
    static void insBit(A * alloc, Node * left, const Node & right);
    static void insMeta(A * alloc, Node * left, const Node & right);
    static void insArray(
        A * alloc,
        Node * left,
        storage_type * li,
        size_t maxValues,
        const storage_type * ri,
        const storage_type * re
    );

    static void insert(A * alloc, Node * left, Node && right);
    static void insError(A * alloc, Node * left, Node && right);
    static void insRSmv(A * alloc, Node * left, Node && right);
    static void insLSmv(A * alloc, Node * left, Node && right);
    static void insRVec(A * alloc, Node * left, Node && right);
    static void insLVec(A * alloc, Node * left, Node && right);
    static void insBit(A * alloc, Node * left, Node && right);
    static void insMeta(A * alloc, Node * left, Node && right);

    // erase
    static bool erase(A *, Node * node, storage_type start, size_t len);
    static bool eraFull(A *, Node * node, storage_type start, size_t len);
    static bool eraSmv(A *, Node * node, storage_type start, size_t len);
    static bool eraVec(A *, Node * node, storage_type start, size_t len);
    static bool eraBit(A *, Node * node, storage_type start, size_t len);
    static bool eraMeta(A *, Node * node, storage_type start, size_t len);

    static void erase(A * alloc, Node * left, const Node & right);
    static void eraError(A * alloc, Node * left, const Node & right);
    static void eraChange(A * alloc, Node * left, const Node & right);
    static void eraSmv(A * alloc, Node * left, const Node & right);
    static void eraLSmv(A * alloc, Node * left, const Node & right);
    static void eraRSmv(A * alloc, Node * left, const Node & right);
    static void eraVec(A * alloc, Node * left, const Node & right);
    static void eraLVec(A * alloc, Node * left, const Node & right);
    static void eraRVec(A * alloc, Node * left, const Node & right);
    static void eraBit(A * alloc, Node * left, const Node & right);
    static void eraMeta(A * alloc, Node * left, const Node & right);
    static void eraArray(
        A * alloc,
        Node * left,
        storage_type * li,
        const Node & right,
        const storage_type * ri
    );
    static void eraLArray(
        A * alloc,
        Node * left,
        storage_type * li,
        const Node & right
    );
    static void eraRArray(
        A * alloc,
        Node * left,
        const Node & right,
        const storage_type * ri
    );

    // len
    static size_t count(const Node & node);
    static size_t cntEmpty(const Node & node);
    static size_t cntFull(const Node & node);
    static size_t cntSmv(const Node & node);
    static size_t cntVec(const Node & node);
    static size_t cntBit(const Node & node);
    static size_t cntMeta(const Node & node);

    static size_t count(const Node & node, storage_type start, size_t len);
    static size_t cntEmpty(const Node & node, storage_type start, size_t len);
    static size_t cntFull(const Node & node, storage_type start, size_t len);
    static size_t cntSmv(const Node & node, storage_type start, size_t len);
    static size_t cntVec(const Node & node, storage_type start, size_t len);
    static size_t cntBit(const Node & node, storage_type start, size_t len);
    static size_t cntMeta(const Node & node, storage_type start, size_t len);

    // find
    static bool find(
        const Node ** onode,
        storage_type * ovalue,
        const Node & node,
        storage_type key
    );
    static bool findEmpty(
        const Node ** onode,
        storage_type * ovalue,
        const Node & node,
        storage_type key
    );
    static bool findFull(
        const Node ** onode,
        storage_type * ovalue,
        const Node & node,
        storage_type key
    );
    static bool findSmv(
        const Node ** onode,
        storage_type * ovalue,
        const Node & node,
        storage_type key
    );
    static bool findVec(
        const Node ** onode,
        storage_type * ovalue,
        const Node & node,
        storage_type key
    );
    static bool findBit(
        const Node ** onode,
        storage_type * ovalue,
        const Node & node,
        storage_type key
    );
    static bool findMeta(
        const Node ** onode,
        storage_type * ovalue,
        const Node & node,
        storage_type key
    );

    // rfind
    static bool rfind(
        const Node ** onode,
        storage_type * ovalue,
        const Node & node,
        storage_type key
    );
    static bool rfindSmv(
        const Node ** onode,
        storage_type * ovalue,
        const Node & node,
        storage_type key
    );
    static bool rfindVec(
        const Node ** onode,
        storage_type * ovalue,
        const Node & node,
        storage_type key
    );
    static bool rfindBit(
        const Node ** onode,
        storage_type * ovalue,
        const Node & node,
        storage_type key
    );
    static bool rfindMeta(
        const Node ** onode,
        storage_type * ovalue,
        const Node & node,
        storage_type key
    );

    // contig
    static bool contig(
        const Node ** onode,
        storage_type * ovalue,
        const Node & node,
        storage_type key
    );
    static bool contigFull(
        const Node ** onode,
        storage_type * ovalue,
        const Node & node,
        storage_type key
    );
    static bool contigSmv(
        const Node ** onode,
        storage_type * ovalue,
        const Node & node,
        storage_type key
    );
    static bool contigVec(
        const Node ** onode,
        storage_type * ovalue,
        const Node & node,
        storage_type key
    );
    static bool contigBit(
        const Node ** onode,
        storage_type * ovalue,
        const Node & node,
        storage_type key
    );
    static bool contigMeta(
        const Node ** onode,
        storage_type * ovalue,
        const Node & node,
        storage_type key
    );

    // rcontig
    static bool rcontig(
        const Node ** onode,
        storage_type * ovalue,
        const Node & node,
        storage_type key
    );
    static bool rcontigFull(
        const Node ** onode,
        storage_type * ovalue,
        const Node & node,
        storage_type key
    );
    static bool rcontigSmv(
        const Node ** onode,
        storage_type * ovalue,
        const Node & node,
        storage_type key
    );
    static bool rcontigVec(
        const Node ** onode,
        storage_type * ovalue,
        const Node & node,
        storage_type key
    );
    static bool rcontigBit(
        const Node ** onode,
        storage_type * ovalue,
        const Node & node,
        storage_type key
    );
    static bool rcontigMeta(
        const Node ** onode,
        storage_type * ovalue,
        const Node & node,
        storage_type key
    );

    // compare
    static int compare(const Node & left, const Node & right);
    static int cmpError(const Node & left, const Node & right);
    static int cmpLessIf(const Node & left, const Node & right);
    static int cmpMoreIf(const Node & left, const Node & right);
    static int cmpEqual(const Node & left, const Node & right);
    static int cmpVecIf(const Node & left, const Node & right);
    static int cmpRVecIf(const Node & left, const Node & right);
    static int cmpSmvIf(const Node & left, const Node & right);
    static int cmpRSmvIf(const Node & left, const Node & right);
    static int cmpBitIf(const Node & left, const Node & right);
    static int cmpMetaIf(const Node & left, const Node & right);
    static int cmpRBitIf(const Node & left, const Node & right);
    static int cmpRMetaIf(const Node & left, const Node & right);
    static int cmpIter(const Node & left, const Node & right);
    static int cmpVec(const Node & left, const Node & right);
    static int cmpSmv(const Node & left, const Node & right);
    static int cmpLSmv(const Node & left, const Node & right);
    static int cmpRSmv(const Node & left, const Node & right);
    static int cmpBit(const Node & left, const Node & right);
    static int cmpMeta(const Node & left, const Node & right);
    static int cmpBit(uint64_t left, uint64_t right);
    static int cmpArray(
        const storage_type * li,
        size_t lcount,
        const storage_type * ri,
        size_t rcount
    );

    // contains
    static bool contains(const Node & left, const Node & right);
    static bool conError(const Node & left, const Node & right);
    static bool conRVec(const Node & left, const Node & right);
    static bool conLVec(const Node & left, const Node & right);
    static bool conVec(const Node & left, const Node & right);
    static bool conRSmv(const Node & left, const Node & right);
    static bool conLSmv(const Node & left, const Node & right);
    static bool conSmv(const Node & left, const Node & right);
    static bool conBit(const Node & left, const Node & right);
    static bool conMeta(const Node & left, const Node & right);
    static bool conRArray(
        const Node & left,
        const Node & right,
        const storage_type * ri
    );
    static bool conLArray(
        const Node & left,
        const storage_type * li,
        const Node & right
    );
    static bool conArray(
        const storage_type * li,
        size_t lcount,
        const storage_type * ri,
        size_t rcount
    );

    // intersects
    static bool intersects(const Node & left, const Node & right);
    static bool isecError(const Node & left, const Node & right);
    static bool isecRVec(const Node & left, const Node & right);
    static bool isecLVec(const Node & left, const Node & right);
    static bool isecVec(const Node & left, const Node & right);
    static bool isecRSmv(const Node & left, const Node & right);
    static bool isecLSmv(const Node & left, const Node & right);
    static bool isecSmv(const Node & left, const Node & right);
    static bool isecBit(const Node & left, const Node & right);
    static bool isecMeta(const Node & left, const Node & right);
    static bool isecArray(
        const storage_type * li,
        size_t lcount,
        const storage_type * ri,
        size_t rcount
    );
    static bool isecRArray(
        const Node & left,
        const Node & right,
        const storage_type * ri
    );

    // intersect
    static void intersect(A * alloc, Node * left, const Node & right);
    static void isecError(A * alloc, Node * left, const Node & right);
    static void isecSmv(A * alloc, Node * left, const Node & right);
    static void isecRSmv(A * alloc, Node * left, const Node & right);
    static void isecLSmv(A * alloc, Node * left, const Node & right);
    static void isecVec(A * alloc, Node * left, const Node & right);
    static void isecRVec(A * alloc, Node * left, const Node & right);
    static void isecLVec(A * alloc, Node * left, const Node & right);
    static void isecBit(A * alloc, Node * left, const Node & right);
    static void isecMeta(A * alloc, Node * left, const Node & right);
    static void isecArray(
        A * alloc,
        Node * left,
        storage_type * li,
        const Node & right,
        const storage_type * ri
    );
    static void isecLArray(
        A * alloc,
        Node * left,
        storage_type * li,
        const Node & right
    );

    static void intersect(A * alloc, Node * left, Node && right);
    static void isecError(A * alloc, Node * left, Node && right);
    static void isecSmv(A * alloc, Node * left, Node && right);
    static void isecRSmv(A * alloc, Node * left, Node && right);
    static void isecLSmv(A * alloc, Node * left, Node && right);
    static void isecVec(A * alloc, Node * left, Node && right);
    static void isecRVec(A * alloc, Node * left, Node && right);
    static void isecLVec(A * alloc, Node * left, Node && right);
    static void isecBit(A * alloc, Node * left, Node && right);
    static void isecMeta(A * alloc, Node * left, Node && right);
};


/****************************************************************************
*
*   allocate / deallocate
*
***/

//===========================================================================
template <std::integral T, typename A>
void * IntegralSet<T,A>::Impl::allocate(A * alloc, size_t bytes) {
    using AChar = allocator_traits<A>::template rebind_alloc<char>;
    auto allocChar = static_cast<AChar>(*alloc);
    return allocator_traits<AChar>::allocate(allocChar, bytes);
}

//===========================================================================
template <std::integral T, typename A>
void IntegralSet<T,A>::Impl::deallocate(A * alloc, void * ptr, size_t bytes) {
    using AChar = allocator_traits<A>::template rebind_alloc<char>;
    auto allocChar = static_cast<AChar>(*alloc);
    allocator_traits<AChar>::deallocate(allocChar, (char *) ptr, bytes);
}


/****************************************************************************
*
*   misc
*
***/

//===========================================================================
template <std::integral T, typename A>
void IntegralSet<T,A>::Impl::swap(Node & left, Node & right) {
    Node tmp;
    memcpy(&tmp, &left, sizeof tmp);
    memcpy(&left, &right, sizeof left);
    memcpy(&right, &tmp, sizeof right);
}

//===========================================================================
template <std::integral T, typename A>
bool IntegralSet<T,A>::Impl::yes(const Node & left, const Node & right) {
    return true;
}

//===========================================================================
template <std::integral T, typename A>
bool IntegralSet<T,A>::Impl::yes(
    const Node ** onode,
    storage_type * ovalue,
    const Node & node,
    storage_type key
) {
    return true;
}

//===========================================================================
template <std::integral T, typename A>
bool IntegralSet<T,A>::Impl::no(const Node & left, const Node & right) {
    return false;
}

//===========================================================================
template <std::integral T, typename A>
bool IntegralSet<T,A>::Impl::no(
    const Node ** onode,
    storage_type * ovalue,
    const Node & node,
    storage_type key
) {
    return false;
}

//===========================================================================
template <std::integral T, typename A>
bool IntegralSet<T,A>::Impl::no(
    A * alloc,
    Node * node,
    const storage_type * first,
    const storage_type * last
) {
    return false;
}

//===========================================================================
template <std::integral T, typename A>
bool IntegralSet<T,A>::Impl::no(
    A * alloc,
    Node * node,
    storage_type start,
    size_t len
) {
    return false;
}

//===========================================================================
template <std::integral T, typename A>
void IntegralSet<T,A>::Impl::skip(A * alloc, Node * node)
{}

//===========================================================================
template <std::integral T, typename A>
void IntegralSet<T,A>::Impl::skip(A * alloc, Node * left, const Node & right)
{}

//===========================================================================
template <std::integral T, typename A>
void IntegralSet<T,A>::Impl::skip(A * alloc, Node * left, Node && right)
{}

//===========================================================================
template <std::integral T, typename A>
void IntegralSet<T,A>::Impl::clear(A * alloc, Node * node) {
    destroy(alloc, node);
    node->type = kEmpty;
    init(alloc, node, false);
}

//===========================================================================
template <std::integral T, typename A>
void IntegralSet<T,A>::Impl::clear(
    A * alloc,
    Node * left,
    const Node & right
) {
    clear(alloc, left);
}

//===========================================================================
template <std::integral T, typename A>
void IntegralSet<T,A>::Impl::clear(A * alloc, Node * left, Node && right) {
    clear(alloc, left);
}

//===========================================================================
template <std::integral T, typename A>
void IntegralSet<T,A>::Impl::fill(A * alloc, Node * node) {
    destroy(alloc, node);
    node->type = kFull;
    init(alloc, node, true);
}

//===========================================================================
template <std::integral T, typename A>
void IntegralSet<T,A>::Impl::fill(
    A * alloc,
    Node * left,
    const Node & right
) {
    fill(alloc, left);
}

//===========================================================================
template <std::integral T, typename A>
void IntegralSet<T,A>::Impl::fill(A * alloc, Node * left, Node && right) {
    fill(alloc, left);
}

//===========================================================================
template <std::integral T, typename A>
void IntegralSet<T,A>::Impl::copy(
    A * alloc,
    Node * left,
    const Node & right
) {
    destroy(alloc, left);
    left->type = right.type;
    init(alloc, left, right);
}

//===========================================================================
template <std::integral T, typename A>
void IntegralSet<T,A>::Impl::copy(A * alloc, Node * left, Node && right) {
    swap(*left, right);
}

//===========================================================================
template <std::integral T, typename A>
size_t IntegralSet<T,A>::Impl::nodePos(const Node & node, unsigned value) {
    assert(relBase(value, node.depth) == node.base);
    auto pos = nodePos(value, node.depth);
    assert(pos < node.numValues);
    return pos;
}

//===========================================================================
template <std::integral T, typename A>
void IntegralSet<T,A>::Impl::grow(A * alloc, Node * node, size_t len) {
    Node tmp;
    memcpy(&tmp, node, sizeof tmp);
    if (len <= smvMaxValues()) {
        node->type = kSmVector;
    } else if (len <= vecMaxValues()) {
        node->type = kVector;
    } else if (tmp.depth == kMaxDepth) {
        node->type = kBitmap;
    } else {
        node->type = kMeta;
    }
    init(alloc, node, false);
    insert(alloc, node, move(tmp));
    destroy(alloc, &tmp);
}

//===========================================================================
template <std::integral T, typename A>
bool IntegralSet<T,A>::Impl::convertMetaIf(
    A * alloc,
    Node * node,
    NodeType type
) {
    assert(type == kEmpty || type == kFull);
    for (size_t i = 0; i < node->numValues; ++i) {
        if (node->nodes[i].type != type)
            return false;
    }
    // convert
    if (type == kEmpty) {
        clear(alloc, node);
    } else {
        fill(alloc, node);
    }
    return true;
}


/****************************************************************************
*
*   init(A * alloc, Node * node, bool full)
*
***/

//===========================================================================
template <std::integral T, typename A>
void IntegralSet<T,A>::Impl::init(A * alloc, Node * node, bool full) {
    using Fn = void(A * alloc, Node * node, bool full);
    constinit static Fn * const functs[kNodeTypes] = {
        initEmpty,
        initFull,
        initSmv,
        initVec,
        initBit,
        initMeta,
    };
    return functs[node->type](alloc, node, full);
}

//===========================================================================
template <std::integral T, typename A>
void IntegralSet<T,A>::Impl::initEmpty(A * alloc, Node * node, bool full) {
    assert(node->type == kEmpty);
    assert(!full);
    node->numBytes = 0;
    node->numValues = 0;
    node->values = nullptr;
}

//===========================================================================
template <std::integral T, typename A>
void IntegralSet<T,A>::Impl::initFull(A * alloc, Node * node, bool full) {
    assert(node->type == kFull);
    assert(full);
    node->numBytes = 0;
    node->numValues = 0;
    node->values = nullptr;
}

//===========================================================================
template <std::integral T, typename A>
void IntegralSet<T,A>::Impl::initSmv(A * alloc, Node * node, bool full) {
    assert(node->type == kSmVector);
    assert(!full);
    node->numBytes = 0;
    node->numValues = 0;
}

//===========================================================================
template <std::integral T, typename A>
void IntegralSet<T,A>::Impl::initVec(A * alloc, Node * node, bool full) {
    assert(node->type == kVector);
    assert(!full);
    node->numBytes = kDataSize;
    node->numValues = 0;
    node->values = (storage_type *) allocate(alloc, node->numBytes);
    assert(node->values);
}

//===========================================================================
template <std::integral T, typename A>
void IntegralSet<T,A>::Impl::initBit(A * alloc, Node * node, bool full) {
    assert(node->type == kBitmap);
    node->numBytes = kDataSize;
    node->values = (storage_type *) allocate(alloc, node->numBytes);
    assert(node->values);
    if (full) {
        node->numValues = kDataSize / sizeof uint64_t;
        memset(node->values, 0xff, kDataSize);
    } else {
        node->numValues = 0;
        memset(node->values, 0, kDataSize);
    }
}

//===========================================================================
template <std::integral T, typename A>
void IntegralSet<T,A>::Impl::initMeta(A * alloc, Node * node, bool full) {
    assert(node->type == kMeta);
    node->numValues = (uint16_t) numNodes(node->depth + 1);
    node->numBytes = (uint16_t) (node->numValues + 1) * sizeof *node->nodes;
    node->nodes = (Node *) allocate(alloc, node->numBytes);
    assert(node->nodes);
    Node def;
    def.depth = node->depth + 1;
    def.base = node->base;
    if (!full) {
        def.type = kEmpty;
        initEmpty(alloc, &def, full);
    } else {
        def.type = kFull;
        initFull(alloc, &def, full);
    }
    auto domain = relBase(absSize(def), def.depth);
    auto nptr = node->nodes;
    auto nlast = node->nodes + node->numValues;
    for (; nptr != nlast; ++nptr, def.base += domain)
        memcpy(nptr, &def, sizeof *nptr);

    // Internally the array of nodes contains a trailing "node" at the end that
    // is a pointer to the parent node->
    memset(nlast, 0, sizeof *nlast);
    nlast->type = kMetaParent;
    nlast->nodes = node;
}


/****************************************************************************
*
*   init(A * alloc, Node * node, const Node & from)
*
***/

//===========================================================================
template <std::integral T, typename A>
void IntegralSet<T,A>::Impl::init(
    A * alloc,
    Node * node,
    const Node & from
) {
    using Fn = void(A * alloc, Node * node, const Node & from);
    constinit static Fn * const functs[kNodeTypes] = {
        initEmpty,
        initFull,
        initSmv,
        initVec,
        initBit,
        initMeta,
    };
    return functs[node->type](alloc, node, from);
}

//===========================================================================
template <std::integral T, typename A>
void IntegralSet<T,A>::Impl::initEmpty(
    A * alloc,
    Node * node,
    const Node & from
) {
    assert(node->type == kEmpty);
    assert(from.type == kEmpty);
    memcpy(node, &from, sizeof *node);
}

//===========================================================================
template <std::integral T, typename A>
void IntegralSet<T,A>::Impl::initFull(
    A * alloc,
    Node * node,
    const Node & from
) {
    assert(node->type == kFull);
    assert(from.type == kFull);
    memcpy(node, &from, sizeof *node);
}

//===========================================================================
template <std::integral T, typename A>
void IntegralSet<T,A>::Impl::initSmv(
    A * alloc,
    Node * node,
    const Node & from
) {
    assert(node->type == kSmVector);
    assert(from.type == kSmVector);
    memcpy(node, &from, sizeof *node);
}

//===========================================================================
template <std::integral T, typename A>
void IntegralSet<T,A>::Impl::initVec(
    A * alloc,
    Node * node,
    const Node & from
) {
    assert(node->type == kVector);
    assert(from.type == kVector);
    memcpy(node, &from, sizeof *node);
    node->values = (storage_type *) allocate(alloc, node->numBytes);
    assert(node->values);
    memcpy(node->values, from.values, node->numValues * sizeof *node->values);
}

//===========================================================================
template <std::integral T, typename A>
void IntegralSet<T,A>::Impl::initBit(
    A * alloc,
    Node * node,
    const Node & from
) {
    assert(node->type == kBitmap);
    assert(from.type == kBitmap);
    memcpy(node, &from, sizeof *node);
    node->values = (storage_type *) allocate(alloc, node->numBytes);
    assert(node->values);
    memcpy(node->values, from.values, node->numBytes);
}

//===========================================================================
template <std::integral T, typename A>
void IntegralSet<T,A>::Impl::initMeta(
    A * alloc,
    Node * node,
    const Node & from
) {
    assert(node->type == kMeta);
    assert(from.type == kMeta);
    memcpy(node, &from, sizeof *node);
    node->nodes = (Node *) allocate(alloc, node->numBytes);
    assert(node->nodes);
    auto nptr = node->nodes;
    auto nlast = node->nodes + node->numValues;
    auto fptr = from.nodes;
    for (; nptr != nlast; ++nptr, ++fptr) {
        nptr->type = fptr->type;
        init(alloc, nptr, *fptr);
    }
    memcpy(nlast, fptr, sizeof *nlast);
    nlast->nodes = node;
}


/****************************************************************************
*
*   destroy(A * alloc, Node * node)
*
***/

//===========================================================================
template <std::integral T, typename A>
void IntegralSet<T,A>::Impl::destroy(A * alloc, Node * node) {
    using Fn = void(A * alloc, Node * node);
    constinit static Fn * const functs[kNodeTypes] = {
        skip,       // empty
        skip,       // full
        skip,       // small vector
        desFree,    // vector
        desFree,    // bitmap
        desMeta,    // meta
    };
    functs[node->type](alloc, node);
}

//===========================================================================
template <std::integral T, typename A>
void IntegralSet<T,A>::Impl::desFree(A * alloc, Node * node) {
    assert(node->values);
    deallocate(alloc, node->values, node->numBytes);
}

//===========================================================================
template <std::integral T, typename A>
void IntegralSet<T,A>::Impl::desMeta(A * alloc, Node * node) {
    assert(node->nodes);
    auto * ptr = node->nodes;
    auto * last = ptr + node->numValues;
    for (; ptr != last; ++ptr)
        destroy(alloc, ptr);
    deallocate(alloc, node->nodes, node->numBytes);
}


/****************************************************************************
*
*   insert(
*       A * alloc,
*       Node * node,
*       const storage_type * first,
*       const storage_type * last
*   )
*
***/

//===========================================================================
template <std::integral T, typename A>
bool IntegralSet<T,A>::Impl::insert(
    A * alloc,
    Node * node,
    const storage_type * first,
    const storage_type * last
) {
    using Fn = bool(
        A * alloc,
        Node * node,
        const storage_type * first,
        const storage_type * last
    );
    constinit static Fn * const functs[kNodeTypes] = {
        insEmpty,   // empty
        no,         // full
        insSmv,     // small vector
        insVec,     // vector
        insBit,     // bitmap
        insMeta,    // meta
    };
    return functs[node->type](alloc, node, first, last);
}

//===========================================================================
template <std::integral T, typename A>
bool IntegralSet<T,A>::Impl::insEmpty(
    A * alloc,
    Node * node,
    const storage_type * first,
    const storage_type * last
) {
    if (first == last)
        return false;

    destroy(alloc, node);
    node->type = kSmVector;
    init(alloc, node, false);
    insert(alloc, node, first, last);
    return true;
}

//===========================================================================
template <std::integral T, typename A>
bool IntegralSet<T,A>::Impl::insSmv(
    A * alloc,
    Node * node,
    const storage_type * first,
    const storage_type * last
) {
    bool changed = false;
    while (first != last) {
        if (node->type != kSmVector) {
            return insert(alloc, node, first, last)
                || changed;
        }
        changed = insert(alloc, node, *first++, 1) || changed;
    }
    return changed;
}

//===========================================================================
template <std::integral T, typename A>
bool IntegralSet<T,A>::Impl::insVec(
    A * alloc,
    Node * node,
    const storage_type * first,
    const storage_type * last
) {
    bool changed = false;
    while (first != last) {
        if (node->type != kVector) {
            return insert(alloc, node, first, last)
                || changed;
        }
        changed = insert(alloc, node, *first++, 1) || changed;
    }
    return changed;
}

//===========================================================================
template <std::integral T, typename A>
bool IntegralSet<T,A>::Impl::insBit(
    A * alloc,
    Node * node,
    const storage_type * first,
    const storage_type * last
) {
    bool changed = false;
    for (; first != last; ++first)
        changed = insert(alloc, node, *first, 1) || changed;
    return changed;
}

//===========================================================================
template <std::integral T, typename A>
bool IntegralSet<T,A>::Impl::insMeta(
    A * alloc,
    Node * node,
    const storage_type * first,
    const storage_type * last
) {
    bool changed = false;
    while (first != last) {
        auto final = absFinal(*first, node->depth + 1);
        auto ptr = node->nodes + nodePos(*node, *first);
        auto mid = first + 1;
        while (mid != last && *mid > *first && *mid <= final)
            mid += 1;
        changed = insert(alloc, ptr, first, mid) || changed;
        first = mid;
    }
    return changed;
}


/****************************************************************************
*
*   insert(A * alloc, Node * node, storage_type start, size_t len)
*
***/

//===========================================================================
template <std::integral T, typename A>
bool IntegralSet<T,A>::Impl::insert(
    A * alloc,
    Node * node,
    storage_type start,
    size_t len
) {
    using Fn = bool(A * alloc, Node * node, storage_type start, size_t len);
    constinit static Fn * const functs[kNodeTypes] = {
        insEmpty,   // empty
        no,         // full
        insSmv,     // small vector
        insVec,     // vector
        insBit,     // bitmap
        insMeta,    // meta
    };
    return functs[node->type](alloc, node, start, len);
}

//===========================================================================
template <std::integral T, typename A>
bool IntegralSet<T,A>::Impl::insEmpty(
    A * alloc,
    Node * node,
    storage_type start,
    size_t len
) {
    assert(node->type == kEmpty);
    assert(len);
    assert(relBase(start, node->depth) == node->base);
    assert(len - 1 <= absFinal(*node) - start);
    grow(alloc, node, len);
    insert(alloc, node, start, len);
    return true;
}

//===========================================================================
template <std::integral T, typename A>
bool IntegralSet<T,A>::Impl::insSmv(
    A * alloc,
    Node * node,
    storage_type start,
    size_t len
) {
    assert(len);
    assert(relBase(start, node->depth) == node->base);
    assert(len - 1 <= absFinal(*node) - start);
    auto last = node->localValues + node->numValues;
    auto ptr = lower_bound(node->localValues, last, start);
    auto eptr = (ptr < last && *ptr < start + len)
        ? lower_bound(ptr, last, start + len)
        : ptr;
    size_t nBelow = ptr - node->localValues;
    size_t nOld = eptr - ptr;
    size_t nAbove = last - eptr;
    if (len == nOld)
        return false;
    auto num = nBelow + len + nAbove;
    if (num > smvMaxValues()) {
        grow(alloc, node, num);
        return insert(alloc, node, start, len);
    }

    node->numValues = (uint16_t) num;
    if (nAbove)
        memmove(ptr + len, eptr, nAbove * sizeof *ptr);
    for (;;) {
        *ptr++ = start;
        if (!--len)
            return true;
        start += 1;
    }
}

//===========================================================================
template <std::integral T, typename A>
bool IntegralSet<T,A>::Impl::insVec(
    A * alloc,
    Node * node,
    storage_type start,
    size_t len
) {
    assert(len);
    assert(relBase(start, node->depth) == node->base);
    assert(len - 1 <= absFinal(*node) - start);
    auto last = node->values + node->numValues;
    auto ptr = lower_bound(node->values, last, start);
    auto eptr = (ptr < last && *ptr < start + len)
        ? lower_bound(ptr, last, start + len)
        : ptr;
    size_t nBelow = ptr - node->values;
    size_t nOld = eptr - ptr;
    size_t nAbove = last - eptr;
    if (len == nOld)
        return false;
    auto num = nBelow + len + nAbove;
    if (num > vecMaxValues()) {
        grow(alloc, node, num);
        return insert(alloc, node, start, len);
    }

    node->numValues = (uint16_t) num;
    if (nAbove)
        memmove(ptr + len, eptr, nAbove * sizeof *ptr);
    for (;;) {
        *ptr++ = start;
        if (!--len)
            return true;
        start += 1;
    }
}

//===========================================================================
template <std::integral T, typename A>
bool IntegralSet<T,A>::Impl::insBit(
    A * alloc,
    Node * node,
    storage_type start,
    size_t len
) {
    assert(node->type == kBitmap);
    assert(len);
    assert(relBase(start, node->depth) == node->base);
    assert(len - 1 <= absFinal(*node) - start);
    auto base = (uint64_t *) node->values;
    BitView bits(base, bitNumInt64s());

    auto low = relValue(start, node->depth);
    auto high = low + len - 1;
    auto lower = bits.data(low);
    auto upper = bits.data(high);
    if (lower == upper) {
        auto tmp = *lower;
        bits.set(low, len);
        if (tmp == *lower)
            return false;
        if (!tmp)
            node->numValues += 1;
        return true;
    }

    bits.remove_suffix(bitNumInt64s() - (upper - base + 1));
    auto zero = bits.findZero(low);
    if (zero > high)
        return false;

    lower = bits.data(zero);
    for (auto ptr = lower; ptr <= upper; ++ptr) {
        if (!*ptr)
            node->numValues += 1;
    }
    bits.set(zero, high - zero + 1);

    if (node->numValues < bitNumInt64s())
        return true;
    bits = BitView(base, bitNumInt64s());
    if (!bits.all())
        return true;

    destroy(alloc, node);
    node->type = kFull;
    init(alloc, node, true);
    return true;
}

//===========================================================================
template <std::integral T, typename A>
bool IntegralSet<T,A>::Impl::insMeta(
    A * alloc,
    Node * node,
    storage_type start,
    size_t len
) {
    assert(len);
    assert(relBase(start, node->depth) == node->base);
    assert(len - 1 <= absFinal(*node) - start);
    auto ptr = node->nodes + nodePos(*node, start);
    auto step = valueMask(node->depth + 1) + 1;
    auto maybeFull = true;
    auto changed = false;
    for (auto&& [st, cnt] : AlignedIntervalGen(start, len, step)) {
        if (cnt == step) {
            if (ptr->type != kFull) {
                changed = true;
                fill(alloc, ptr);
            }
        } else if (insert(alloc, ptr, (storage_type) st, cnt)) {
            changed = true;
            if (ptr->type != kFull)
                maybeFull = false;
        }
        ptr += 1;
    }
    if (changed && maybeFull)
        convertMetaIf(alloc, node, kFull);
    return changed;
}


/****************************************************************************
*
*   erase(A * alloc, Node * node, storage_type start, size_t len)
*
***/

//===========================================================================
template <std::integral T, typename A>
bool IntegralSet<T,A>::Impl::erase(
    A * alloc,
    Node * node,
    storage_type start,
    size_t len
) {
    using Fn = bool(A * alloc, Node * node, storage_type start, size_t len);
    constinit static Fn * const functs[kNodeTypes] = {
        no,         // empty
        eraFull,    // full
        eraSmv,     // small vector
        eraVec,     // vector
        eraBit,     // bitmap
        eraMeta,    // meta
    };
    return functs[node->type](alloc, node, start, len);
}

//===========================================================================
template <std::integral T, typename A>
bool IntegralSet<T,A>::Impl::eraFull(
    A * alloc,
    Node * node,
    storage_type start,
    size_t len
) {
    assert(len);
    assert(relBase(start, node->depth) == node->base);
    assert(len - 1 <= absFinal(*node) - start);
    destroy(alloc, node);
    if (node->depth == kMaxDepth) {
        node->type = kBitmap;
    } else {
        node->type = kMeta;
    }
    init(alloc, node, true);
    erase(alloc, node, start, len);
    return true;
}

//===========================================================================
template <std::integral T, typename A>
bool IntegralSet<T,A>::Impl::eraSmv(
    A * alloc,
    Node * node,
    storage_type start,
    size_t len
) {
    assert(len);
    assert(relBase(start, node->depth) == node->base);
    assert(len - 1 <= absFinal(*node) - start);
    auto last = node->localValues + node->numValues;
    auto ptr = lower_bound(node->localValues, last, start);
    if (ptr == last || *ptr >= start + len)
        return false;
    auto eptr = lower_bound(ptr, last, start + len);
    if (ptr == eptr)
        return false;

    node->numValues -= (uint16_t) (eptr - ptr);
    if (node->numValues) {
        // Still has values, shift remaining ones down
        memmove(ptr, eptr, sizeof *ptr * (last - eptr));
    } else {
        // No more values, convert to empty node.
        clear(alloc, node);
    }
    return true;
}

//===========================================================================
template <std::integral T, typename A>
bool IntegralSet<T,A>::Impl::eraVec(
    A * alloc,
    Node * node,
    storage_type start,
    size_t len
) {
    assert(len);
    assert(relBase(start, node->depth) == node->base);
    assert(len - 1 <= absFinal(*node) - start);
    auto last = node->values + node->numValues;
    auto ptr = lower_bound(node->values, last, start);
    if (ptr == last || *ptr >= start + len)
        return false;
    auto eptr = lower_bound(ptr, last, start + len);
    if (ptr == eptr)
        return false;

    node->numValues -= (uint16_t) (eptr - ptr);
    if (node->numValues) {
        // Still has values, shift remaining ones down
        memmove(ptr, eptr, sizeof *ptr * (last - eptr));
    } else {
        // No more values, convert to empty node->
        clear(alloc, node);
    }
    return true;
}

//===========================================================================
template <std::integral T, typename A>
bool IntegralSet<T,A>::Impl::eraBit(
    A * alloc,
    Node * node,
    storage_type start,
    size_t len
) {
    assert(len);
    assert(relBase(start, node->depth) == node->base);
    assert(len - 1 <= absFinal(*node) - start);
    auto base = (uint64_t *) node->values;
    BitView bits(base, bitNumInt64s());
    auto low = relValue(start, node->depth);
    auto high = low + len - 1;
    auto lower = bits.data(low);
    auto upper = bits.data(high);
    if (lower == upper) {
        auto tmp = *lower;
        bits.reset(low, len);
        if (tmp == *lower)
            return false;
        if (!*lower) {
            node->numValues -= 1;
            goto CHECK_IF_EMPTY;
        }
        return true;
    }

    bits = BitView(lower, upper - lower + 1);
    low %= bits.kWordBits;
    high = low + len - 1;
    low = (storage_type) bits.find(low);
    if (low > high)
        return false;

    len = high - low + 1;
    lower = bits.data(low);
    bits = BitView(lower, upper - lower + 1);
    low %= bits.kWordBits;
    for (auto ptr = lower; ptr <= upper; ++ptr) {
        if (*ptr)
            node->numValues -= 1;
    }
    bits.reset(low, len);
    for (auto ptr = lower; ptr <= upper; ++ptr) {
        if (*ptr)
            node->numValues += 1;
    }

CHECK_IF_EMPTY:
    if (!node->numValues)
        clear(alloc, node);

    return true;
}

//===========================================================================
template <std::integral T, typename A>
bool IntegralSet<T,A>::Impl::eraMeta(
    A * alloc,
    Node * node,
    storage_type start,
    size_t len
) {
    assert(len);
    assert(relBase(start, node->depth) == node->base);
    assert(len - 1 <= absFinal(*node) - start);
    auto ptr = node->nodes + nodePos(*node, start);
    auto step = valueMask(node->depth + 1) + 1;
    auto maybeEmpty = true;
    auto changed = false;
    for (auto&& [st, cnt] : AlignedIntervalGen(start, len, step)) {
        if (cnt == step) {
            if (ptr->type != kEmpty) {
                changed = true;
                clear(alloc, ptr);
            }
        } else if (erase(alloc, ptr, (storage_type) st, cnt)) {
            changed = true;
            if (ptr->type != kEmpty)
                maybeEmpty = false;
        }
        ptr += 1;
    }
    if (changed && maybeEmpty)
        convertMetaIf(alloc, node, kEmpty);
    return changed;
}


/****************************************************************************
*
*   count(const Node & node)
*
***/

//===========================================================================
template <std::integral T, typename A>
size_t IntegralSet<T,A>::Impl::count(const Node & node) {
    using Fn = size_t(const Node & node);
    constinit static Fn * const functs[kNodeTypes] = {
        cntEmpty,   // empty
        cntFull,    // full
        cntSmv,     // small vector
        cntVec,     // vector
        cntBit,     // bitmap
        cntMeta,    // meta
    };
    return functs[node.type](node);
}

//===========================================================================
template <std::integral T, typename A>
size_t IntegralSet<T,A>::Impl::cntEmpty(const Node & node) {
    return 0;
}

//===========================================================================
template <std::integral T, typename A>
size_t IntegralSet<T,A>::Impl::cntFull(const Node & node) {
    return absSize(node);
}

//===========================================================================
template <std::integral T, typename A>
size_t IntegralSet<T,A>::Impl::cntSmv(const Node & node) {
    return node.numValues;
}

//===========================================================================
template <std::integral T, typename A>
size_t IntegralSet<T,A>::Impl::cntVec(const Node & node) {
    return node.numValues;
}

//===========================================================================
template <std::integral T, typename A>
size_t IntegralSet<T,A>::Impl::cntBit(const Node & node) {
    auto base = (uint64_t *) node.values;
    BitView bits{base, bitNumInt64s()};
    return bits.count();
}

//===========================================================================
template <std::integral T, typename A>
size_t IntegralSet<T,A>::Impl::cntMeta(const Node & node) {
    size_t num = 0;
    auto * ptr = node.nodes;
    auto * last = ptr + node.numValues;
    for (; ptr != last; ++ptr)
        num += count(*ptr);
    return num;
}


/****************************************************************************
*
*   count(const Node & node, storage_type start, size_t len)
*
***/

//===========================================================================
template <std::integral T, typename A>
size_t IntegralSet<T,A>::Impl::count(
    const Node & node,
    storage_type start,
    size_t len
) {
    using Fn = size_t(const Node & node, storage_type start, size_t len);
    constinit static Fn * const functs[kNodeTypes] = {
        cntEmpty,   // empty
        cntFull,    // full
        cntSmv,     // small vector
        cntVec,     // vector
        cntBit,     // bitmap
        cntMeta,    // meta
    };
    return functs[node.type](node, start, len);
}

//===========================================================================
template <std::integral T, typename A>
size_t IntegralSet<T,A>::Impl::cntEmpty(
    const Node & node,
    storage_type start,
    size_t len
) {
    return 0;
}

//===========================================================================
template <std::integral T, typename A>
size_t IntegralSet<T,A>::Impl::cntFull(
    const Node & node,
    storage_type start,
    size_t len
) {
    return len;
}

//===========================================================================
template <std::integral T, typename A>
size_t IntegralSet<T,A>::Impl::cntSmv(
    const Node & node,
    storage_type start,
    size_t len
) {
    assert(len);
    assert(relBase(start, node.depth) == node.base);
    assert(len - 1 <= absFinal(node) - start);
    auto last = node.localValues + node.numValues;
    auto ptr = lower_bound(node.localValues, last, start);
    if (ptr == last || *ptr >= start + len)
        return 0;
    auto eptr = lower_bound(ptr, last, start + len);
    return eptr - ptr;
}

//===========================================================================
template <std::integral T, typename A>
size_t IntegralSet<T,A>::Impl::cntVec(
    const Node & node,
    storage_type start,
    size_t len
) {
    assert(len);
    assert(relBase(start, node.depth) == node.base);
    assert(len - 1 <= absFinal(node) - start);
    auto last = node.values + node.numValues;
    auto ptr = lower_bound(node.values, last, start);
    if (ptr == last || *ptr >= start + len)
        return 0;
    auto eptr = lower_bound(ptr, last, start + len);
    return eptr - ptr;
}

//===========================================================================
template <std::integral T, typename A>
size_t IntegralSet<T,A>::Impl::cntBit(
    const Node & node,
    storage_type start,
    size_t len
) {
    assert(len);
    assert(relBase(start, node.depth) == node.base);
    assert(len - 1 <= absFinal(node) - start);
    auto base = (uint64_t *) node.values;
    BitView bits{base, bitNumInt64s()};
    auto low = relValue(start, node.depth);
    return bits.count(low, len);
}

//===========================================================================
template <std::integral T, typename A>
size_t IntegralSet<T,A>::Impl::cntMeta(
    const Node & node,
    storage_type start,
    size_t len
) {
    assert(len);
    assert(relBase(start, node.depth) == node.base);
    assert(len - 1 <= absFinal(node) - start);
    auto pos = nodePos(node, start);
    auto finalPos = nodePos(node, (storage_type) (start + len - 1));
    auto lower = node.nodes + pos;
    auto upper = node.nodes + finalPos;
    if (pos == finalPos) {
        auto ptr = node.nodes + pos;
        return count(*ptr, start, len);
    }

    size_t num = 0;
    num += count(*lower, start, absFinal(*lower) - start + 1);
    for (auto ptr = lower + 1; ptr < upper; ++ptr) {
        num += count(*ptr);
    }
    num += count(*upper, absBase(*upper), start + len - 1);
    return num;
}


/****************************************************************************
*
*   find(
*       const Node ** onode,
*       storage_type * ovalue,
*       const Node & node,
*       storage_type key
*   )
*
*   Find smallest value >= pos, returns false if not found.
*
***/

//===========================================================================
template <std::integral T, typename A>
bool IntegralSet<T,A>::Impl::find(
    const Node ** onode,
    storage_type * ovalue,
    const Node & node,
    storage_type key
) {
    using Fn = bool(
        const Node ** onode,
        storage_type * ovalue,
        const Node & node,
        storage_type key
    );
    constinit static Fn * const functs[kNodeTypes] = {
        findEmpty,   // empty
        findFull,    // full
        findSmv,     // small vector
        findVec,     // vector
        findBit,     // bitmap
        findMeta,    // meta
    };
    return functs[node.type](onode, ovalue, node, key);
}

//===========================================================================
template <std::integral T, typename A>
bool IntegralSet<T,A>::Impl::findEmpty(
    const Node ** onode,
    storage_type * ovalue,
    const Node & node,
    storage_type key
) {
    assert(relBase(key, node.depth) == node.base);
    return false;
}

//===========================================================================
template <std::integral T, typename A>
bool IntegralSet<T,A>::Impl::findFull(
    const Node ** onode,
    storage_type * ovalue,
    const Node & node,
    storage_type key
) {
    assert(relBase(key, node.depth) == node.base);
    *onode = &node;
    *ovalue = key;
    return true;
}

//===========================================================================
template <std::integral T, typename A>
bool IntegralSet<T,A>::Impl::findSmv(
    const Node ** onode,
    storage_type * ovalue,
    const Node & node,
    storage_type key
) {
    assert(relBase(key, node.depth) == node.base);
    auto last = node.localValues + node.numValues;
    auto ptr = lower_bound(node.localValues, last, key);
    if (ptr != last) {
        *onode = &node;
        *ovalue = *ptr;
        return true;
    }
    return false;
}

//===========================================================================
template <std::integral T, typename A>
bool IntegralSet<T,A>::Impl::findVec(
    const Node ** onode,
    storage_type * ovalue,
    const Node & node,
    storage_type key
) {
    assert(relBase(key, node.depth) == node.base);
    auto last = node.values + node.numValues;
    auto ptr = lower_bound(node.values, last, key);
    if (ptr != last) {
        *onode = &node;
        *ovalue = *ptr;
        return true;
    }
    return false;
}

//===========================================================================
template <std::integral T, typename A>
bool IntegralSet<T,A>::Impl::findBit(
    const Node ** onode,
    storage_type * ovalue,
    const Node & node,
    storage_type key
) {
    assert(relBase(key, node.depth) == node.base);
    auto base = (uint64_t *) node.values;
    auto rel = relValue(key, node.depth);
    assert(rel < bitNumBits());
    BitView bits{base, bitNumInt64s()};
    if (auto pos = bits.find(rel); pos != bits.npos) {
        *onode = &node;
        *ovalue = (storage_type) pos + absBase(node);
        return true;
    }
    return false;
}

//===========================================================================
template <std::integral T, typename A>
bool IntegralSet<T,A>::Impl::findMeta(
    const Node ** onode,
    storage_type * ovalue,
    const Node & node,
    storage_type key
) {
    assert(Impl::relBase(key, node.depth) == node.base);
    auto pos = Impl::nodePos(node, key);
    auto ptr = node.nodes + pos;
    auto last = node.nodes + node.numValues;
    for (;;) {
        if (Impl::find(onode, ovalue, *ptr, key))
            return true;
        if (++ptr == last)
            return false;
        key = Impl::absBase(*ptr);
    }
}


/****************************************************************************
*
*   rfind(
*       const Node ** onode,
*       storage_type * ovalue,
*       const Node & node,
*       storage_type key
*   )
*
*   Find largest value <= pos, returns false if there are none.
*
***/

//===========================================================================
template <std::integral T, typename A>
bool IntegralSet<T,A>::Impl::rfind(
    const Node ** onode,
    storage_type * ovalue,
    const Node & node,
    storage_type key
) {
    using Fn = bool(
        const Node ** onode,
        storage_type * ovalue,
        const Node & node,
        storage_type key
    );
    constinit static Fn * const functs[kNodeTypes] = {
        findEmpty,    // empty
        findFull,     // full
        rfindSmv,     // small vector
        rfindVec,     // vector
        rfindBit,     // bitmap
        rfindMeta,    // meta
    };
    return functs[node.type](onode, ovalue, node, key);
}

//===========================================================================
template <std::integral T, typename A>
bool IntegralSet<T,A>::Impl::rfindSmv(
    const Node ** onode,
    storage_type * ovalue,
    const Node & node,
    storage_type key
) {
    assert(relBase(key, node.depth) == node.base);
    if (key < *node.localValues) {
        return false;
    } else if (key == *node.localValues) {
        *ovalue = key;
    } else {
        // base > *node.localValues
        auto last = node.localValues + node.numValues;
        auto ptr = lower_bound(node.localValues + 1, last, key);
        *ovalue = ptr[-1];
    }
    *onode = &node;
    return true;
}

//===========================================================================
template <std::integral T, typename A>
bool IntegralSet<T,A>::Impl::rfindVec(
    const Node ** onode,
    storage_type * ovalue,
    const Node & node,
    storage_type key
) {
    assert(relBase(key, node.depth) == node.base);
    if (key < *node.values) {
        return false;
    } else if (key == *node.values) {
        *ovalue = key;
    } else {
        // base > *node.values
        auto last = node.values + node.numValues;
        auto ptr = lower_bound(node.values + 1, last, key);
        *ovalue = ptr[-1];
    }
    *onode = &node;
    return true;
}

//===========================================================================
template <std::integral T, typename A>
bool IntegralSet<T,A>::Impl::rfindBit(
    const Node ** onode,
    storage_type * ovalue,
    const Node & node,
    storage_type key
) {
    assert(relBase(key, node.depth) == node.base);
    auto base = (uint64_t *) node.values;
    auto rel = relValue(key, node.depth);
    assert(rel < bitNumBits());
    BitView bits{base, bitNumInt64s()};
    if (auto pos = bits.rfind(rel); pos != bits.npos) {
        *onode = &node;
        *ovalue = (storage_type) pos + absBase(node);
        return true;
    }
    return false;
}

//===========================================================================
template <std::integral T, typename A>
bool IntegralSet<T,A>::Impl::rfindMeta(
    const Node ** onode,
    storage_type * ovalue,
    const Node & node,
    storage_type key
) {
    assert(relBase(key, node.depth) == node.base);
    auto pos = nodePos(node, key);
    auto ptr = node.nodes + pos;
    for (;;) {
        if (rfind(onode, ovalue, *ptr, key))
            return true;
        if (ptr == node.nodes)
            return false;
        ptr -= 1;
        key = absFinal(*ptr);
    }
}


/****************************************************************************
*
*   contig(
*       const Node ** onode,
*       storage_type * ovalue,
*       const Node & node,
*       storage_type key
*   )
*
*   Find largest value in segment contiguously connected with pos. Returns
*   false if may extend into the next node.
*
***/

//===========================================================================
template <std::integral T, typename A>
bool IntegralSet<T,A>::Impl::contig(
    const Node ** onode,
    storage_type * ovalue,
    const Node & node,
    storage_type key
) {
    using Fn = bool(
        const Node ** onode,
        storage_type * ovalue,
        const Node & node,
        storage_type key
    );
    constinit static Fn * const functs[kNodeTypes] = {
        yes,            // empty
        contigFull,     // full
        contigSmv,      // small vector
        contigVec,      // vector
        contigBit,      // bitmap
        contigMeta,     // meta
    };
    return functs[node.type](onode, ovalue, node, key);
}

//===========================================================================
template <std::integral T, typename A>
bool IntegralSet<T,A>::Impl::contigFull(
    const Node ** onode,
    storage_type * ovalue,
    const Node & node,
    storage_type key
) {
    assert(relBase(key, node.depth) == node.base);
    *onode = &node;
    *ovalue = absFinal(node);
    return false;
}

//===========================================================================
template <std::integral T, typename A>
bool IntegralSet<T,A>::Impl::contigSmv(
    const Node ** onode,
    storage_type * ovalue,
    const Node & node,
    storage_type key
) {
    assert(relBase(key, node.depth) == node.base);
    auto last = node.localValues + node.numValues;
    auto ptr = lower_bound(node.localValues, last, key);
    auto val = key;
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
template <std::integral T, typename A>
bool IntegralSet<T,A>::Impl::contigVec(
    const Node ** onode,
    storage_type * ovalue,
    const Node & node,
    storage_type key
) {
    assert(relBase(key, node.depth) == node.base);
    auto last = node.values + node.numValues;
    auto ptr = lower_bound(node.values, last, key);
    auto val = key;
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
template <std::integral T, typename A>
bool IntegralSet<T,A>::Impl::contigBit(
    const Node ** onode,
    storage_type * ovalue,
    const Node & node,
    storage_type key
) {
    assert(relBase(key, node.depth) == node.base);
    auto base = (uint64_t *) node.values;
    auto rel = relValue(key, node.depth);
    assert(rel < bitNumBits());
    BitView bits{base, bitNumInt64s()};
    if (auto pos = bits.findZero(rel); pos != bits.npos) {
        if (auto val = (storage_type) pos + absBase(node); val != key) {
            *onode = &node;
            *ovalue = val - 1;
        }
        return true;
    }
    *onode = &node;
    *ovalue = absFinal(node);
    return false;
}

//===========================================================================
template <std::integral T, typename A>
bool IntegralSet<T,A>::Impl::contigMeta(
    const Node ** onode,
    storage_type * ovalue,
    const Node & node,
    storage_type key
) {
    auto pos = nodePos(node, key);
    auto ptr = (const Node *) node.nodes + pos;
    auto last = node.nodes + node.numValues;
    for (;;) {
        if (contig(onode, ovalue, *ptr, key))
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
*   rcontig(
*       const Node ** onode,
*       storage_type * ovalue,
*       const Node & node,
*       storage_type key
*   )
*
*   Find smallest value in segment contiguously connected with pos. Returns
*   false if may extend into preceding node.
*
***/

//===========================================================================
template <std::integral T, typename A>
bool IntegralSet<T,A>::Impl::rcontig(
    const Node ** onode,
    storage_type * ovalue,
    const Node & node,
    storage_type key
) {
    using Fn = bool(
        const Node ** onode,
        storage_type * ovalue,
        const Node & node,
        storage_type key
    );
    constinit static Fn * const functs[kNodeTypes] = {
        yes,            // empty
        rcontigFull,     // full
        rcontigSmv,     // small vector
        rcontigVec,     // vector
        rcontigBit,     // bitmap
        rcontigMeta,    // meta
    };
    return functs[node.type](onode, ovalue, node, key);
}

//===========================================================================
template <std::integral T, typename A>
bool IntegralSet<T,A>::Impl::rcontigFull(
    const Node ** onode,
    storage_type * ovalue,
    const Node & node,
    storage_type key
) {
    assert(relBase(key, node.depth) == node.base);
    *onode = &node;
    *ovalue = absBase(node);
    return false;
}

//===========================================================================
template <std::integral T, typename A>
bool IntegralSet<T,A>::Impl::rcontigSmv(
    const Node ** onode,
    storage_type * ovalue,
    const Node & node,
    storage_type key
) {
    assert(relBase(key, node.depth) == node.base);
    auto last = node.localValues + node.numValues;
    auto ptr = lower_bound(node.localValues, last, key);
    auto val = key;
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
template <std::integral T, typename A>
bool IntegralSet<T,A>::Impl::rcontigVec(
    const Node ** onode,
    storage_type * ovalue,
    const Node & node,
    storage_type key
) {
    assert(relBase(key, node.depth) == node.base);
    auto last = node.values + node.numValues;
    auto ptr = lower_bound(node.values, last, key);
    auto val = key;
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
template <std::integral T, typename A>
bool IntegralSet<T,A>::Impl::rcontigBit(
    const Node ** onode,
    storage_type * ovalue,
    const Node & node,
    storage_type key
) {
    assert(relBase(key, node.depth) == node.base);
    auto base = (uint64_t *) node.values;
    auto rel = relValue(key, node.depth);
    assert(rel < bitNumBits());
    BitView bits{base, bitNumInt64s()};
    if (auto pos = bits.rfindZero(rel); pos != bits.npos) {
        if (auto val = (storage_type) pos + absBase(node); val != key) {
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
template <std::integral T, typename A>
bool IntegralSet<T,A>::Impl::rcontigMeta(
    const Node ** onode,
    storage_type * ovalue,
    const Node & node,
    storage_type key
) {
    auto pos = nodePos(node, key);
    auto ptr = (const Node *) node.nodes + pos;
    auto last = node.nodes + node.numValues;
    for (;;) {
        if (rcontig(onode, ovalue, *ptr, key))
            return true;
        *onode = ptr;
        *ovalue = absBase(*ptr);
        if (++ptr == last)
            return false;
        key = absFinal(*ptr);
    }
}


/****************************************************************************
*
*   Compare
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

//===========================================================================
template <std::integral T, typename A>
int IntegralSet<T,A>::Impl::compare(const Node & left, const Node & right) {
    using Fn = int(const Node & left, const Node & right);
    static Fn * functs[][kNodeTypes] = {
// LEFT                         RIGHT
//         empty      full        sm vec     vector     bitmap     meta
/*empty*/{ cmpEqual,  cmpLessIf,  cmpLessIf, cmpLessIf, cmpLessIf, cmpLessIf },
/*full */{ cmpMoreIf, cmpEqual,   cmpSmvIf,  cmpVecIf,  cmpBitIf,  cmpMetaIf },
/*smv  */{ cmpMoreIf, cmpRSmvIf,  cmpSmv,    cmpLSmv,   cmpIter,   cmpIter   },
/*vec  */{ cmpMoreIf, cmpRVecIf,  cmpRSmv,   cmpVec,    cmpIter,   cmpIter   },
/*bit  */{ cmpMoreIf, cmpRBitIf,  cmpIter,   cmpIter,   cmpBit,    cmpError  },
/*meta */{ cmpMoreIf, cmpRMetaIf, cmpIter,   cmpIter,   cmpError,  cmpMeta   },
    };
    return functs[left.type][right.type](left, right);
}

//===========================================================================
template <std::integral T, typename A>
int IntegralSet<T,A>::Impl::cmpError(const Node & left, const Node & right) {
    logMsgFatal() << "compare: incompatible node types, " << left.type
        << ", " << right.type;
    return 0;
}

//===========================================================================
template <std::integral T, typename A>
int IntegralSet<T,A>::Impl::cmpArray(
    const storage_type * li,
    size_t lcount,
    const storage_type * ri,
    size_t rcount
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
template <std::integral T, typename A>
int IntegralSet<T,A>::Impl::cmpLessIf(const Node & left, const Node & right) {
    return -2;
}

//===========================================================================
template <std::integral T, typename A>
int IntegralSet<T,A>::Impl::cmpMoreIf(const Node & left, const Node & right) {
    return 2;
}

//===========================================================================
template <std::integral T, typename A>
int IntegralSet<T,A>::Impl::cmpEqual(const Node & left, const Node & right) {
    return 0;
}

//===========================================================================
template <std::integral T, typename A>
int IntegralSet<T,A>::Impl::cmpVecIf(const Node & left, const Node & right) {
    auto minMax = absBase(right) + right.numValues - 1;
    if (minMax == right.values[right.numValues - 1]) {
        return 2;
    } else {
        return -1;
    }
}

//===========================================================================
template <std::integral T, typename A>
int IntegralSet<T,A>::Impl::cmpRVecIf(const Node & left, const Node & right) {
    return -cmpVecIf(right, left);
}

//===========================================================================
template <std::integral T, typename A>
int IntegralSet<T,A>::Impl::cmpSmvIf(const Node & left, const Node & right) {
    auto minMax = absBase(right) + right.numValues - 1;
    if (minMax == right.localValues[right.numValues - 1]) {
        return 2;
    } else {
        return -1;
    }
}

//===========================================================================
template <std::integral T, typename A>
int IntegralSet<T,A>::Impl::cmpRSmvIf(const Node & left, const Node & right) {
    return -cmpSmvIf(right, left);
}

//===========================================================================
template <std::integral T, typename A>
int IntegralSet<T,A>::Impl::cmpBitIf(const Node & left, const Node & right) {
    auto base = (uint64_t *) right.values;
    BitView bits{base, bitNumInt64s()};
    auto i = bits.findZero();
    if (i && bits.find(i) == BitView::npos) {
        return 2;
    } else {
        return -1;
    }
}

//===========================================================================
template <std::integral T, typename A>
int IntegralSet<T,A>::Impl::cmpMetaIf(const Node & left, const Node & right) {
    auto i = IntegralSet<T,A>::iterator::makeFirst(&right);
    auto ri = typename IntegralSet<T,A>::range_iterator{i};
    if (toStorage(ri->first) == absBase(right) && !++ri) {
        return 2;
    } else {
        return -1;
    }
}

//===========================================================================
template <std::integral T, typename A>
int IntegralSet<T,A>::Impl::cmpRBitIf(const Node & left, const Node & right) {
    return -cmpBitIf(right, left);
}

//===========================================================================
template <std::integral T, typename A>
int IntegralSet<T,A>::Impl::cmpRMetaIf(const Node & left, const Node & right) {
    return -cmpMetaIf(right, left);
}

//===========================================================================
template <std::integral T, typename A>
int IntegralSet<T,A>::Impl::cmpIter(const Node & left, const Node & right) {
    auto li = IntegralSet<T,A>::iterator::makeFirst(&left);
    auto ri = IntegralSet<T,A>::iterator::makeFirst(&right);
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
template <std::integral T, typename A>
int IntegralSet<T,A>::Impl::cmpVec(const Node & left, const Node & right) {
    return cmpArray(
        left.values,
        left.numValues,
        right.values,
        right.numValues
    );
}

//===========================================================================
// small vector <=> small vector
template <std::integral T, typename A>
int IntegralSet<T,A>::Impl::cmpSmv(const Node & left, const Node & right) {
    return cmpArray(
        left.localValues,
        left.numValues,
        right.localValues,
        right.numValues
    );
}

//===========================================================================
// small vector <=> vector
template <std::integral T, typename A>
int IntegralSet<T,A>::Impl::cmpLSmv(const Node & left, const Node & right) {
    return cmpArray(
        left.localValues,
        left.numValues,
        right.values,
        right.numValues
    );
}

//===========================================================================
// vector <=> small vector
template <std::integral T, typename A>
int IntegralSet<T,A>::Impl::cmpRSmv(const Node & left, const Node & right) {
    return cmpArray(
        left.values,
        left.numValues,
        right.localValues,
        right.numValues
    );
}

//===========================================================================
template <std::integral T, typename A>
int IntegralSet<T,A>::Impl::cmpBit(uint64_t left, uint64_t right) {
    if (left == right)
        return 0;   // equal
    constexpr uint64_t kMask = numeric_limits<uint64_t>::max();
    if (left < right) {
        if (left != (right & (kMask << countr_zero(left)))) {
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
        if (right != (left & (kMask << countr_zero(right)))) {
            return -1;
        } else {
            return 2;
        }
    }
}

//===========================================================================
template <std::integral T, typename A>
int IntegralSet<T,A>::Impl::cmpBit(const Node & left, const Node & right) {
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
template <std::integral T, typename A>
int IntegralSet<T,A>::Impl::cmpMeta(const Node & left, const Node & right) {
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
}


/****************************************************************************
*
*   contains(const Node &, const Node &)
*
***/

//===========================================================================
template <std::integral T, typename A>
bool IntegralSet<T,A>::Impl::contains(const Node & left, const Node & right) {
    using Fn = bool(const Node & left, const Node & right);
    static Fn * const functs[][kNodeTypes] = {
// LEFT                       RIGHT
//         empty  full  sm vec   vector   bitmap    meta
/*empty*/{ yes,   no,   no,      no,      no,       no    },
/*full */{ yes,   yes,  yes,     yes,     yes,      yes   },
/*smv  */{ yes,   no,   conSmv,  conLSmv, conLSmv,  conLSmv  },
/*vec  */{ yes,   no,   conRSmv, conVec,  conLVec,  conLVec  },
/*bit  */{ yes,   no,   conRSmv, conRVec, conBit,   conError },
/*meta */{ yes,   no,   conRSmv, conRVec, conError, conMeta  },
    };
    assert(!"not tested, contains(const Node&, const Node&)");
    return functs[left.type][right.type](left, right);
}

//===========================================================================
template <std::integral T, typename A>
bool IntegralSet<T,A>::Impl::conError(const Node & left, const Node & right) {
    logMsgFatal() << "contains: incompatible node types, " << left.type
        << ", " << right.type;
    return false;
}

//===========================================================================
template <std::integral T, typename A>
bool IntegralSet<T,A>::Impl::conRArray(
    const Node & left,
    const Node & right,
    const storage_type * ri
) {
    auto re = ri + right.numValues;
    const Node * onode;
    storage_type ovalue;

    for (;;) {
        if (!find(&onode, &ovalue, left, *ri))
            return false;
        if (*ri != ovalue || ++ri == re)
            return true;
    }
}

//===========================================================================
template <std::integral T, typename A>
bool IntegralSet<T,A>::Impl::conLArray(
    const Node & left,
    const storage_type * li,
    const Node & right
) {
    auto le = li + left.numValues;
    const Node * onode;
    storage_type ovalue;

    find(&onode, &ovalue, right, absBase(right));
    for (;;) {
        li = lower_bound(li, le, ovalue);
        if (li == le || *li != ovalue)
            return false;
        if (!find(&onode, &ovalue, right, ovalue + 1))
            return true;
    }
}

//===========================================================================
template <std::integral T, typename A>
bool IntegralSet<T,A>::Impl::conArray(
    const storage_type * li,
    size_t lcount,
    const storage_type * ri,
    size_t rcount
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
template <std::integral T, typename A>
bool IntegralSet<T,A>::Impl::conRVec(const Node & left, const Node & right) {
    return conRArray(left, right, right.values);
}

//===========================================================================
template <std::integral T, typename A>
bool IntegralSet<T,A>::Impl::conLVec(const Node & left, const Node & right) {
    return conLArray(left, left.values, right);
}

//===========================================================================
template <std::integral T, typename A>
bool IntegralSet<T,A>::Impl::conVec(const Node & left, const Node & right) {
    return conArray(
        left.values,
        left.numValues,
        right.values,
        right.numValues
    );
}

//===========================================================================
template <std::integral T, typename A>
bool IntegralSet<T,A>::Impl::conRSmv(const Node& left, const Node& right) {
    return conRArray(left, right, right.localValues);
}

//===========================================================================
template <std::integral T, typename A>
bool IntegralSet<T,A>::Impl::conLSmv(const Node& left, const Node& right) {
    return conLArray(left, left.localValues, right);
}

//===========================================================================
template <std::integral T, typename A>
bool IntegralSet<T,A>::Impl::conSmv(const Node& left, const Node& right) {
    return conArray(
        left.localValues,
        left.numValues,
        right.localValues,
        right.numValues
    );
}

//===========================================================================
template <std::integral T, typename A>
bool IntegralSet<T,A>::Impl::conBit(const Node & left, const Node & right) {
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
template <std::integral T, typename A>
bool IntegralSet<T,A>::Impl::conMeta(const Node & left, const Node & right) {
    auto li = left.nodes;
    auto le = li + left.numValues;
    auto ri = right.nodes;
    for (; li != le; ++li, ++ri) {
        if (!contains(*li, *ri))
            return false;
    }
    return true;
}


/****************************************************************************
*
*   intersects(const Node &, const Node &)
*
***/

//===========================================================================
template <std::integral T, typename A>
bool IntegralSet<T,A>::Impl::intersects(
    const Node & left,
    const Node & right
) {
    using Fn = bool(const Node & left, const Node & right);
    static Fn * const functs[][kNodeTypes] = {
// LEFT                         RIGHT
//         empty  full  sm vec    vector    bitmap     meta
/*empty*/{ no,    no,   no,       no,       no,        no        },
/*full */{ no,    yes,  yes,      yes,      yes,       yes       },
/*smv  */{ no,    yes,  isecSmv,  isecLSmv, isecLSmv,  isecLSmv  },
/*vec  */{ no,    yes,  isecRSmv, isecVec,  isecLVec,  isecLVec  },
/*bit  */{ no,    yes,  isecRSmv, isecRVec, isecBit,   isecError },
/*meta */{ no,    yes,  isecRSmv, isecRVec, isecError, isecMeta  },
    };
    assert(!"not tested, intersects(const Node&, const Node&)");
    return functs[left.type][right.type](left, right);
}

//===========================================================================
template <std::integral T, typename A>
bool IntegralSet<T,A>::Impl::isecError(const Node & left, const Node & right) {
    logMsgFatal() << "intersects: incompatible node types, " << left.type
        << ", " << right.type;
    return false;
}

//===========================================================================
template <std::integral T, typename A>
bool IntegralSet<T,A>::Impl::isecRArray(
    const Node & left,
    const Node & right,
    const storage_type * ri
) {
    auto re = ri + right.numValues;
    const Node * onode;
    storage_type ovalue;

    for (;;) {
        if (find(&onode, &ovalue, left, *ri)
            && *ri == ovalue
        ) {
            return true;
        }
        if (++ri == re)
            return false;
    }
}

//===========================================================================
template <std::integral T, typename A>
bool IntegralSet<T,A>::Impl::isecArray(
    const storage_type * li,
    size_t lcount,
    const storage_type * ri,
    size_t rcount
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
template <std::integral T, typename A>
bool IntegralSet<T,A>::Impl::isecRVec(const Node & left, const Node & right) {
    return isecRArray(left, right, right.values);
}

//===========================================================================
template <std::integral T, typename A>
bool IntegralSet<T,A>::Impl::isecLVec(const Node & left, const Node & right) {
    return isecRArray(right, left, left.values);
}

//===========================================================================
template <std::integral T, typename A>
bool IntegralSet<T,A>::Impl::isecVec(const Node & left, const Node & right) {
    return isecArray(
        left.values,
        left.numValues,
        right.values,
        right.numValues
    );
}

//===========================================================================
template <std::integral T, typename A>
bool IntegralSet<T,A>::Impl::isecRSmv(const Node & left, const Node & right) {
    return isecRArray(left, right, right.localValues);
}

//===========================================================================
template <std::integral T, typename A>
bool IntegralSet<T,A>::Impl::isecLSmv(const Node & left, const Node & right) {
    return isecRArray(right, left, left.localValues);
}

//===========================================================================
template <std::integral T, typename A>
bool IntegralSet<T,A>::Impl::isecSmv(const Node & left, const Node & right) {
    return isecArray(
        left.localValues,
        left.numValues,
        right.localValues,
        right.numValues
    );
}

//===========================================================================
template <std::integral T, typename A>
bool IntegralSet<T,A>::Impl::isecBit(const Node & left, const Node & right) {
    auto li = (uint64_t *) left.values;
    auto le = li + bitNumInt64s();
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
template <std::integral T, typename A>
bool IntegralSet<T,A>::Impl::isecMeta(const Node & left, const Node & right) {
    auto li = left.nodes;
    auto le = li + left.numValues;
    auto ri = right.nodes;
    for (; li != le; ++li, ++ri) {
        if (intersects(*li, *ri))
            return true;
    }
    return false;
}


/****************************************************************************
*
*   insert(A *, Node &, const Node &)
*
***/

//===========================================================================
template <std::integral T, typename A>
void IntegralSet<T,A>::Impl::insert(
    A * alloc,
    Node * left,
    const Node & right
) {
    using Fn = void(A * alloc, Node * left, const Node & right);
    static Fn * const functs[][kNodeTypes] = {
// LEFT                         RIGHT
//         empty  full  sm vec   vector   bitmap    meta
/*empty*/{ skip,  fill, copy,    copy,    copy,     copy     },
/*full */{ skip,  skip, skip,    skip,    skip,     skip     },
/*smv  */{ skip,  fill, insRSmv, insLSmv, insLSmv,  insLSmv  },
/*vec  */{ skip,  fill, insRSmv, insRVec, insLVec,  insLVec  },
/*bit  */{ skip,  fill, insRSmv, insRVec, insBit,   insError },
/*meta */{ skip,  fill, insRSmv, insRVec, insError, insMeta  },
    };
    functs[left->type][right.type](alloc, left, right);
}

//===========================================================================
template <std::integral T, typename A>
void IntegralSet<T,A>::Impl::insError(
    A * alloc,
    Node * left,
    const Node & right
) {
    logMsgFatal() << "insert: incompatible node types, " << left->type
        << ", " << right.type;
}

//===========================================================================
// FIXME: insArray is buggy, fix and update insert() to use it via insVec.
template <std::integral T, typename A>
void IntegralSet<T,A>::Impl::insArray(
    A * alloc,
    Node * left,
    storage_type * li,
    size_t maxValues,
    const storage_type * ri,
    const storage_type * re
) {
    assert(left->type == kVector);
    assert(left->numValues);
    assert(ri < re);
    auto rbase = ri;
    auto rcount = (uint16_t) min<size_t>(
        re - ri,
        maxValues - left->numValues
    );
    if (rcount) {
        // Copy from
        // rbase       ri
        //   a    b    c    d    e
        //        |<------------ re
        //
        // Copy to
        //       li
        //  -    A    B    -    -    -
        //  |<------- le
        //       |<----------------- out
        auto lfinal = li + left->numValues - 1;
        auto le = lfinal;
        auto out = le + rcount;
        ri = re - rcount;
        re -= 1;
        for (;;) {
            auto cmp = *le <=> *re;
            if (cmp > 0) {
                *out-- = *le--;
                if (le < li) {
                    // Remaining right values.
                    auto ucnt = re - ri + 1;
                    memcpy(li, ri, ucnt * sizeof *ri);
                    // Move down already merged values.
                    auto cnt = lfinal + rcount - out;
                    if (li + ucnt != out + 1)
                        memmove(li + ucnt, out + 1, cnt * sizeof *ri);
                    // Set to new len.
                    left->numValues = (uint16_t) (ucnt + cnt);
                    break;
                }
            } else {
                *out-- = *re--;
                if (cmp == 0) {
                    le -= 1;
                } else if (le == out) {
                    // Reached the end after copying all values; update len.
                    left->numValues += rcount;
                    break;
                }
                if (re < ri) {
                    // Move down already merged values.
                    auto cnt = (li + left->numValues - 1) + rcount - out;
                    memmove(le + 1, out + 1, cnt * sizeof *ri);
                    // Set to new len.
                    left->numValues = (uint16_t) ((le + 1 - li) + cnt);
                    break;
                }
            }
        }
    }
    if (rbase != ri)
        insert(alloc, left, rbase, ri);
}

//===========================================================================
template <std::integral T, typename A>
void IntegralSet<T,A>::Impl::insRSmv(
    A * alloc,
    Node * left,
    const Node & right
) {
    assert(right.type == kSmVector);
    auto ri = right.localValues;
    auto re = ri + right.numValues;
    insert(alloc, left, ri, re);
}

//===========================================================================
template <std::integral T, typename A>
void IntegralSet<T,A>::Impl::insLSmv(
    A * alloc,
    Node * left,
    const Node & right
) {
    assert(left->type == kSmVector);
    Node tmp;
    copy(alloc, &tmp, right);
    insRSmv(alloc, &tmp, move(*left));
    swap(*left, tmp);
    destroy(alloc, &tmp);
}

//===========================================================================
template <std::integral T, typename A>
void IntegralSet<T,A>::Impl::insVec(
    A * alloc,
    Node * left,
    const Node & right
) {
    assert(left->type == kVector);
    assert(right.type == kVector);
    auto ri = right.values;
    auto re = ri + right.numValues;
    insArray(alloc, left, left->values, vecMaxValues(), ri, re);
}

//===========================================================================
template <std::integral T, typename A>
void IntegralSet<T,A>::Impl::insRVec(
    A * alloc,
    Node * left,
    const Node & right
) {
    assert(right.type == kVector);
    auto ri = right.values;
    auto re = ri + right.numValues;
    insert(alloc, left, ri, re);
}

//===========================================================================
template <std::integral T, typename A>
void IntegralSet<T,A>::Impl::insLVec(
    A * alloc,
    Node * left,
    const Node & right
) {
    assert(left->type == kVector);
    Node tmp;
    copy(alloc, &tmp, right);
    insRVec(alloc, &tmp, move(*left));
    swap(*left, tmp);
    destroy(alloc, &tmp);
}

//===========================================================================
template <std::integral T, typename A>
void IntegralSet<T,A>::Impl::insBit(
    A * alloc,
    Node * left,
    const Node & right
) {
    auto li = (uint64_t *) left->values;
    auto le = li + kDataSize / sizeof *li;
    auto ri = (const uint64_t *) right.values;
    left->numValues = 0;
    for (; li != le; ++li, ++ri) {
        if (*li |= *ri)
            left->numValues += 1;
    }
    if (left->numValues == bitNumInt64s()) {
        BitView bits((uint64_t *) left->values, bitNumInt64s());
        if (bits.all())
            fill(alloc, left, right);
    }
}

//===========================================================================
template <std::integral T, typename A>
void IntegralSet<T,A>::Impl::insMeta(
    A * alloc,
    Node * left,
    const Node & right
) {
    auto li = left->nodes;
    auto le = li + left->numValues;
    auto ri = right.nodes;
    for (; li != le; ++li, ++ri) {
        insert(alloc, li, *ri);
        if (li->type != kFull)
            goto NOT_FULL;
    }
    // Convert to full node.
    fill(alloc, left, right);
    return;

NOT_FULL:
    ++li, ++ri;
    for (; li != le; ++li, ++ri)
        insert(alloc, li, *ri);
}


/****************************************************************************
*
*   insert(A *, Node &, Node &&)
*
***/

//===========================================================================
template <std::integral T, typename A>
void IntegralSet<T,A>::Impl::insert(A * alloc, Node * left, Node && right) {
    using Fn = void(A * alloc, Node * left, Node && right);
    static Fn * const functs[][kNodeTypes] = {
// LEFT                     RIGHT
//         empty  full  sm vec   vector   bitmap    meta
/*empty*/{ skip,  fill, copy,    copy,    copy,     copy     },
/*full */{ skip,  skip, skip,    skip,    skip,     skip     },
/*smv  */{ skip,  fill, insRSmv, insLSmv, insLSmv,  insLSmv  },
/*vec  */{ skip,  fill, insRSmv, insRVec, insLVec,  insLVec  },
/*bit  */{ skip,  fill, insRSmv, insRVec, insBit,   insError },
/*meta */{ skip,  fill, insRSmv, insRVec, insError, insMeta  },
    };
    functs[left->type][right.type](alloc, left, move(right));
}

//===========================================================================
template <std::integral T, typename A>
void IntegralSet<T,A>::Impl::insError(A * alloc, Node * left, Node && right) {
    insError(alloc, left, right);
}

//===========================================================================
template <std::integral T, typename A>
void IntegralSet<T,A>::Impl::insRSmv(A * alloc, Node * left, Node && right) {
    insRSmv(alloc, left, right);
}

//===========================================================================
template <std::integral T, typename A>
void IntegralSet<T,A>::Impl::insLSmv(A * alloc, Node * left, Node && right) {
    swap(*left, right);
    insRSmv(alloc, left, move(right));
}

//===========================================================================
template <std::integral T, typename A>
void IntegralSet<T,A>::Impl::insRVec(A * alloc, Node * left, Node && right) {
    insRVec(alloc, left, right);
}

//===========================================================================
template <std::integral T, typename A>
void IntegralSet<T,A>::Impl::insLVec(A * alloc, Node * left, Node && right) {
    swap(*left, right);
    insRVec(alloc, left, move(right));
}

//===========================================================================
template <std::integral T, typename A>
void IntegralSet<T,A>::Impl::insBit(A * alloc, Node * left, Node && right) {
    insBit(alloc, left, right);
}

//===========================================================================
template <std::integral T, typename A>
void IntegralSet<T,A>::Impl::insMeta(A * alloc, Node * left, Node && right) {
    auto li = left->nodes;
    auto le = li + left->numValues;
    auto ri = right.nodes;
    for (; li != le; ++li, ++ri) {
        insert(alloc, li, move(*ri));
        if (li->type != kFull)
            goto NOT_FULL;
    }
    fill(alloc, left, move(right));
    return;

NOT_FULL:
    ++li, ++ri;
    for (; li != le; ++li, ++ri)
        insert(alloc, li, move(*ri));
}


/****************************************************************************
*
*   erase(A* alloc, Node * left, const Node & right)
*
***/

//===========================================================================
template <std::integral T, typename A>
void IntegralSet<T,A>::Impl::erase(
    A * alloc,
    Node * left,
    const Node & right
) {
    using Fn = void(A * alloc, Node * left, const Node & right);
    static Fn * const functs[][kNodeTypes] = {
// LEFT                         RIGHT
//         empty full   sm vec     vector     bitmap     meta
/*empty*/{ skip, skip,  skip,      skip,      skip,      skip      },
/*full */{ skip, clear, eraChange, eraChange, eraChange, eraChange },
/*smv  */{ skip, clear, eraSmv,    eraLSmv,   eraLSmv,   eraLSmv   },
/*vec  */{ skip, clear, eraRSmv,   eraVec,    eraLVec,   eraLVec   },
/*bit  */{ skip, clear, eraRSmv,   eraRVec,   eraBit,    eraError  },
/*meta */{ skip, clear, eraRSmv,   eraRVec,   eraError,  eraMeta   },
    };
    functs[left->type][right.type](alloc, left, right);
}

//===========================================================================
template <std::integral T, typename A>
void IntegralSet<T,A>::Impl::eraError(
    A * alloc,
    Node * left,
    const Node & right
) {
    logMsgFatal() << "erase: incompatible node types, " << left->type
        << ", " << right.type;
}

//===========================================================================
template <std::integral T, typename A>
void IntegralSet<T,A>::Impl::eraChange(
    A * alloc,
    Node * left,
    const Node & right
) {
    auto ri = iterator::makeFirst(&right);
    assert(ri);

    // Convert from full to either bitmap or meta, and only then erase the
    // rest.
    erase(alloc, left, *ri, 1);
    erase(alloc, left, right);
}

//===========================================================================
template <std::integral T, typename A>
void IntegralSet<T,A>::Impl::eraArray(
    A * alloc,
    Node * left,
    storage_type * li,
    const Node & right,
    const storage_type * ri
) {
    auto le = set_difference(
        li, li + left->numValues,
        ri, ri + right.numValues,
        li
    );
    left->numValues = uint16_t(le - li);
    if (!left->numValues)
        clear(alloc, left);
}

//===========================================================================
template <std::integral T, typename A>
void IntegralSet<T,A>::Impl::eraLArray(
    A * alloc,
    Node * left,
    storage_type * li,
    const Node & right
) {
    // Go through values of left vector and skip (aka remove) the ones that
    // are found in right node (values to be erased).
    auto base = li;
    auto out = li;
    auto le = li + left->numValues;
    const Node * node = nullptr;
    storage_type value = 0;
    for (; li != le; ++li) {
        if (*li < value) {
            *out++ = *li;
            continue;
        }
        if (!find(&node, &value, right, *li)) {
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
    left->numValues = uint16_t(out - base);
    if (!left->numValues)
        clear(alloc, left);
}

//===========================================================================
template <std::integral T, typename A>
void IntegralSet<T,A>::Impl::eraRArray(
    A * alloc,
    Node * left,
    const Node & right,
    const storage_type * ri
) {
    auto re = ri + right.numValues;
    for (; ri != re; ++ri)
        erase(alloc, left, *ri, 1);
}

//===========================================================================
template <std::integral T, typename A>
void IntegralSet<T,A>::Impl::eraSmv(
    A * alloc,
    Node * left,
    const Node & right
) {
    eraArray(alloc, left, left->localValues, right, right.localValues);
}

//===========================================================================
template <std::integral T, typename A>
void IntegralSet<T,A>::Impl::eraLSmv(
    A * alloc,
    Node * left,
    const Node & right
) {
    assert(left->type == kSmVector);
    eraLArray(alloc, left, left->localValues, right);
}

//===========================================================================
template <std::integral T, typename A>
void IntegralSet<T,A>::Impl::eraRSmv(
    A * alloc,
    Node * left,
    const Node & right
) {
    assert(right.type == kSmVector);
    eraRArray(alloc, left, right, right.localValues);
}

//===========================================================================
template <std::integral T, typename A>
void IntegralSet<T,A>::Impl::eraVec(
    A * alloc,
    Node * left,
    const Node & right
) {
    eraArray(alloc, left, left->values, right, right.values);
}

//===========================================================================
template <std::integral T, typename A>
void IntegralSet<T,A>::Impl::eraLVec(
    A * alloc,
    Node * left,
    const Node & right
) {
    assert(left->type == kVector);
    eraLArray(alloc, left, left->values, right);
}

//===========================================================================
template <std::integral T, typename A>
void IntegralSet<T,A>::Impl::eraRVec(
    A * alloc,
    Node * left,
    const Node & right
) {
    assert(right.type == kVector);
    eraRArray(alloc, left, right, right.values);
}

//===========================================================================
template <std::integral T, typename A>
void IntegralSet<T,A>::Impl::eraBit(
    A * alloc,
    Node * left,
    const Node & right
) {
    auto li = (uint64_t *) left->values;
    auto le = li + kDataSize / sizeof *li;
    auto ri = (uint64_t *) right.values;
    left->numValues = 0;
    for (; li != le; ++li, ++ri) {
        if (*li &= ~*ri)
            left->numValues += 1;
    }
    if (!left->numValues)
        clear(alloc, left);
}

//===========================================================================
template <std::integral T, typename A>
void IntegralSet<T,A>::Impl::eraMeta(
    A * alloc,
    Node * left,
    const Node & right
) {
    assert(left->numValues == right.numValues);
    auto li = left->nodes;
    auto le = li + left->numValues;
    auto ri = right.nodes;
    for (; li != le; ++li, ++ri) {
        erase(alloc, li, *ri);
        if (li->type != kEmpty)
            goto NOT_EMPTY;
    }
    clear(alloc, left);
    return;

NOT_EMPTY:
    ++li, ++ri;
    for (; li != le; ++li, ++ri)
        erase(alloc, li, *ri);
}


/****************************************************************************
*
*   intersect(A * alloc, Node &, const Node &)
*
***/

//===========================================================================
template <std::integral T, typename A>
void IntegralSet<T,A>::Impl::intersect(
    A * alloc,
    Node * left,
    const Node & right
) {
    using Fn = void(A * alloc, Node * left, const Node & right);
    static Fn * const functs[][kNodeTypes] = {
// LEFT                         RIGHT
//         empty  full  sm vec    vector    bitmap     meta
/*empty*/{ skip,  skip, skip,     skip,     skip,      skip      },
/*full */{ clear, skip, copy,     copy,     copy,      copy      },
/*smv  */{ clear, skip, isecSmv,  isecLSmv, isecLSmv,  isecLSmv  },
/*vec  */{ clear, skip, isecRSmv, isecVec,  isecLVec,  isecLVec  },
/*bit  */{ clear, skip, isecRSmv, isecRVec, isecBit,   isecError },
/*meta */{ clear, skip, isecRSmv, isecRVec, isecError, isecMeta  },
    };
    functs[left->type][right.type](alloc, left, right);
}

//===========================================================================
template <std::integral T, typename A>
void IntegralSet<T,A>::Impl::isecError(
    A * alloc,
    Node * left,
    const Node & right
) {
    logMsgFatal() << "intersect: incompatible node types, " << left->type
        << ", " << right.type;
}

//===========================================================================
template <std::integral T, typename A>
void IntegralSet<T,A>::Impl::isecArray(
    A * alloc,
    Node * left,
    storage_type * li,
    const Node & right,
    const storage_type * ri
) {
    auto le = set_intersection(
        li, li + left->numValues,
        ri, ri + right.numValues,
        li
    );
    left->numValues = uint16_t(le - li);
    if (!left->numValues)
        clear(alloc, left);
}

//===========================================================================
template <std::integral T, typename A>
void IntegralSet<T,A>::Impl::isecLArray(
    A * alloc,
    Node * left,
    storage_type * li,
    const Node & right
) {
    // Go through values of left vector and remove the ones that aren't
    // found in right node.
    auto base = li;
    auto out = li;
    auto le = li + left->numValues;
    const Node * node = nullptr;
    storage_type value = 0;
    for (; li != le; ++li) {
        if (*li < value)
            continue;
        if (!find(&node, &value, right, *li))
            break;
        if (value == *li)
            *out++ = *li;
    }
    left->numValues = uint16_t(out - base);
    if (!left->numValues)
        clear(alloc, left);
}

//===========================================================================
template <std::integral T, typename A>
void IntegralSet<T,A>::Impl::isecSmv(
    A * alloc,
    Node * left,
    const Node & right
) {
    assert(left->type == kSmVector);
    isecArray(alloc, left, left->localValues, right, right.localValues);
}

//===========================================================================
template <std::integral T, typename A>
void IntegralSet<T,A>::Impl::isecLSmv(
    A * alloc,
    Node * left,
    const Node & right
) {
    assert(left->type == kSmVector);
    isecLArray(alloc, left, left->localValues, right);
}

//===========================================================================
template <std::integral T, typename A>
void IntegralSet<T,A>::Impl::isecRSmv(
    A * alloc,
    Node * left,
    const Node & right
) {
    assert(right.type == kSmVector);
    Node tmp;
    copy(alloc, &tmp, right);
    isecLArray(alloc, &tmp, tmp.localValues, *left);
    swap(*left, tmp);
    destroy(alloc, &tmp);
}

//===========================================================================
template <std::integral T, typename A>
void IntegralSet<T,A>::Impl::isecVec(
    A * alloc,
    Node * left,
    const Node & right
) {
    assert(left->type == kVector);
    isecArray(alloc, left, left->values, right, right.values);
}

//===========================================================================
template <std::integral T, typename A>
void IntegralSet<T,A>::Impl::isecLVec(
    A * alloc,
    Node * left,
    const Node & right
) {
    assert(left->type == kVector);
    isecLArray(alloc, left, left->values, right);
}

//===========================================================================
template <std::integral T, typename A>
void IntegralSet<T,A>::Impl::isecRVec(
    A * alloc,
    Node * left,
    const Node & right
) {
    assert(right.type == kVector);
    Node tmp;
    copy(alloc, &tmp, right);
    swap(*left, tmp);
    isecLArray(alloc, left, left->values, tmp);
    destroy(alloc, &tmp);
}

//===========================================================================
template <std::integral T, typename A>
void IntegralSet<T,A>::Impl::isecBit(
    A * alloc,
    Node * left,
    const Node & right
) {
    auto li = (uint64_t *) left->values;
    auto le = li + kDataSize / sizeof *li;
    auto ri = (uint64_t *) right.values;
    left->numValues = 0;
    for (; li != le; ++li, ++ri) {
        if (*li &= *ri)
            left->numValues += 1;
    }
    if (!left->numValues)
        clear(alloc, left);
}

//===========================================================================
template <std::integral T, typename A>
void IntegralSet<T,A>::Impl::isecMeta(
    A * alloc,
    Node * left,
    const Node & right
) {
    auto li = left->nodes;
    auto le = li + left->numValues;
    auto ri = right.nodes;
    for (; li != le; ++li, ++ri) {
        intersect(alloc, li, *ri);
        if (li->type != kEmpty)
            goto NOT_EMPTY;
    }
    clear(alloc, left);
    return;

NOT_EMPTY:
    ++li, ++ri;
    for (; li != le; ++li, ++ri)
        intersect(alloc, li, *ri);
}


/****************************************************************************
*
*   intersect(A * alloc, Node &, Node &&)
*
***/

//===========================================================================
template <std::integral T, typename A>
void IntegralSet<T,A>::Impl::intersect(A * alloc, Node * left, Node && right) {
    using Fn = void(A * alloc, Node * left, Node && right);
    static Fn * const functs[][kNodeTypes] = {
// LEFT                         RIGHT
//         empty  full  sm vec    vector    bitmap     meta
/*empty*/{ skip,  skip, clear,    clear,    clear,     clear     },
/*full */{ clear, skip, copy,     copy,     copy,      copy      },
/*smv  */{ clear, skip, isecSmv,  isecLSmv, isecLSmv,  isecLSmv  },
/*vec  */{ clear, skip, isecRSmv, isecVec,  isecLVec,  isecLVec  },
/*bit  */{ clear, skip, isecRSmv, isecRVec, isecBit,   isecError },
/*meta */{ clear, skip, isecRSmv, isecRVec, isecError, isecMeta  },
    };
    functs[left->type][right.type](alloc, left, move(right));
}

//===========================================================================
template <std::integral T, typename A>
void IntegralSet<T,A>::Impl::isecError(A * alloc, Node * left, Node && right) {
    isecError(alloc, left, right);
}

//===========================================================================
template <std::integral T, typename A>
void IntegralSet<T,A>::Impl::isecSmv(A * alloc, Node * left, Node && right) {
    isecSmv(alloc, left, right);
}

//===========================================================================
template <std::integral T, typename A>
void IntegralSet<T,A>::Impl::isecLSmv(A * alloc, Node * left, Node && right) {
    isecLSmv(alloc, left, right);
}

//===========================================================================
template <std::integral T, typename A>
void IntegralSet<T,A>::Impl::isecRSmv(A * alloc, Node * left, Node && right) {
    swap(*left, right);
    isecLSmv(alloc, left, right);
}

//===========================================================================
template <std::integral T, typename A>
void IntegralSet<T,A>::Impl::isecVec(A * alloc, Node * left, Node && right) {
    isecVec(alloc, left, right);
}

//===========================================================================
template <std::integral T, typename A>
void IntegralSet<T,A>::Impl::isecLVec(A * alloc, Node * left, Node && right) {
    isecLVec(alloc, left, right);
}

//===========================================================================
template <std::integral T, typename A>
void IntegralSet<T,A>::Impl::isecRVec(A * alloc, Node * left, Node && right) {
    swap(*left, right);
    isecLVec(alloc, left, right);
}

//===========================================================================
template <std::integral T, typename A>
void IntegralSet<T,A>::Impl::isecBit(A * alloc, Node * left, Node && right) {
    isecBit(alloc, left, right);
}

//===========================================================================
template <std::integral T, typename A>
void IntegralSet<T,A>::Impl::isecMeta(A * alloc, Node * left, Node && right) {
    auto li = left->nodes;
    auto le = li + left->numValues;
    auto ri = right.nodes;
    for (; li != le; ++li, ++ri) {
        intersect(alloc, li, move(*ri));
        if (li->type != kEmpty)
            goto NOT_EMPTY;
    }
    clear(alloc, left);
    return;

NOT_EMPTY:
    ++li, ++ri;
    for (; li != le; ++li, ++ri)
        intersect(alloc, li, move(*ri));
}


/****************************************************************************
*
*   IntegralSet::Node
*
***/

//===========================================================================
template <std::integral T, typename A>
IntegralSet<T,A>::Node::Node()
    : type{Impl::kEmpty}
    , depth{0}
    , base{0}
    , numBytes{0}
    , numValues{0}
    , values{nullptr}
{}


/****************************************************************************
*
*   IntegralSet
*
***/

//===========================================================================
template <std::integral T, typename A>
IntegralSet<T,A>::IntegralSet(const A & alloc) noexcept
    : m_alloc(alloc)
{}

//===========================================================================
template <std::integral T, typename A>
IntegralSet<T,A>::IntegralSet(IntegralSet && from) noexcept
    : m_alloc(move(from.m_alloc))
{
    Impl::copy(&m_alloc, &m_node, move(from.m_node));
}

//===========================================================================
template <std::integral T, typename A>
IntegralSet<T,A>::IntegralSet(IntegralSet && from, const A & alloc)
    : m_alloc(alloc)
{
    if (m_alloc == from.m_alloc) {
        Impl::copy(&m_alloc, &m_node, move(from.m_node));
    } else {
        Impl::copy(&m_alloc, &m_node, from.m_node);
    }
}

//===========================================================================
template <std::integral T, typename A>
IntegralSet<T,A>::IntegralSet(const IntegralSet & from)
    : m_alloc(allocator_traits<A>::select_on_container_copy_construction(
        from.m_alloc
    ))
{
    Impl::copy(&m_alloc, &m_node, from.m_node);
}

//===========================================================================
template <std::integral T, typename A>
IntegralSet<T,A>::IntegralSet(const IntegralSet & from, const A & alloc)
    : m_alloc(alloc)
{
    Impl::copy(&m_alloc, &m_node, from.m_node);
}

//===========================================================================
template <std::integral T, typename A>
IntegralSet<T,A>::IntegralSet(
    std::initializer_list<value_type> il,
    const A & alloc
)
    : m_alloc(alloc)
{
    insert(il);
}

//===========================================================================
template <std::integral T, typename A>
IntegralSet<T,A>::IntegralSet(string_view from, const A & alloc)
    : m_alloc(alloc)
{
    insert(from);
}

//===========================================================================
template <std::integral T, typename A>
IntegralSet<T,A>::IntegralSet(
    value_type start,
    size_t len,
    const A & alloc
)
    : m_alloc(alloc)
{
    insert(start, len);
}

//===========================================================================
template <std::integral T, typename A>
IntegralSet<T,A>::~IntegralSet() {
    clear();
}

//===========================================================================
template <std::integral T, typename A>
IntegralSet<T,A> & IntegralSet<T,A>::operator=(
    IntegralSet && from
) noexcept {
    if (this != &from) {
        if constexpr(
            allocator_traits<A>::propagate_on_container_move_assignment::value
        ) {
            m_alloc = move(from.m_alloc);
        }
        assign(move(from));
    }
    return *this;
}

//===========================================================================
template <std::integral T, typename A>
IntegralSet<T,A> & IntegralSet<T,A>::operator=(
    const IntegralSet & from
) {
    if (this != &from) {
        if constexpr(
            allocator_traits<A>::propagate_on_container_copy_assignment::value
        ) {
            m_alloc = from.m_alloc;
        }
        assign(from);
    }
    return *this;
}

//===========================================================================
template <std::integral T, typename A>
auto IntegralSet<T,A>::get_allocator() const -> allocator_type {
    return m_alloc;
}

//===========================================================================
template <std::integral T, typename A>
auto IntegralSet<T,A>::begin() const -> iterator {
    return iterator::makeFirst(&m_node);
}

//===========================================================================
template <std::integral T, typename A>
auto IntegralSet<T,A>::end() const -> iterator {
    return iterator::makeEnd(&m_node);
}

//===========================================================================
template <std::integral T, typename A>
auto IntegralSet<T,A>::rbegin() const -> reverse_iterator {
    return reverse_iterator(end());
}

//===========================================================================
template <std::integral T, typename A>
auto IntegralSet<T,A>::rend() const -> reverse_iterator {
    return reverse_iterator(begin());
}

//===========================================================================
template <std::integral T, typename A>
auto IntegralSet<T,A>::ranges() const -> RangeRange {
    return RangeRange(begin());
}

//===========================================================================
template <std::integral T, typename A>
bool IntegralSet<T,A>::empty() const {
    return m_node.type == Impl::kEmpty;
}

//===========================================================================
template <std::integral T, typename A>
size_t IntegralSet<T,A>::size() const {
    return Impl::count(m_node);
}

//===========================================================================
template <std::integral T, typename A>
size_t IntegralSet<T,A>::max_size() const {
    return numeric_limits<storage_type>::max();
}

//===========================================================================
template <std::integral T, typename A>
void IntegralSet<T,A>::clear() {
    Impl::clear(&m_alloc, &m_node);
}

//===========================================================================
template <std::integral T, typename A>
void IntegralSet<T,A>::fill() {
    Impl::fill(&m_alloc, &m_node);
}

//===========================================================================
template <std::integral T, typename A>
void IntegralSet<T,A>::assign(IntegralSet && from) {
    if (this != &from) {
        if (m_alloc == from.m_alloc) {
            Impl::copy(&m_alloc, &m_node, move(from.m_node));
        } else {
            Impl::copy(&m_alloc, &m_node, from.m_node);
        }
    }
}

//===========================================================================
template <std::integral T, typename A>
void IntegralSet<T,A>::assign(const IntegralSet & from) {
    if (this != &from)
        Impl::copy(&m_alloc, &m_node, from.m_node);
}

//===========================================================================
template <std::integral T, typename A>
void IntegralSet<T,A>::assign(value_type value) {
    clear();
    insert(value);
}

//===========================================================================
template <std::integral T, typename A>
void IntegralSet<T,A>::assign(std::initializer_list<value_type> il) {
    clear();
    insert(il);
}

//===========================================================================
template <std::integral T, typename A>
void IntegralSet<T,A>::assign(value_type start, size_t len) {
    clear();
    insert(start, len);
}

//===========================================================================
template <std::integral T, typename A>
void IntegralSet<T,A>::assign(string_view src) {
    clear();
    insert(src);
}

//===========================================================================
template <std::integral T, typename A>
void IntegralSet<T,A>::insert(IntegralSet && other) {
    if (this != &other)
        Impl::insert(&m_alloc, &m_node, move(other.m_node));
}

//===========================================================================
template <std::integral T, typename A>
void IntegralSet<T,A>::insert(const IntegralSet & other) {
    if (this != &other)
        Impl::insert(&m_alloc, &m_node, other.m_node);
}

//===========================================================================
template <std::integral T, typename A>
bool IntegralSet<T,A>::insert(value_type value) {
    return Impl::insert(&m_alloc, &m_node, Impl::toStorage(value), 1);
}

//===========================================================================
template <std::integral T, typename A>
void IntegralSet<T,A>::insert(std::initializer_list<value_type> il) {
    iInsert(il.begin(), il.end());
}

//===========================================================================
template <std::integral T, typename A>
void IntegralSet<T,A>::insert(value_type start, size_t len) {
    if (!len)
        return;
    auto sstart = Impl::toStorage(start);
    if (len == dynamic_extent)
        len = Impl::valueMask(0) - sstart + 1;
    Impl::insert(&m_alloc, &m_node, sstart, len);
}

//===========================================================================
template <std::integral T, typename A>
void IntegralSet<T,A>::insert(string_view src) {
    for (;;) {
        while (src[0] == ' ')
            src.remove_prefix(1);

        T first;
        auto res = from_chars(src.data(), src.data() + src.size(), first);
        if (res.ec != errc())
            break;

        auto sepLen = 0;
        if (*res.ptr == '-') {
            sepLen = 1;
        } else if (*res.ptr == '.' && res.ptr[1] == '.') {
            sepLen = 2;
        } else {
            insert(first);
            src.remove_prefix(res.ptr - src.data());
            continue;
        }

        src.remove_prefix(res.ptr - src.data() + sepLen);
        T second;
        res = from_chars(src.data(), src.data() + src.size(), second);
        if (res.ec != errc() || first > second)
            break;
        insert(first, second - first + 1);
        src.remove_prefix(res.ptr - src.data());
    }
}

//===========================================================================
template <std::integral T, typename A>
bool IntegralSet<T,A>::erase(value_type value) {
    return Impl::erase(&m_alloc, &m_node, Impl::toStorage(value), 1);
}

//===========================================================================
template <std::integral T, typename A>
void IntegralSet<T,A>::erase(iterator where) {
    assert(where);
    erase(*where);
}

//===========================================================================
template <std::integral T, typename A>
void IntegralSet<T,A>::erase(const IntegralSet & other) {
    if (this != &other) {
        Impl::erase(&m_alloc, &m_node, other.m_node);
    } else {
        clear();
    }
}

//===========================================================================
template <std::integral T, typename A>
void IntegralSet<T,A>::erase(value_type start, size_t len) {
    if (!len)
        return;
    auto sstart = Impl::toStorage(start);
    if (len == dynamic_extent)
        len = Impl::valueMask(0) - sstart + 1;
    Impl::erase(&m_alloc, &m_node, sstart, len);
}

//===========================================================================
template <std::integral T, typename A>
auto IntegralSet<T,A>::pop_front() -> value_type {
    auto val = front();
    erase(val);
    return val;
}

//===========================================================================
template <std::integral T, typename A>
auto IntegralSet<T,A>::pop_back() -> value_type {
    auto val = back();
    erase(val);
    return val;
}

//===========================================================================
template <std::integral T, typename A>
void IntegralSet<T,A>::intersect(IntegralSet && other) {
    if (this != &other)
        Impl::intersect(&m_alloc, &m_node, move(other.m_node));
}

//===========================================================================
template <std::integral T, typename A>
void IntegralSet<T,A>::intersect(const IntegralSet & other) {
    if (this != &other)
        Impl::intersect(&m_alloc, &m_node, other.m_node);
}

//===========================================================================
template <std::integral T, typename A>
void IntegralSet<T,A>::swap(IntegralSet & other) {
    assert(m_alloc == other.m_alloc);
    Impl::swap(m_node, other.m_node);
}

//===========================================================================
template <std::integral T, typename A>
strong_ordering IntegralSet<T,A>::compare(const IntegralSet & right) const {
    if (auto cmp = Impl::compare(m_node, right.m_node)) {
        return cmp < 0 ? strong_ordering::less : strong_ordering::greater;
    }
    return strong_ordering::equal;
}

//===========================================================================
template <std::integral T, typename A>
strong_ordering IntegralSet<T,A>::operator<=>(
    const IntegralSet & other
) const {
    return compare(other);
}

//===========================================================================
template <std::integral T, typename A>
bool IntegralSet<T,A>::operator==(const IntegralSet & right) const {
    return compare(right) == 0;
}

//===========================================================================
template <std::integral T, typename A>
auto IntegralSet<T,A>::front() const -> value_type {
    assert(!empty());
    auto i = begin();
    return *i;
}

//===========================================================================
template <std::integral T, typename A>
auto IntegralSet<T,A>::back() const -> value_type {
    assert(!empty());
    auto i = rbegin();
    return *i;
}

//===========================================================================
template <std::integral T, typename A>
size_t IntegralSet<T,A>::count() const {
    return size();
}

//===========================================================================
template <std::integral T, typename A>
size_t IntegralSet<T,A>::count(value_type val) const {
    return count(val, 1);
}

//===========================================================================
template <std::integral T, typename A>
size_t IntegralSet<T,A>::count(value_type start, size_t len) const {
    return Impl::count(m_node, Impl::toStorage(start), len);
}

//===========================================================================
template <std::integral T, typename A>
auto IntegralSet<T,A>::find(value_type val) const -> iterator {
    auto first = lowerBound(val);
    return first && *first == val ? first : end();
}

//===========================================================================
template <std::integral T, typename A>
bool IntegralSet<T,A>::contains(value_type val) const {
    return count(val);
}

//===========================================================================
template <std::integral T, typename A>
bool IntegralSet<T,A>::contains(const IntegralSet & other) const {
    if (this != &other) {
        return Impl::contains(m_node, other.m_node);
    } else {
        return true;
    }
}

//===========================================================================
template <std::integral T, typename A>
bool IntegralSet<T,A>::intersects(const IntegralSet & other) const {
    return Impl::intersects(m_node, other.m_node);
}

//===========================================================================
template <std::integral T, typename A>
auto IntegralSet<T,A>::findLessEqual(value_type val) const -> iterator {
    return iterator::makeLast(&m_node, val);
}

//===========================================================================
template <std::integral T, typename A>
auto IntegralSet<T,A>::findLess(value_type val) const -> iterator {
    return val > numeric_limits<value_type>::min()
        ? findLessEqual(val - 1)
        : end();
}

//===========================================================================
template <std::integral T, typename A>
auto IntegralSet<T,A>::lowerBound(value_type val) const -> iterator {
    return iterator::makeFirst(&m_node, val);
}

//===========================================================================
template <std::integral T, typename A>
auto IntegralSet<T,A>::upperBound(value_type val) const -> iterator {
    return val < numeric_limits<value_type>::max() 
        ? lowerBound(val + 1) 
        : end();
}

//===========================================================================
template <std::integral T, typename A>
auto IntegralSet<T,A>::equalRange(value_type val) const
    -> pair<iterator, iterator>
{
    auto first = lowerBound(val);
    auto last = first;
    if (last && *last == val)
        ++last;
    return {first, last};
}

//===========================================================================
template <std::integral T, typename A>
auto IntegralSet<T,A>::firstContiguous(iterator where) const -> iterator {
    return where.firstContiguous();
}

//===========================================================================
template <std::integral T, typename A>
auto IntegralSet<T,A>::lastContiguous(iterator where) const -> iterator {
    return where.lastContiguous();
}

//===========================================================================
// Private
//===========================================================================
template <std::integral T, typename A>
void IntegralSet<T,A>::iInsert(
    const value_type * first,
    const value_type * last
) {
    if constexpr (is_same_v<value_type, storage_type>) {
        Impl::insert(&m_alloc, &m_node, first, last);
    } else {
        storage_type svals[Impl::vecMaxValues()];
        auto pos = 0;
        for (; first < last; ++first) {
            svals[pos] = Impl::toStorage(*first);
            if (++pos == ::size(svals)) {
                Impl::insert(&m_alloc, &m_node, svals, svals + pos);
                pos = 0;
            }
        }
        if (pos)
            Impl::insert(&m_alloc, &m_node, svals, svals + pos);
    }
}


/****************************************************************************
*
*   IntegralSet::Iter
*
***/

//===========================================================================
// static
template <std::integral T, typename A>
auto IntegralSet<T,A>::Iter::makeEnd(const Node * node) -> Iter {
    while (node->depth) {
        auto pos = Impl::nodePos(Impl::absBase(*node), node->depth - 1);
        node += Impl::numNodes(node->depth) - pos;
        node = node->nodes;
    }
    return Iter(node);
}

//===========================================================================
// static
template <std::integral T, typename A>
auto IntegralSet<T,A>::Iter::makeFirst(const Node * node) -> Iter {
    return makeFirst(node, Impl::toValue(Impl::absBase(*node)));
}

//===========================================================================
// static
template <std::integral T, typename A>
auto IntegralSet<T,A>::Iter::makeFirst(
    const Node * node,
    value_type value
) -> Iter {
    auto svalue = Impl::toStorage(value);
    if (Impl::find(&node, &svalue, *node, svalue))
        return {node, Impl::toValue(svalue)};
    return {node};
}

//===========================================================================
// static
template <std::integral T, typename A>
auto IntegralSet<T,A>::Iter::makeLast(const Node * node) -> Iter {
    return makeLast(node, Impl::toValue(Impl::absFinal(*node)));
}

//===========================================================================
// static
template <std::integral T, typename A>
auto IntegralSet<T,A>::Iter::makeLast(
    const Node * node,
    value_type value
) -> Iter {
    auto svalue = Impl::toStorage(value);
    if (Impl::rfind(&node, &svalue, *node, svalue))
        return {node, Impl::toValue(svalue)};
    return {node};
}

//===========================================================================
template <std::integral T, typename A>
IntegralSet<T,A>::Iter::Iter(const Node * node)
    : m_node(node)
{
    assert(node->depth == 0 && node->base == 0);
}

//===========================================================================
template <std::integral T, typename A>
IntegralSet<T,A>::Iter::Iter(const Node * node, value_type value)
    : m_node(node)
    , m_value(value)
    , m_endmark(false)
{}

//===========================================================================
template <std::integral T, typename A>
bool IntegralSet<T,A>::Iter::operator== (const Iter & right) const {
    return m_value == right.m_value
        && m_endmark == right.m_endmark
        && (m_node == right.m_node || !m_node || !right.m_node);
}

//===========================================================================
template <std::integral T, typename A>
auto IntegralSet<T,A>::Iter::operator++() -> Iter & {
    auto svalue = Impl::toStorage(m_value);
    if (!*this) {
        if (m_node) {
            auto from = Impl::absBase(*m_node);
            if (Impl::find(&m_node, &svalue, *m_node, from)) {
                m_value = Impl::toValue(svalue);
                m_endmark = false;
            }
        }
        return *this;
    }
    if (svalue < Impl::absFinal(*m_node)) {
        svalue += 1;
        if (Impl::find(&m_node, &svalue, *m_node, svalue)) {
            m_value = Impl::toValue(svalue);
            return *this;
        }
    }

CHECK_DEPTH:
    if (m_node->depth == 0) {
        *this = end();
        return *this;
    }

CHECK_END_OF_DEPTH:
    if (Impl::absFinal(*m_node)
            == Impl::absFinal(svalue, m_node->depth - 1)
    ) {
        m_node += 1;
        assert(m_node->type == Impl::kMetaParent);
        m_node = m_node->nodes;
        svalue = Impl::absBase(*m_node);
        goto CHECK_DEPTH;
    }

    m_node += 1;
    svalue = Impl::absBase(*m_node);
    if (Impl::find(&m_node, &svalue, *m_node, svalue)) {
        m_value = Impl::toValue(svalue);
        return *this;
    }
    goto CHECK_END_OF_DEPTH;
}

//===========================================================================
template <std::integral T, typename A>
auto IntegralSet<T,A>::Iter::operator--() -> Iter & {
    auto svalue = Impl::toStorage(m_value);
    if (!*this) {
        if (m_node) {
            auto from = Impl::absFinal(*m_node);
            if (Impl::rfind(&m_node, &svalue, *m_node, from)) {
                m_value = Impl::toValue(svalue);
                m_endmark = false;
            }
        }
        return *this;
    }
    if (svalue > Impl::absBase(*m_node)) {
        svalue -= 1;
        if (Impl::rfind(&m_node, &svalue, *m_node, svalue)) {
            m_value = Impl::toValue(svalue);
            return *this;
        }
    }

CHECK_DEPTH:
    if (m_node->depth == 0) {
        *this = end();
        return *this;
    }

CHECK_END_OF_DEPTH:
    if (Impl::absBase(*m_node) == Impl::absBase(svalue, m_node->depth - 1)) {
        m_node += Impl::numNodes(m_node->depth);
        assert(m_node->type == Impl::kMetaParent);
        m_node = m_node->nodes;
        svalue = Impl::absFinal(*m_node);
        goto CHECK_DEPTH;
    }

    m_node -= 1;
    svalue = Impl::absFinal(*m_node);
    if (Impl::rfind(&m_node, &svalue, *m_node, svalue)) {
        m_value = Impl::toValue(svalue);
        return *this;
    }
    goto CHECK_END_OF_DEPTH;
}

//===========================================================================
template <std::integral T, typename A>
auto IntegralSet<T,A>::Iter::end() const -> Iter {
    return iterator::makeEnd(m_node);
}

//===========================================================================
template <std::integral T, typename A>
auto IntegralSet<T,A>::Iter::firstContiguous() const -> Iter {
    auto node = m_node;
    auto svalue = Impl::toStorage(m_value);
    if (Impl::rcontig(&node, &svalue, *node, svalue))
        return {node, Impl::toValue(svalue)};

    // Use a temporary to scout out the next node (which may require traversing
    // the tree to get there) since if it starts with a discontinuity we'll end
    // the search without advancing.
    auto ptr = node;

CHECK_DEPTH:
    if (ptr->depth == 0)
        return {node, Impl::toValue(svalue)};

CHECK_END_OF_DEPTH:
    if (Impl::absBase(*ptr) == Impl::absBase(svalue, ptr->depth - 1)) {
        ptr += Impl::numNodes(ptr->depth);
        assert(ptr->type == Impl::kMetaParent);
        ptr = ptr->nodes;
        goto CHECK_DEPTH;
    }

    ptr -= 1;
    if (Impl::rcontig(&node, &svalue, *ptr, Impl::absFinal(*ptr)))
        return {node, Impl::toValue(svalue)};
    goto CHECK_END_OF_DEPTH;
}

//===========================================================================
template <std::integral T, typename A>
auto IntegralSet<T,A>::Iter::lastContiguous() const -> Iter {
    auto node = m_node;
    auto svalue = Impl::toStorage(m_value);
    if (Impl::contig(&node, &svalue, *node, svalue))
        return {node, Impl::toValue(svalue)};

    // Use a temporary to scout out the next node (which may require traversing
    // the tree to find) since if it starts with a discontinuity we end the
    // search at the current node.
    auto ptr = node;

CHECK_DEPTH:
    if (ptr->depth == 0)
        return {node, Impl::toValue(svalue)};

CHECK_END_OF_DEPTH:
    if (Impl::absFinal(*ptr) == Impl::absFinal(svalue, ptr->depth - 1)) {
        ptr += 1;
        assert(ptr->type == Impl::kMetaParent);
        ptr = ptr->nodes;
        goto CHECK_DEPTH;
    }

    ptr += 1;
    if (Impl::contig(&node, &svalue, *ptr, Impl::absBase(*ptr)))
        return {node, Impl::toValue(svalue)};
    goto CHECK_END_OF_DEPTH;
}


/****************************************************************************
*
*   UnsignedSet::RangeIter
*
***/

//===========================================================================
template <std::integral T, typename A>
IntegralSet<T,A>::RangeIter::RangeIter(iterator where)
    : m_low(where)
    , m_high(where)
{
    if (m_low) {
        m_high = m_low.lastContiguous();
        m_value = { *m_low, *m_high };
    }
}

//===========================================================================
template <std::integral T, typename A>
bool IntegralSet<T,A>::RangeIter::operator== (
    const RangeIter & other
) const {
    return m_low == other.m_low;
}

//===========================================================================
template <std::integral T, typename A>
auto IntegralSet<T,A>::RangeIter::operator++() -> RangeIter & {
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
template <std::integral T, typename A>
auto IntegralSet<T,A>::RangeIter::operator--() -> RangeIter & {
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

template class IntegralSet<int>;
template class IntegralSet<unsigned>;

template class IntegralSet<unsigned,
    std::pmr::polymorphic_allocator<unsigned>>;
