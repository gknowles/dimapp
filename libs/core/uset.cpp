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

struct NodeBase : UnsignedSet::Node {
    virtual size_t size() const = 0;
    virtual void destroy() = 0;
    virtual bool insert(unsigned value) = 0;
};

} // namespace


/****************************************************************************
*
*   Helpers
*
***/

static NodeBase & nodeType(UnsignedSet::Node & node);
static const NodeBase & nodeType(const UnsignedSet::Node & node);


/****************************************************************************
*
*   EmptyNode
*
***/

namespace {
struct EmptyNode : NodeBase {
    size_t size() const override;
    void destroy() override;
    bool insert(unsigned value) override;
};
} // namespace

//===========================================================================
size_t EmptyNode::size() const {
    return 0;
}

//===========================================================================
void EmptyNode::destroy() 
{}

//===========================================================================
bool EmptyNode::insert(unsigned value) {
    m_type = kVector;
    m_numBytes = kDataSize;
    m_numValues = 1;
    m_values = (unsigned *) malloc(m_numBytes);
    m_values[0] = value;
    return true;
}


/****************************************************************************
*
*   FullNode
*
***/

namespace {
struct FullNode : NodeBase {
    size_t size() const override;
    void destroy() override;
    bool insert(unsigned value) override;
};
} // namespace

//===========================================================================
size_t FullNode::size() const {
    return 0;
}

//===========================================================================
void FullNode::destroy() 
{}

//===========================================================================
bool FullNode::insert(unsigned value) {
    return false;
}


/****************************************************************************
*
*   VectorNode
*
***/

namespace {
struct VectorNode : NodeBase {
    size_t maxValues() const { return kDataSize / sizeof(*m_values); }

    size_t size() const override;
    void destroy() override;
    bool insert(unsigned value) override;
};
} // namespace

//===========================================================================
size_t VectorNode::size() const {
    return m_numValues;
}

//===========================================================================
void VectorNode::destroy() {
    if (m_values)
        free(m_values);
}

//===========================================================================
bool VectorNode::insert(unsigned value) {
    auto last = m_values + m_numValues;
    auto ptr = upper_bound(m_values, last, value);
    if (ptr == last) {
        if (m_numValues == maxValues()) {
            assert(0 && "convert to MetaNode (or BitmapNode :P)");
        } else {
            *ptr = value;
            m_numValues += 1;
        }
        return true;
    }
    if (*ptr == value)
        return false;
    if (m_numValues == maxValues()) {
        assert(0 && "convert to MetaNode (or BitmapNode :P)");
    } else {
        memmove(ptr + 1, ptr, last - ptr);
        *ptr = value;
        m_numValues += 1;
    }
    return true;
}


/****************************************************************************
*
*   MetaNode
*
***/

namespace {
struct MetaNode : NodeBase {
    size_t maxNodes() const { return kDataSize / sizeof(*m_nodes); }

    size_t size() const override;
    void destroy() override;
    bool insert(unsigned value) override;
};
} // namespace

//===========================================================================
size_t MetaNode::size() const {
    size_t num = 0;
    for (unsigned i = 0; i < m_numValues; ++i)
        num += nodeType(m_nodes[i]).size();
    return num;
}

//===========================================================================
void MetaNode::destroy() {
    if (m_nodes) {
        for (unsigned i = 0; i < m_numValues; ++i)
            nodeType(m_nodes[i]).destroy();
        free(m_nodes);
    }
}

//===========================================================================
bool MetaNode::insert(unsigned value) {
    return false;
}


/****************************************************************************
*
*   Helpers
*
***/

//===========================================================================
static NodeBase & nodeType(UnsignedSet::Node & node) {
    switch (node.m_type) {
    case kEmpty: return static_cast<EmptyNode &>(node);
    case kFull: return static_cast<FullNode &>(node);
    case kVector: return static_cast<VectorNode &>(node);
    case kBitmap: break;
    case kMeta: return static_cast<MetaNode &>(node);
    }

    logMsgCrash() << "invalid node type: " << node.m_type;
    return static_cast<NodeBase &>(node);
}

//===========================================================================
static const NodeBase & nodeType(const UnsignedSet::Node & node) {
    return nodeType(const_cast<UnsignedSet::Node &>(node));
}


/****************************************************************************
*
*   UnsignedSet::Node
*
***/

//===========================================================================
UnsignedSet::Node::Node()
    : m_type{kEmpty}
    , m_depth{0}
    , m_base{0}
    , m_numBytes{0}
    , m_numValues{0}
    , m_values{nullptr}
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
bool UnsignedSet::operator==(const UnsignedSet & right) const;

//===========================================================================
bool UnsignedSet::empty() const {
    return m_node.m_type == kEmpty;
}

//===========================================================================
size_t UnsignedSet::size() const {
    return nodeType(m_node).size();
}

//===========================================================================
void UnsignedSet::clear() {
    nodeType(m_node).destroy();
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
