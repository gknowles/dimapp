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


namespace {
enum NodeType {
    kEmpty,     // contains no values
    kFull,      // contains all values in node's domain
    kVector,    // has vector of values
    kBitmap,    // has bitmap of values
    kMeta,      // vector of child nodes
};

struct IImplBase {
    virtual size_t size(const UnsignedSet::Node & node) = 0;
    virtual void destroy(UnsignedSet::Node & node) = 0;
    virtual bool insert(UnsignedSet::Node & node, unsigned value) = 0;
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
    size_t size(const UnsignedSet::Node & node) override;
    void destroy(UnsignedSet::Node & node) override;
    bool insert(UnsignedSet::Node & node, unsigned value) override;
};
} // namespace
static EmptyImpl s_emptyImpl;

//===========================================================================
size_t EmptyImpl::size(const UnsignedSet::Node & node) {
    return 0;
}

//===========================================================================
void EmptyImpl::destroy(UnsignedSet::Node & node) 
{}

//===========================================================================
bool EmptyImpl::insert(UnsignedSet::Node & node, unsigned value) {
    node.type = kVector;
    node.numBytes = kDataSize;
    node.numValues = 1;
    node.values = (unsigned *) malloc(node.numBytes);
    node.values[0] = value;
    return true;
}


/****************************************************************************
*
*   FullImpl
*
***/

namespace {
struct FullImpl : IImplBase {
    size_t size(const UnsignedSet::Node & node) override;
    void destroy(UnsignedSet::Node & node) override;
    bool insert(UnsignedSet::Node & node, unsigned value) override;
};
} // namespace
static FullImpl s_fullImpl;

//===========================================================================
size_t FullImpl::size(const UnsignedSet::Node & node) {
    return 0;
}

//===========================================================================
void FullImpl::destroy(UnsignedSet::Node & node) 
{}

//===========================================================================
bool FullImpl::insert(UnsignedSet::Node & node, unsigned value) {
    return false;
}


/****************************************************************************
*
*   VectorImpl
*
***/

namespace {
struct VectorImpl : IImplBase {
    size_t maxValues() const { 
        return kDataSize / sizeof(UnsignedSet::Node::values); 
    }

    size_t size(const UnsignedSet::Node & node) override;
    void destroy(UnsignedSet::Node & node) override;
    bool insert(UnsignedSet::Node & node, unsigned value) override;
};
} // namespace
static VectorImpl s_vectorImpl;

//===========================================================================
size_t VectorImpl::size(const UnsignedSet::Node & node) {
    return node.numValues;
}

//===========================================================================
void VectorImpl::destroy(UnsignedSet::Node & node) {
    if (node.values)
        free(node.values);
}

//===========================================================================
bool VectorImpl::insert(UnsignedSet::Node & node, unsigned value) {
    auto last = node.values + node.numValues;
    auto ptr = upper_bound(node.values, last, value);
    if (ptr == last) {
        if (node.numValues == maxValues()) {
            assert(0 && "convert to MetaNode (or BitmapNode :P)");
        } else {
            *ptr = value;
            node.numValues += 1;
        }
        return true;
    }
    if (*ptr == value)
        return false;
    if (node.numValues == maxValues()) {
        assert(0 && "convert to MetaNode (or BitmapNode :P)");
    } else {
        memmove(ptr + 1, ptr, last - ptr);
        *ptr = value;
        node.numValues += 1;
    }
    return true;
}


/****************************************************************************
*
*   MetaImpl
*
***/

namespace {
struct MetaImpl : IImplBase {
    size_t maxNodes() const { 
        return kDataSize / sizeof(UnsignedSet::Node::nodes); 
    }

    size_t size(const UnsignedSet::Node & node) override;
    void destroy(UnsignedSet::Node & node) override;
    bool insert(UnsignedSet::Node & node, unsigned value) override;
};
} // namespace
static MetaImpl s_metaImpl;

//===========================================================================
size_t MetaImpl::size(const UnsignedSet::Node & node) {
    size_t num = 0;
    auto * ptr = node.nodes;
    auto * last = ptr + node.numValues;
    for (; ptr != last; ++ptr)
        num += impl(*ptr)->size(*ptr);
    return num;
}

//===========================================================================
void MetaImpl::destroy(UnsignedSet::Node & node) {
    if (auto * ptr = node.nodes) {
        auto * last = ptr + node.numValues;
        for (; ptr != last; ++ptr)
            impl(*ptr)->destroy(*ptr);
        free(node.nodes);
    }
}

//===========================================================================
bool MetaImpl::insert(UnsignedSet::Node & node, unsigned value) {
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
}

//===========================================================================
void UnsignedSet::erase(unsigned value) {
}

//===========================================================================
void UnsignedSet::swap(UnsignedSet & other) {
}
