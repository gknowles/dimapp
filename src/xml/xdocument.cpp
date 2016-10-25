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
    XElem * m_parent{nullptr};
    XElem * m_firstElem{nullptr};
    XAttr * m_firstAttr{nullptr};
};
struct XElemRootInfo : XElemInfo {
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
    XElemInfo * m_curElem{nullptr};
    XAttr * m_curAttr{nullptr};
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
    return true;
}

//===========================================================================
bool ParserNotify::EndDoc() {
    return true;
}

//===========================================================================
bool ParserNotify::StartElem(const char name[], size_t nameLen) {
    return true;
}

//===========================================================================
bool ParserNotify::EndElem() {
    return true;
}

//===========================================================================
bool ParserNotify::Attr(
    const char name[], size_t nameLen, const char value[], size_t valueLen) {
    return true;
}

//===========================================================================
bool ParserNotify::Text(const char value[], size_t valueLen) {
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
void XDocument::clear() {}

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
    XElemInfo * elem = heap().emplace<XElemInfo>();
    elem->m_name = elemName;
    elem->m_value = text ? text : "";
    m_root = elem;
    return elem;
}

//===========================================================================
XElem * XDocument::addElem(XElem * parent, const char name[], const char text[]) {
    assert(parent);
    assert(name);
    XElemInfo * elem = heap().emplace<XElemInfo>();
    elem->m_name = name;
    elem->m_value = text ? text : "";
    auto * p = static_cast<XElemInfo *>(parent);
    if (!p->m_firstElem) {
        p->m_firstElem = elem;
    } else {
    }
    return elem;
}

//===========================================================================
XAttr * XDocument::addAttr(XElem * elem, const char name[], const char text[]) {
    return nullptr;
}


/****************************************************************************
*
*   Public API
*
***/
