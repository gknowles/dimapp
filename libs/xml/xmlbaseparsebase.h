// Copyright Glen Knowles 2016 - 2017.
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
    const char * m_base{nullptr};
    char * m_cur{nullptr};
    const char * m_attr{nullptr};
    size_t m_attrLen{0};
    unsigned m_char{0};
};

//===========================================================================
inline XmlBaseParserBase::XmlBaseParserBase(XStreamParser & parser)
    : m_parser(parser)
    , m_notify(parser.notify())
{}

} // namespace
