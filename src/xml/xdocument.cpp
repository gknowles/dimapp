// xdocument.cpp - dim xml
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

struct XElemInfo;

struct XBaseInfo {
    XElemInfo * parent{nullptr};
    XBaseInfo * prev{nullptr};
    XBaseInfo * next{nullptr};
};
struct XAttrInfo : XAttr, XBaseInfo {
    XAttrInfo(const char name[], const char value[]) : XAttr{name, value} {}
};

struct XNodeInfo : XNode, XBaseInfo {
    XNodeInfo(const char name[], const char value[]) : XNode{name, value} {}
};
struct XElemInfo : XNodeInfo {
    XBaseInfo * firstElem{nullptr};
    XAttrInfo * firstAttr{nullptr};
    using XNodeInfo::XNodeInfo;
};
struct XElemRootInfo : XElemInfo {
    XDocument * document{nullptr};
    using XElemInfo::XElemInfo;
};

// struct for node types that are neither attributes nor elements (text, 
// comment, processing instruction, etc)
struct XOtherInfo : XNodeInfo {
    XType type;
    using XNodeInfo::XNodeInfo;
};

class ParserNotify : public IXStreamParserNotify {
public:
    ParserNotify(XDocument & doc);

    // IXStreamParserNotify
    bool StartDoc() override;
    bool EndDoc() override;
    bool StartElem(const char name[], size_t nameLen) override;
    bool EndElem() override;
    bool
    Attr(const char name[], size_t nameLen, const char value[], size_t valueLen)
        override;
    bool Text(const char value[], size_t valueLen) override;

private:
    XDocument & m_doc;
    XElemInfo * m_curElem{nullptr};
};

} // namespace


/****************************************************************************
*
*   Dom parser notify
*
***/

//===========================================================================
ParserNotify::ParserNotify(XDocument & doc)
    : m_doc(doc) {}

//===========================================================================
bool ParserNotify::StartDoc() {
    m_doc.clear();
    m_curElem = nullptr;
    return true;
}

//===========================================================================
bool ParserNotify::EndDoc() {
    return true;
}

//===========================================================================
bool ParserNotify::StartElem(const char name[], size_t nameLen) {
    const_cast<char *>(name)[nameLen] = 0;
    if (!m_curElem) {
        m_curElem = static_cast<XElemInfo*>(m_doc.setRoot(name));
    } else {
        m_curElem = static_cast<XElemInfo*>(m_doc.addElem(m_curElem, name));
    }
    return true;
}

//===========================================================================
bool ParserNotify::EndElem() {
    m_curElem = m_curElem->parent;
    return true;
}

//===========================================================================
bool ParserNotify::Attr(
    const char name[], size_t nameLen, const char value[], size_t valueLen) {
    const_cast<char *>(name)[nameLen] = 0;
    const_cast<char *>(value)[valueLen] = 0;
    m_doc.addAttr(m_curElem, name, value);
    return true;
}

//===========================================================================
bool ParserNotify::Text(const char value[], size_t valueLen) {
    const_cast<char *>(value)[valueLen] = 0;
    *const_cast<const char **>(&m_curElem->value) = value;
    return true;
}


/****************************************************************************
*
*   Xml document
*
***/

//===========================================================================
XDocument::XDocument() {}

//===========================================================================
void XDocument::clear() {
    m_root = nullptr;
    m_heap.clear();
}

//===========================================================================
XNode * XDocument::parse(char src[]) {
    clear();
    ParserNotify notify(*this);
    XStreamParser parser(notify);
    parser.parse(src);
    return m_root;
}

//===========================================================================
XNode * XDocument::setRoot(const char elemName[], const char text[]) {
    assert(elemName);
    auto * elem = heap().emplace<XElemRootInfo>(elemName, text ? text : "");
    elem->document = this;
    elem->prev = elem->next = elem;
    m_root = elem;
    return elem;
}

//===========================================================================
XNode *
XDocument::addElem(XNode * parent, const char name[], const char text[]) {
    assert(parent);
    assert(name);
    auto * elem = heap().emplace<XElemInfo>(name, text ? text : "");
    auto * p = static_cast<XElemInfo *>(parent);
    elem->parent = p;
    if (auto first = p->firstElem) {
        elem->prev = first->prev;
        elem->next = first;
        first->prev = elem;
        elem->prev->next = elem;
    } else {
        p->firstElem = elem;
        elem->prev = elem->next = elem;
    }
    return elem;
}

//===========================================================================
XAttr * XDocument::addAttr(XNode * elem, const char name[], const char text[]) {
    assert(elem);
    assert(name);
    assert(text);
    auto * attr = heap().emplace<XAttrInfo>(name, text);
    auto * p = static_cast<XElemInfo *>(elem);
    attr->parent = p;
    if (auto first = p->firstAttr) {
        attr->prev = first->prev;
        attr->next = first;
        first->prev = attr;
        attr->prev->next = attr;
    } else {
        p->firstAttr = attr;
        attr->prev = attr->next = attr;
    }
    return attr;
}


/****************************************************************************
*
*   Public API
*
***/

//===========================================================================
XType Dim::type(const XNode * node) {
    if (!node)
        return XType::kInvalid;
    if (*node->name)
        return XType::kElement;
    return static_cast<const XOtherInfo *>(node)->type;
}

//===========================================================================
XNode * Dim::nextSibling(XNode * inode, const char name[]) {
    if (inode) {
        auto node = static_cast<XNodeInfo *>(inode);
        auto first = node->parent->firstElem;
        while (node->next != first) {
            node = static_cast<XNodeInfo *>(node->next);
            if (!name || strcmp(node->name, name) == 0)
                return node;
        }
    }
    return nullptr;
}

//===========================================================================
XNode * Dim::prevSibling(XNode * elem, const char name[]) {
    if (!elem)
        return nullptr;
    auto first = static_cast<XElemInfo *>(elem->m_parent)->m_firstElem;
    while (elem != first) {
        elem = static_cast<XNode *>(elem->m_prev);
        if (!name || strcmp(elem->m_name, name) == 0)
            return elem;
    }
    return nullptr;
}

//===========================================================================
const XNode * Dim::nextSibling(const XNode * elem, const char name[]) {
    return nextSibling(const_cast<XNode *>(elem), name);
}

//===========================================================================
const XNode * Dim::prevSibling(const XNode * elem, const char name[]) {
    return prevSibling(const_cast<XNode *>(elem), name);
}

//===========================================================================
// XElemRange
//===========================================================================
XElemRange<XNode> Dim::elems(XNode * elem, const char name[]) {
    auto base = static_cast<XElemInfo *>(elem);
    auto first = XNode::Iterator<XNode>(base ? base->m_firstElem : base, name);
    return {first};
}

//===========================================================================
XElemRange<const XNode> Dim::elems(const XNode * elem, const char name[]) {
    auto base = static_cast<const XElemInfo *>(elem);
    auto first =
        XNode::Iterator<const XNode>(base ? base->m_firstElem : base, name);
    return {first};
}

//===========================================================================
// XAttrRange
//===========================================================================
XAttrRange<XAttr> Dim::attrs(XNode * elem) {
    auto base = static_cast<XElemInfo *>(elem);
    auto first = ForwardListIterator<XAttr>(base ? base->m_firstAttr : nullptr);
    return {first};
}

//===========================================================================
XAttrRange<const XAttr> Dim::attrs(const XNode * elem) {
    auto base = static_cast<const XElemInfo *>(elem);
    auto first =
        ForwardListIterator<const XAttr>(base ? base->m_firstAttr : nullptr);
    return {first};
}

//===========================================================================
static XAttr * next(XAttr * attr) {
    if (!attr)
        return nullptr;
    auto first = static_cast<XElemInfo *>(attr->m_parent)->m_firstAttr;
    if (attr->m_next == first)
        return nullptr;
    return static_cast<XAttr *>(attr->m_next);
}

//===========================================================================
auto ForwardListIterator<XAttr>::operator++() -> ForwardListIterator & {
    m_current = next(m_current);
    return *this;
}

//===========================================================================
auto ForwardListIterator<const XAttr>::operator++() -> ForwardListIterator & {
    m_current = next(const_cast<XAttr *>(m_current));
    return *this;
}

template ForwardListIterator<XAttr>;
template ForwardListIterator<const XAttr>;
