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
    size_t valueLen{0};
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
    bool startDoc() override;
    bool endDoc() override;
    bool startElem(const char name[], size_t nameLen) override;
    bool endElem() override;
    bool
    attr(const char name[], size_t nameLen, const char value[], size_t valueLen)
        override;
    bool text(const char value[], size_t valueLen) override;

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
bool ParserNotify::startDoc() {
    m_doc.clear();
    m_curElem = nullptr;
    return true;
}

//===========================================================================
bool ParserNotify::endDoc() {
    return true;
}

//===========================================================================
bool ParserNotify::startElem(const char name[], size_t nameLen) {
    const_cast<char *>(name)[nameLen] = 0;
    if (!m_curElem) {
        m_curElem = static_cast<XElemInfo*>(m_doc.setRoot(name));
    } else {
        m_curElem = static_cast<XElemInfo*>(m_doc.addElem(m_curElem, name));
    }
    return true;
}

//===========================================================================
bool ParserNotify::endElem() {
    m_doc.normalizeText(m_curElem);
    m_curElem = m_curElem->parent;
    return true;
}

//===========================================================================
bool ParserNotify::attr(
    const char name[], size_t nameLen, const char value[], size_t valueLen) {
    const_cast<char *>(name)[nameLen] = 0;
    const_cast<char *>(value)[valueLen] = 0;
    m_doc.addAttr(m_curElem, name, value);
    return true;
}

//===========================================================================
bool ParserNotify::text(const char value[], size_t valueLen) {
    const_cast<char *>(value)[valueLen] = 0;
    m_doc.addText(m_curElem, value);
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
static void linkNode(XElemInfo * parent, XNodeInfo * node) {
    node->parent = parent;
    if (auto first = parent->firstElem) {
        node->prev = first->prev;
        node->next = first;
        first->prev = node;
        node->prev->next = node;
    } else {
        parent->firstElem = node;
        node->prev = node->next = node;
    }
}

//===========================================================================
XNode *
XDocument::addElem(XNode * parent, const char name[], const char text[]) {
    assert(parent);
    assert(name);
    auto * elem = heap().emplace<XElemInfo>(name, text ? text : "");
    auto * p = static_cast<XElemInfo *>(parent);
    linkNode(p, elem);
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

//===========================================================================
XNode *
XDocument::addText(XNode * parent, const char text[]) {
    assert(parent);
    assert(text);
    auto * node = heap().emplace<XOtherInfo>("", text);
    node->type = XType::kText;
    auto * p = static_cast<XElemInfo *>(parent);
    p->valueLen += strlen(text);
    linkNode(p, node);
    if (!*p->value)
        *const_cast<const char **>(&p->value) = text;
    return node;
}

//===========================================================================
void XDocument::normalizeText(XNode * node) {
    auto * elem = static_cast<XElemInfo *>(node);
    if (elem->valueLen == -1)
        return;



    elem->valueLen = size_t(-1);
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
    if (!inode) 
        return nullptr;
    auto node = static_cast<XNodeInfo *>(inode);
    auto first = node->parent->firstElem;
    while (node->next != first) {
        node = static_cast<XNodeInfo *>(node->next);
        if (!name || strcmp(node->name, name) == 0)
            return node;
    }
    return nullptr;
}

//===========================================================================
XNode * Dim::prevSibling(XNode * inode, const char name[]) {
    if (!inode)
        return nullptr;
    auto node = static_cast<XNodeInfo *>(inode);
    auto first = node->parent->firstElem;
    while (node != first) {
        node = static_cast<XNodeInfo *>(node->prev);
        if (!name || strcmp(node->name, name) == 0)
            return node;
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
XElemRange<XNode> Dim::elems(XNode * inode, const char name[]) {
    if (type(inode) != XType::kElement) 
        return {{nullptr, nullptr}};

    auto elem = static_cast<XElemInfo *>(inode);
    elem = static_cast<XElemInfo *>(elem->firstElem);
    return {{elem, name}};
}

//===========================================================================
XElemRange<const XNode> Dim::elems(const XNode * inode, const char name[]) {
    if (type(inode) != XType::kElement) 
        return {{nullptr, nullptr}};

    auto elem = static_cast<const XElemInfo *>(inode);
    elem = static_cast<XElemInfo *>(elem->firstElem);
    return {{elem, name}};
}

//===========================================================================
// XAttrRange
//===========================================================================
XAttrRange<XAttr> Dim::attrs(XNode * inode) {
    if (type(inode) != XType::kElement) 
        return {{nullptr}};

    auto elem = static_cast<XElemInfo *>(inode);
    return {{elem->firstAttr}};
}

//===========================================================================
XAttrRange<const XAttr> Dim::attrs(const XNode * inode) {
    if (type(inode) != XType::kElement) 
        return {{nullptr}};

    auto elem = static_cast<const XElemInfo *>(inode);
    return {{elem->firstAttr}};
}

//===========================================================================
static XAttr * next(XAttr * iattr) {
    if (!iattr)
        return nullptr;
    auto attr = static_cast<XAttrInfo *>(iattr);
    auto first = attr->parent->firstAttr;
    if (attr->next == first)
        return nullptr;
    return static_cast<XAttrInfo *>(attr->next);
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
