// Copyright Glen Knowles 2016 - 2020.
// Distributed under the Boost Software License, Version 1.0.
//
// xstreamparser.cpp - dim xml
#include "pch.h"
#pragma hdrstop

using namespace std;
using namespace Dim;


/****************************************************************************
*
*   XStreamParser
*
***/

//===========================================================================
XStreamParser::XStreamParser(IXStreamParserNotify * notify)
    : m_notify(*notify)
{}

//===========================================================================
XStreamParser::~XStreamParser()
{}

//===========================================================================
void XStreamParser::clear() {
    m_errmsg = nullptr;
}

//===========================================================================
bool XStreamParser::parseMore(char src[]) {
    m_errmsg = nullptr;
    m_base = m_heap.emplace<Detail::XmlBaseParser>(*this);
    m_notify.startDoc();
    if (!m_base->parse(src))
        return false;
    m_notify.endDoc();
    return true;
}

//===========================================================================
bool XStreamParser::fail(const char errmsg[]) {
    m_errmsg = m_heap.strDup(errmsg);
    return false;
}

//===========================================================================
size_t XStreamParser::errpos() const {
    return m_base->errpos();
}


/****************************************************************************
*
*   Public API
*
***/
