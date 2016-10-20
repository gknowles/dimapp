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
XElem * XParser::setRoot(const char elemName[], const char text[]) {
    assert(elemName);
    XElemInfo * elem = heap().emplace<XElemInfo>();
    elem->m_name = elemName;
    elem->m_value = text ? text : "";
    return elem;
}


/****************************************************************************
*
*   Public API
*
***/

