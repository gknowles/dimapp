// xparser.cpp - dim xml
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
    XElem * m_firstElem{nullptr};
    XAttr * m_firstAttr{nullptr};
};

} // namespace


/****************************************************************************
*
*   Dom parser
*
***/

//===========================================================================
XParser::XParser() 
    : m_parser(*this)
{}

//===========================================================================
void XParser::clear() {
}

//===========================================================================
XElem * XParser::parse(char src[]) {
    clear();
    m_parser.parse(src);
    return m_root;
}

//===========================================================================
XElem * XParser::setRoot(const char elemName[], const char text[]) {
    assert(elemName);
    XElemInfo * elem = heap().emplace<XElemInfo>();
    elem->m_name = elemName;
    elem->m_value = text ? text : "";
    m_root = elem;
    return elem;
}

//===========================================================================
bool XParser::StartDoc(XStreamParser & parser) {
    return true;
}

//===========================================================================
bool XParser::EndDoc(XStreamParser & parser) {
    return true;
}

//===========================================================================
bool XParser::StartElem(
    XStreamParser & parser, const char name[], size_t nameLen) {
    return true;
}

//===========================================================================
bool XParser::EndElem(XStreamParser & parser) {
    return true;
}

//===========================================================================
bool XParser::Attr(
    XStreamParser & parser,
    const char name[],
    size_t nameLen,
    const char value[],
    size_t valueLen) {
    return true;
    }

//===========================================================================
bool
XParser::Text(XStreamParser & parser, const char value[], size_t valueLen) {
    return true;
}


/****************************************************************************
*
*   Public API
*
***/

