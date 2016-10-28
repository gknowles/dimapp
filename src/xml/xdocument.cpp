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

struct XElemInfo : XElem {
    using XElem::XElem;
    XElem * m_firstElem{nullptr};
    XAttr * m_firstAttr{nullptr};
};
struct XElemRootInfo : XElemInfo {
    XElemRootInfo()
        : XElemInfo(this) {}
    XDocument * m_document{nullptr};
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
    XElem * m_curElem{nullptr};
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
        m_curElem = m_doc.setRoot(name);
    } else {
        m_curElem = m_doc.addElem(m_curElem, name);
    }
    return true;
}

//===========================================================================
bool ParserNotify::EndElem() {
    m_curElem = m_curElem->m_parent;
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
    m_curElem->m_value = value;
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
XElem * XDocument::parse(char src[]) {
    clear();
    ParserNotify notify(*this);
    XStreamParser parser(notify);
    parser.parse(src);
    return m_root;
}

//===========================================================================
XElem * XDocument::setRoot(const char elemName[], const char text[]) {
    assert(elemName);
    auto * elem = heap().emplace<XElemRootInfo>();
    elem->m_document = this;
    elem->m_prev = elem->m_next = elem;
    elem->m_name = elemName;
    elem->m_value = text ? text : "";
    m_root = elem;
    return elem;
}

//===========================================================================
XElem *
XDocument::addElem(XElem * parent, const char name[], const char text[]) {
    assert(parent);
    assert(name);
    auto * elem = heap().emplace<XElemInfo>(parent);
    elem->m_name = name;
    elem->m_value = text ? text : "";
    auto * p = static_cast<XElemInfo *>(parent);
    if (auto first = p->m_firstElem) {
        elem->m_prev = first->m_prev;
        elem->m_next = first;
        first->m_prev = elem;
        elem->m_prev->m_next = elem;
    } else {
        p->m_firstElem = elem;
        elem->m_prev = elem->m_next = elem;
    }
    return elem;
}

//===========================================================================
XAttr * XDocument::addAttr(XElem * elem, const char name[], const char text[]) {
    assert(elem);
    assert(name);
    assert(text);
    auto * attr = heap().emplace<XAttr>(elem);
    attr->m_name = name;
    attr->m_value = text;
    auto * p = static_cast<XElemInfo *>(elem);
    if (auto first = p->m_firstAttr) {
        attr->m_prev = first->m_prev;
        attr->m_next = first;
        first->m_prev = attr;
        attr->m_prev->m_next = attr;
    } else {
        p->m_firstAttr = attr;
        attr->m_prev = attr->m_next = attr;
    }
    return attr;
}


/****************************************************************************
*
*   Public API
*
***/

//===========================================================================
XElem * Dim::nextSibling(XElem * elem, const char name[]) {
    if (!elem)
        return nullptr;
    auto first = static_cast<XElemInfo *>(elem->m_parent)->m_firstElem;
    while (elem->m_next != first) {
        elem = elem->m_next;
        if (!name || strcmp(elem->m_name, name) == 0)
            return elem;
    }
    return nullptr;
}

//===========================================================================
XElem * Dim::prevSibling(XElem * elem, const char name[]) {
    if (!elem)
        return nullptr;
    auto first = static_cast<XElemInfo *>(elem->m_parent)->m_firstElem;
    while (elem != first) {
        elem = elem->m_prev;
        if (!name || strcmp(elem->m_name, name) == 0)
            return elem;
    }
    return nullptr;
}

//===========================================================================
const XElem * Dim::nextSibling(const XElem * elem, const char name[]) {
    return nextSibling(const_cast<XElem *>(elem), name);
}

//===========================================================================
const XElem * Dim::prevSibling(const XElem * elem, const char name[]) {
    return prevSibling(const_cast<XElem *>(elem), name);
}

//===========================================================================
XAttr * Dim::next(XAttr * attr) {
    if (!attr)
        return nullptr;
    auto first = static_cast<XElemInfo *>(attr->m_parent)->m_firstAttr;
    if (attr->m_next == first)
        return nullptr;
    return attr->m_next;
}

//===========================================================================
XAttr * Dim::prev(XAttr * attr) {
    if (!attr)
        return nullptr;
    auto first = static_cast<XElemInfo *>(attr->m_parent)->m_firstAttr;
    if (attr == first)
        return nullptr;
    return attr->m_prev;
}

//===========================================================================
const XAttr * Dim::next(const XAttr * attr) {
    return next(const_cast<XAttr *>(attr));
}

//===========================================================================
const XAttr * Dim::prev(const XAttr * attr) {
    return prev(const_cast<XAttr *>(attr));
}

//===========================================================================
// XElemRange
//===========================================================================
XElemRange<XElem> Dim::elems(XElem * elem, const char name[]) {
    auto base = static_cast<XElemInfo *>(elem);
    auto first = XElem::Iterator<XElem>(base ? base->m_firstElem : base, name);
    return {first};
}

//===========================================================================
XElemRange<const XElem> Dim::elems(const XElem * elem, const char name[]) {
    auto base = static_cast<const XElemInfo *>(elem);
    auto first =
        XElem::Iterator<const XElem>(base ? base->m_firstElem : base, name);
    return {first};
}

//===========================================================================
// XAttrRange
//===========================================================================
XAttrRange<XAttr> Dim::attrs(XElem * elem) {
    auto base = static_cast<XElemInfo *>(elem);
    auto first = ForwardListIterator<XAttr>(base ? base->m_firstAttr : nullptr);
    return {first};
}

//===========================================================================
XAttrRange<const XAttr> Dim::attrs(const XElem * elem) {
    auto base = static_cast<const XElemInfo *>(elem);
    auto first =
        ForwardListIterator<const XAttr>(base ? base->m_firstAttr : nullptr);
    return {first};
}
