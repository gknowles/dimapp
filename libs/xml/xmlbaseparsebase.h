// Copyright Glen Knowles 2016 - 2023.
// Distributed under the Boost Software License, Version 1.0.
//
// xmlbaseparsebase.h - dim xml

#include "xml/xml.h"

namespace Dim {


/****************************************************************************
*
*   XmlBaseParserBase
*
***/

struct XmlBaseParserBase {
    XmlBaseParserBase(XStreamParser & parser);

    XStreamParser & m_parser;
    IXStreamParserNotify & m_notify;
    size_t m_lineNumber = {};
    char * m_base = {};
    char * m_cur = {};
    char * m_attr = {};
    size_t m_attrLen = {};
    unsigned m_char = {};
};

//===========================================================================
inline XmlBaseParserBase::XmlBaseParserBase(XStreamParser & parser)
    : m_parser(parser)
    , m_notify(parser.notify())
{}

} // namespace
