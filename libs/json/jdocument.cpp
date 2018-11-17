// Copyright Glen Knowles 2018.
// Distributed under the Boost Software License, Version 1.0.
//
// jdocument.cpp - dim json
#include "pch.h"
#pragma hdrstop

using namespace std;
using namespace Dim;


/****************************************************************************
*
*   Private
*
***/

namespace {

class StreamNotify : public IJsonStreamNotify {
public:
    explicit StreamNotify(JDocument * doc);

    // Inherited via IJsonStreamNotify
    bool startDoc() override;
    bool endDoc() override;
    bool startArray(string_view name) override;
    bool endArray() override;
    bool startObject(string_view name) override;
    bool endObject() override;
    bool value(string_view name, string_view val) override;
    bool value(string_view name, double val) override;
    bool value(string_view name, bool val) override;
    bool value(string_view name, nullptr_t) override;

private:
    JDocument & m_doc;
    JNode * m_curNode{};
};

} // namespace


/****************************************************************************
*
*   StreamNotify
*
***/

//===========================================================================
StreamNotify::StreamNotify(JDocument * doc)
    : m_doc(*doc)
{}

//===========================================================================
bool StreamNotify::startDoc() {
    m_curNode = nullptr;
    return true;
}

//===========================================================================
bool StreamNotify::endDoc() {
    return true;
}

//===========================================================================
bool StreamNotify::startArray(string_view name) {
    m_curNode = m_doc.addArray(m_curNode, name);
    return true;
}

//===========================================================================
bool StreamNotify::endArray() {
    m_curNode = parent(m_curNode);
    return true;
}

//===========================================================================
bool StreamNotify::startObject(string_view name) {
    m_curNode = m_doc.addObject(m_curNode, name);
    return true;
}

//===========================================================================
bool StreamNotify::endObject() {
    m_curNode = parent(m_curNode);
    return true;
}

//===========================================================================
bool StreamNotify::value(string_view name, string_view val) {
    m_doc.addValue(m_curNode, val, name);
    return true;
}

//===========================================================================
bool StreamNotify::value(string_view name, double val) {
    m_doc.addValue(m_curNode, val, name);
    return true;
}

//===========================================================================
bool StreamNotify::value(string_view name, bool val) {
    m_doc.addValue(m_curNode, val, name);
    return true;
}

//===========================================================================
bool StreamNotify::value(string_view name, nullptr_t) {
    m_doc.addValue(m_curNode, nullptr, name);
    return true;
}


/****************************************************************************
*
*   JNode
*
***/

//===========================================================================
JNode::JNode(JType type)
    : type{type}
{
    if (type == JType::kString) {
        new(&sval) string_view;
    } else if (type == JType::kArray || type == JType::kObject) {
        new(&vals) List<JNode>;
    }
}

//===========================================================================
JNode::~JNode() {
    assert(!"~JNode() unimplemented");
}


/****************************************************************************
*
*   JNodeInfo
*
***/

namespace {

struct JNodeInfo : JNode {
    JNodeInfo * parent;
    union {
        string_view name;       // set if parent is an object
        JDocument * document;   // set if !parent
    };

    JNodeInfo(
        JDocument * doc,
        JNode * parent,
        string_view name,
        JNode::JType type
    );
};

} // namespace

//===========================================================================
JNodeInfo::JNodeInfo(
    JDocument * doc,
    JNode * parent,
    string_view name,
    JNode::JType type
)
    : JNode{type}
    , parent{static_cast<JNodeInfo *>(parent)}
    , name{name}
{
    if (!parent) {
        document = doc;
        doc->setRoot(this);
    } else {
        parent->vals.link(this);
    }
}


/****************************************************************************
*
*   JDocument
*
***/

//===========================================================================
void JDocument::clear() {
    m_root = nullptr;
    m_heap.clear();
}

//===========================================================================
JNode * JDocument::parse(char src[], string_view filename) {
    clear();
    if (!filename.empty())
        m_filename = m_heap.strdup(filename);
    StreamNotify notify(this);
    JsonStream parser(&notify);
    if (!parser.parseMore(src)) {
        m_errmsg = parser.errmsg();
        m_errpos = parser.errpos();
        return nullptr;
    }
    return m_root;
}

//===========================================================================
void JDocument::setRoot(JNode * root) {
    m_root = root;
}

//===========================================================================
JNode * JDocument::addArray(JNode * parent, string_view name) {
    auto node = m_heap.emplace<JNodeInfo>(this, parent, name, JNode::kArray);
    return node;
}

//===========================================================================
JNode * JDocument::addObject(JNode * parent, string_view name) {
    auto node = m_heap.emplace<JNodeInfo>(this, parent, name, JNode::kObject);
    return node;
}

//===========================================================================
JNode * JDocument::addValue(
    JNode * parent,
    string_view val,
    string_view name
) {
    auto node = m_heap.emplace<JNodeInfo>(this, parent, name, JNode::kString);
    node->sval = val;
    return node;
}

//===========================================================================
JNode * JDocument::addValue(JNode * parent, double val, string_view name) {
    auto node = m_heap.emplace<JNodeInfo>(this, parent, name, JNode::kNumber);
    node->nval = val;
    return node;
}

//===========================================================================
JNode * JDocument::addValue(JNode * parent, bool val, string_view name) {
    auto node = m_heap.emplace<JNodeInfo>(this, parent, name, JNode::kBoolean);
    node->bval = val;
    return node;
}

//===========================================================================
JNode * JDocument::addValue(JNode * parent, nullptr_t, string_view name) {
    auto node = m_heap.emplace<JNodeInfo>(this, parent, name, JNode::kNull);
    return node;
}


/****************************************************************************
*
*   JNodeIterator
*
***/

//===========================================================================
JNodeIterator::JNodeIterator()
    : ForwardListIterator{nullptr}
{}

//===========================================================================
JNodeIterator::JNodeIterator(JNode * node)
    : ForwardListIterator{nullptr}
{
    if (node->type == JNode::kArray || node->type == JNode::kObject)
        m_current = node->vals.front();
}

//===========================================================================
JNodeIterator JNodeIterator::operator++() {
    auto ni = static_cast<JNodeInfo const *>(m_current);
    m_current = ni->parent->vals.next(ni);
    return *this;
}

//===========================================================================
// free
JNodeIterator Dim::nodes(JNode * node) {
    return JNodeIterator{node};
}


/****************************************************************************
*
*   Public API
*
***/

IJBuilder & Dim::operator<<(IJBuilder & out, JNode const & node) {
    auto & ni = static_cast<JNodeInfo const &>(node);
    if (!ni.name.empty())
        out.member(ni.name);
    switch (node.type) {
    case JNode::kInvalid:
        out.value("INVALID");
        assert(!"Writing invalid JSON");
        break;
    case JNode::kObject:
        out.object();
        for (auto & val : node.vals)
            out << val;
        out.end();
        break;
    case JNode::kArray:
        out.array();
        for (auto & val : node.vals)
            out << val;
        out.end();
        break;
    case JNode::kString:
        out.value(node.sval);
        break;
    case JNode::kNumber:
        out.value(node.nval);
        break;
    case JNode::kBoolean:
        out.value(node.bval);
        break;
    case JNode::kNull:
        out.value(nullptr);
        break;
    }
    return out;
}

//===========================================================================
JNode::JType Dim::nodeType(JNode const * node) {
    return node->type;
}

//===========================================================================
string_view Dim::nodeName(JNode const * node) {
    return static_cast<JNodeInfo const *>(node)->name;
}

//===========================================================================
JDocument * Dim::document(JNode * node) {
    while (auto next = parent(node))
        node = next;
    return static_cast<JNodeInfo *>(node)->document;
}

//===========================================================================
JNode * Dim::parent(JNode * node) {
    return static_cast<JNodeInfo *>(node)->parent;
}

//===========================================================================
void Dim::unlinkNode(JNode * node) {
    parent(node)->vals.unlink(node);
}
