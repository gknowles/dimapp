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

struct XAttrInfo : XAttr {
    XElemInfo * parent{nullptr};
    XAttrInfo * prev{nullptr};
    XAttrInfo * next{nullptr};

    XAttrInfo(const char name[], const char value[])
        : XAttr{name, value} {}
};

struct XNodeInfo : XNode {
    XElemInfo * parent{nullptr};
    XNodeInfo * prev{nullptr};
    XNodeInfo * next{nullptr};

    XNodeInfo(const char name[], const char value[])
        : XNode{name, value} {}
};
struct XElemInfo : XNodeInfo {
    XNodeInfo * firstElem{nullptr};
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
    bool attr(
        const char name[],
        size_t nameLen,
        const char value[],
        size_t valueLen) override;
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
        m_curElem = static_cast<XElemInfo *>(m_doc.setRoot(name));
    } else {
        m_curElem = static_cast<XElemInfo *>(m_doc.addElem(m_curElem, name));
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
    const char name[],
    size_t nameLen,
    const char value[],
    size_t valueLen) {
    const_cast<char *>(name)[nameLen] = 0;
    const_cast<char *>(value)[valueLen] = 0;
    if (m_curElem)
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
    if (!parser.parse(src)) {
        m_errmsg = parser.errmsg();
        m_errpos = parser.errpos();
        return nullptr;
    }
    return m_root;
}

//===========================================================================
XNode * XDocument::setRoot(const char elemName[], const char text[]) {
    assert(elemName);
    auto * ri = heap().emplace<XElemRootInfo>(elemName, text ? text : "");
    ri->document = this;
    ri->prev = ri->next = ri;
    m_root = ri;
    return ri;
}

//===========================================================================
static void linkNode(XElemInfo * parent, XNodeInfo * ni) {
    ni->parent = parent;
    if (auto first = parent->firstElem) {
        ni->prev = first->prev;
        ni->next = first;
        first->prev = ni;
        ni->prev->next = ni;
    } else {
        parent->firstElem = ni;
        ni->prev = ni->next = ni;
    }
}

//===========================================================================
XNode *
XDocument::addElem(XNode * parent, const char name[], const char text[]) {
    assert(parent);
    assert(name);
    auto * ni = heap().emplace<XElemInfo>(name, text ? text : "");
    auto * p = static_cast<XElemInfo *>(parent);
    linkNode(p, ni);
    return ni;
}

//===========================================================================
XAttr *
XDocument::addAttr(XNode * elem, const char name[], const char text[]) {
    assert(elem);
    assert(name);
    assert(text);
    auto * ai = heap().emplace<XAttrInfo>(name, text);
    auto * p = static_cast<XElemInfo *>(elem);
    ai->parent = p;
    if (auto first = p->firstAttr) {
        ai->prev = first->prev;
        ai->next = first;
        first->prev = ai;
        ai->prev->next = ai;
    } else {
        p->firstAttr = ai;
        ai->prev = ai->next = ai;
    }
    return ai;
}

//===========================================================================
XNode * XDocument::addText(XNode * parent, const char text[]) {
    assert(parent);
    assert(text);
    auto * ni = heap().emplace<XOtherInfo>("", text);
    ni->type = XType::kText;
    auto * p = static_cast<XElemInfo *>(parent);
    p->valueLen += strlen(text);
    linkNode(p, ni);
    if (!*p->value)
        *const_cast<const char **>(&p->value) = text;
    return ni;
}

//===========================================================================
void XDocument::normalizeText(XNode * node) {
    assert(nodeType(node) == XType::kElement);
    auto * ei = static_cast<XElemInfo *>(node);
    if (ei->valueLen == -1)
        return;

    auto firstNode = firstChild(ei, nullptr, XType::kText);
    XNode * prev;
    if (!firstNode)
        goto NO_TEXT;
    const char * firstChar = firstNode->value;
    // skip leading whitespace
    for (;;) {
        switch (*firstChar) {
        case ' ':
        case '\t':
        case '\n':
        case '\r': firstChar += 1; continue;
        case 0:
            ei->valueLen -= (firstChar - firstNode->value);
            prev = firstNode;
            firstNode = nextSibling(firstNode, nullptr, XType::kText);
            unlinkNode(prev);
            if (!firstNode) {
                *const_cast<const char **>(&ei->value) = "";
                goto NO_TEXT;
            }
            firstChar = firstNode->value;
            continue;
        }
        break;
    }
    ei->valueLen -= (firstChar - firstNode->value);

    // skip trailing whitespace
    //  - since the leading whitespace stopped at a non-whitespace, we know
    //    we'll also find a non-whitespace as we skip trailing.
    auto lastNode = lastChild(ei, nullptr, XType::kText);
    const char * lastChar = lastNode->value + strlen(lastNode->value);
    for (;;) {
        if (lastChar == lastNode->value) {
            prev = lastNode;
            lastNode = prevSibling(lastNode, nullptr, XType::kText);
            unlinkNode(prev);
            lastChar = lastNode->value + strlen(lastNode->value);
        }
        switch (lastChar[-1]) {
        case ' ':
        case '\t':
        case '\n':
        case '\r':
            lastChar -= 1;
            ei->valueLen -= 1;
            continue;
        }
        break;
    }
    if (firstNode == lastNode) {
        *const_cast<const char **>(&ei->value) = firstChar;
        *const_cast<char *>(lastChar) = 0;
    } else {
        char * ptr = heap().alloc(ei->valueLen + 1);
        *const_cast<const char **>(&ei->value) = ptr;
        for (;;) {
            *ptr++ = *firstChar++;
            if (firstChar == lastChar)
                break;
            while (!*firstChar) {
                prev = firstNode;
                firstNode = nextSibling(firstNode, nullptr, XType::kText);
                unlinkNode(prev);
                firstChar = firstNode->value;
            }
        }
        assert(ptr == ei->value + ei->valueLen);
        *ptr = 0;
    }
    unlinkNode(firstNode);

NO_TEXT:
    ei->valueLen = size_t(-1);
}


/****************************************************************************
*
*   Public API
*
***/

//===========================================================================
static bool matchNode(const XNode * node, const char name[], XType type) {
    if (type != XType::kInvalid && type != nodeType(node))
        return false;
    if (name && strcmp(node->name, name) != 0)
        return false;
    return true;
}

//===========================================================================
XType Dim::nodeType(const XNode * node) {
    if (!node)
        return XType::kInvalid;
    if (*node->name)
        return XType::kElement;
    return static_cast<const XOtherInfo *>(node)->type;
}

//===========================================================================
void Dim::unlinkAttr(XAttr * attr) {
    if (!attr)
        return;
    auto ai = static_cast<XAttrInfo *>(attr);
    XElemInfo * parent = ai->parent;
    ai->parent = nullptr;
    if (ai == parent->firstAttr) {
        if (ai == ai->next) {
            parent->firstAttr = nullptr;
            return;
        } else {
            parent->firstAttr = ai->next;
        }
    }
    ai->next->prev = ai->prev;
    ai->prev->next = ai->next;
}

//===========================================================================
void Dim::unlinkNode(XNode * node) {
    if (!node)
        return;
    auto ni = static_cast<XNodeInfo *>(node);
    XElemInfo * parent = ni->parent;
    ni->parent = nullptr;
    if (ni == parent->firstElem) {
        if (ni == ni->next) {
            parent->firstElem = nullptr;
            return;
        } else {
            parent->firstElem = ni->next;
        }
    }
    ni->next->prev = ni->prev;
    ni->prev->next = ni->next;
}

//===========================================================================
XNode * Dim::firstChild(XNode * elem, const char name[], XType type) {
    if (nodeType(elem) != XType::kElement)
        return nullptr;

    auto ei = static_cast<XElemInfo *>(elem);
    elem = static_cast<XElemInfo *>(ei->firstElem);
    if (!matchNode(elem, name, type))
        elem = nextSibling(elem, name, type);
    return elem;
}

//===========================================================================
XNode * Dim::lastChild(XNode * elem, const char name[], XType type) {
    if (nodeType(elem) != XType::kElement)
        return nullptr;

    auto ei = static_cast<XElemInfo *>(elem);
    elem = static_cast<XElemInfo *>(ei->firstElem->prev);
    if (!matchNode(elem, name, type))
        elem = prevSibling(elem, name, type);
    return elem;
}

//===========================================================================
const XNode *
Dim::firstChild(const XNode * elem, const char name[], XType type) {
    return firstChild(const_cast<XNode *>(elem), name, type);
}

//===========================================================================
const XNode *
Dim::lastChild(const XNode * elem, const char name[], XType type) {
    return lastChild(const_cast<XNode *>(elem), name, type);
}

//===========================================================================
XNode * Dim::nextSibling(XNode * node, const char name[], XType type) {
    if (!node)
        return nullptr;
    auto ni = static_cast<XNodeInfo *>(node);
    auto first = ni->parent->firstElem;
    while (ni->next != first) {
        ni = ni->next;
        if (matchNode(ni, name, type))
            return ni;
    }
    return nullptr;
}

//===========================================================================
XNode * Dim::prevSibling(XNode * node, const char name[], XType type) {
    if (!node)
        return nullptr;
    auto ni = static_cast<XNodeInfo *>(node);
    auto first = ni->parent->firstElem;
    while (ni != first) {
        ni = ni->prev;
        if (matchNode(ni, name, type))
            return ni;
    }
    return nullptr;
}

//===========================================================================
const XNode *
Dim::nextSibling(const XNode * node, const char name[], XType type) {
    return nextSibling(const_cast<XNode *>(node), name, type);
}

//===========================================================================
const XNode *
Dim::prevSibling(const XNode * node, const char name[], XType type) {
    return prevSibling(const_cast<XNode *>(node), name, type);
}

//===========================================================================
// XNodeRange
//===========================================================================
XNodeRange<XNode> Dim::elems(XNode * node, const char name[]) {
    node = firstChild(node, name);
    XType type = name ? XType::kInvalid : XType::kElement;
    return {{node, type, name}};
}

//===========================================================================
XNodeRange<const XNode> Dim::elems(const XNode * node, const char name[]) {
    node = firstChild(node, name);
    XType type = name ? XType::kInvalid : XType::kElement;
    return {{node, type, name}};
}

//===========================================================================
XNodeRange<XNode> Dim::nodes(XNode * node, XType type) {
    node = firstChild(node, nullptr, type);
    return {{node, type, nullptr}};
}

//===========================================================================
XNodeRange<const XNode> Dim::nodes(const XNode * node, XType type) {
    node = firstChild(node, nullptr, type);
    return {{node, type, nullptr}};
}

//===========================================================================
// XAttrRange
//===========================================================================
XAttrRange<XAttr> Dim::attrs(XNode * node) {
    if (nodeType(node) != XType::kElement)
        return {{nullptr}};

    auto ei = static_cast<XElemInfo *>(node);
    return {{ei->firstAttr}};
}

//===========================================================================
XAttrRange<const XAttr> Dim::attrs(const XNode * node) {
    if (nodeType(node) != XType::kElement)
        return {{nullptr}};

    auto ei = static_cast<const XElemInfo *>(node);
    return {{ei->firstAttr}};
}

//===========================================================================
static XAttr * next(XAttr * attr) {
    if (!attr)
        return nullptr;
    auto ai = static_cast<XAttrInfo *>(attr);
    auto first = ai->parent->firstAttr;
    if (ai->next == first)
        return nullptr;
    return ai->next;
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