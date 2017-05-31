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
struct NodeBase : UnsignedSet::Node {
    virtual size_t size() const = 0;
    virtual void clear() = 0;
};

} // namespace


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
    void clear() override;
};
} // namespace

//===========================================================================
size_t EmptyNode::size() const {
    return 0;
}

//===========================================================================
void EmptyNode::clear() 
{}


/****************************************************************************
*
*   MetaNode
*
***/

namespace {
struct MetaNode : NodeBase {
    size_t size() const override;
    void clear() override;
};
} // namespace

//===========================================================================
size_t MetaNode::size() const {
    size_t num = 0;
    for (unsigned i = 0; i < bytes / sizeof(*nodes); ++i)
        num += nodeType(nodes[i]).size();
    return num;
}

//===========================================================================
void MetaNode::clear() {
    for (unsigned i = 0; i < bytes / sizeof(*nodes); ++i)
        nodeType(nodes[i]).clear();
    free(nodes);
    //setType(kEmpty);
}


/****************************************************************************
*
*   Helpers
*
***/

//===========================================================================
static NodeBase & nodeType(UnsignedSet::Node & node) {
    switch (node.type) {
    case UnsignedSet::kEmpty: return static_cast<EmptyNode &>(node);
    case UnsignedSet::kFull: break;
    case UnsignedSet::kVector: break;
    case UnsignedSet::kBitmap: break;
    case UnsignedSet::kMeta: return static_cast<MetaNode &>(node);
    }

    logMsgCrash() << "invalid node type: " << node.type;
    return static_cast<NodeBase &>(node);
}

//===========================================================================
static const NodeBase & nodeType(const UnsignedSet::Node & node) {
    return nodeType(const_cast<UnsignedSet::Node &>(node));
}


/****************************************************************************
*
*   UnsignedSet
*
***/

//===========================================================================
UnsignedSet::UnsignedSet() {
    m_node.type = kEmpty;
    m_node.height = 0;
    m_node.base = 0;
    m_node.values = nullptr;
}

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
    return m_node.type == kEmpty;
}

//===========================================================================
size_t UnsignedSet::size() const {
    return nodeType(m_node).size();
}

//===========================================================================
void UnsignedSet::clear() {
    nodeType(m_node).clear();
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
