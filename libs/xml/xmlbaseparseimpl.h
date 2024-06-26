// Copyright Glen Knowles 2016 - 2024.
// Distributed under the Boost Software License, Version 1.0.
//
// xmlbaseparseimpl.h - dim xml

#include "xmlbaseparse.g.h"

#include <string>

namespace Dim::Detail {


/****************************************************************************
*
*   XmlBaseParser
*
*   Implementation of events
*
***/

//===========================================================================
inline bool XmlBaseParser::onAttrCopyChar(char ch) {
    *m_cur++ = ch;
    return true;
}

//===========================================================================
inline bool XmlBaseParser::onAttrInPlaceEnd(const char * eptr) {
    m_cur = const_cast<char *>(eptr);
    return true;
}

//===========================================================================
inline bool XmlBaseParser::onAttrNameStart(const char * ptr) {
    m_attr = const_cast<char *>(ptr);
    return true;
}

//===========================================================================
inline bool XmlBaseParser::onAttrNameEnd(const char * eptr) {
    m_attrLen = eptr - m_attr;
    return true;
}

//===========================================================================
inline bool XmlBaseParser::onAttrValueStart(const char * ptr) {
    m_base = const_cast<char *>(ptr) + 1;
    m_cur = const_cast<char *>(m_base);
    return true;
}

//===========================================================================
inline bool XmlBaseParser::onAttrValueEnd() {
    m_notify.attr(
        m_attr,
        m_attrLen,
        m_base,
        m_cur - m_base
    );
    return true;
}

//===========================================================================
inline bool XmlBaseParser::onCDataWithEndStart(const char * ptr) {
    m_base = const_cast<char *>(ptr);
    return true;
}

//===========================================================================
inline bool XmlBaseParser::onCDataWithEndEnd(const char * eptr) {
    // leave off trailing "]]>"
    m_notify.text(m_base, eptr - m_base - 3);
    return true;
}

//===========================================================================
inline bool XmlBaseParser::onCharDataChar(char ch) {
    *m_cur++ = ch;
    return true;
}

//===========================================================================
inline bool XmlBaseParser::onCharRefStart() {
    m_char = 0;
    return true;
}

//===========================================================================
inline bool XmlBaseParser::onCharRefEnd() {
    if (m_char < 0x80) {
        if (m_char < 0x20
            && m_char != '\t' && m_char != '\n' && m_char != '\r'
        ) {
            return m_parser.fail("char ref of invalid code point");
        }
        *m_cur++ = (unsigned char)m_char;
    } else if (m_char < 0x800) {
        *m_cur++ = (unsigned char)(m_char >> 6) | 0xc0;
        *m_cur++ = (unsigned char)(m_char & 0xbf | 0x80);
    } else if (m_char < 0xd800 || m_char >= 0xe000 && m_char < 0xfffe) {
        *m_cur++ = (unsigned char)(m_char >> 12) | 0xe0;
        *m_cur++ = (unsigned char)(m_char >> 6) & 0xbf | 0x80;
        *m_cur++ = (unsigned char)(m_char & 0xbf | 0x80);
    } else if (m_char >= 0x10000 && m_char < 0x110000) {
        *m_cur++ = (unsigned char)(m_char >> 18) | 0xf0;
        *m_cur++ = (unsigned char)(m_char >> 12) & 0xbf | 0x80;
        *m_cur++ = (unsigned char)(m_char >> 6) & 0xbf | 0x80;
        *m_cur++ = (unsigned char)(m_char & 0xbf | 0x80);
    } else {
        return m_parser.fail("char ref of invalid code point");
    }
    return true;
}

//===========================================================================
inline bool XmlBaseParser::onCharRefDigitChar(char ch) {
    m_char = m_char * 10 + ch - '0';
    return true;
}

//===========================================================================
inline bool XmlBaseParser::onCharRefHexdigChar(char ch) {
    m_char = m_char * 16 + hexToNibbleUnsafe(ch);
    return true;
}

//===========================================================================
inline bool XmlBaseParser::onElemNameStart(const char * ptr) {
    m_base = const_cast<char *>(ptr);
    return true;
}

//===========================================================================
inline bool XmlBaseParser::onElemNameEnd(const char * eptr) {
    m_notify.startElem(m_base, eptr - m_base);
    return true;
}

//===========================================================================
inline bool XmlBaseParser::onElemTextStart(const char * ptr) {
    m_base = const_cast<char *>(ptr);
    m_cur = const_cast<char *>(ptr);
    return true;
}

//===========================================================================
inline bool XmlBaseParser::onElemTextEnd() {
    m_notify.text(m_base, m_cur - m_base);
    return true;
}

//===========================================================================
inline bool XmlBaseParser::onElementEnd() {
    m_notify.endElem();
    return true;
}

//===========================================================================
inline bool XmlBaseParser::onEntityAmpEnd() {
    *m_cur++ = '&';
    return true;
}

//===========================================================================
inline bool XmlBaseParser::onEntityAposEnd() {
    *m_cur++ = '\'';
    return true;
}

//===========================================================================
inline bool XmlBaseParser::onEntityGtEnd() {
    *m_cur++ = '>';
    return true;
}

//===========================================================================
inline bool XmlBaseParser::onEntityLtEnd() {
    *m_cur++ = '<';
    return true;
}

//===========================================================================
inline bool XmlBaseParser::onEntityOtherEnd(const char * eptr) {
    const char * amp = eptr;
    while (*amp != '&')
        amp -= 1;
    std::string err{"unknown entity '"};
    err.append(amp, eptr - amp);
    err.append("'");
    return m_parser.fail(err.c_str());
}

//===========================================================================
inline bool XmlBaseParser::onEntityQuotEnd() {
    *m_cur++ = '"';
    return true;
}

//===========================================================================
inline bool XmlBaseParser::onEntityValueStart(const char * ptr) {
    m_base = const_cast<char *>(ptr);
    m_cur = const_cast<char *>(ptr);
    return true;
}

//===========================================================================
inline bool XmlBaseParser::onEntityValueEnd(const char * eptr) {
    return true;
}

//===========================================================================
inline bool XmlBaseParser::onNormalizableWsChar() {
    *m_cur++ = ' ';
    return true;
}

} // namespace
