// Copyright Glen Knowles 2016 - 2017.
// Distributed under the Boost Software License, Version 1.0.
//
// xmlbaseparseevent.h - dim xml

#include "xmlbaseparse.h"

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
    m_cur = const_cast<char *>(eptr - 1);
    return true;
}

//===========================================================================
inline bool XmlBaseParser::onAttrNameStart(const char * ptr) {
    m_attr = ptr;
    return true;
}

//===========================================================================
inline bool XmlBaseParser::onAttrNameEnd(const char * eptr) {
    m_attrLen = eptr - m_attr - 1;
    return true;
}

//===========================================================================
inline bool XmlBaseParser::onAttrValueStart(const char * ptr) {
    m_base = ptr + 1;
    m_cur = const_cast<char *>(m_base);
    return true;
}

//===========================================================================
inline bool XmlBaseParser::onAttrValueEnd(const char * eptr) {
    m_notify.attr(m_attr, m_attrLen, m_base, m_cur - m_base);
    return true;
}

//===========================================================================
inline bool XmlBaseParser::onCDataWithEndStart(const char * ptr) {
    m_base = ptr;
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
inline bool XmlBaseParser::onCharRefStart(const char * ptr) {
    m_char = 0;
    return true;
}

//===========================================================================
inline bool XmlBaseParser::onCharRefEnd(const char * eptr) {
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
    m_char = m_char * 16 + hexToNibble(ch);
    return true;
}

//===========================================================================
inline bool XmlBaseParser::onElemNameStart(const char * ptr) {
    m_base = ptr;
    return true;
}

//===========================================================================
inline bool XmlBaseParser::onElemNameEnd(const char * eptr) {
    m_notify.startElem(m_base, eptr - m_base - 1);
    return true;
}

//===========================================================================
inline bool XmlBaseParser::onElemTextStart(const char * ptr) {
    m_base = ptr;
    m_cur = const_cast<char *>(ptr);
    return true;
}

//===========================================================================
inline bool XmlBaseParser::onElemTextEnd(const char * eptr) {
    m_notify.text(m_base, m_cur - m_base);
    return true;
}

//===========================================================================
inline bool XmlBaseParser::onElementEnd(const char * eptr) {
    m_notify.endElem();
    return true;
}

//===========================================================================
inline bool XmlBaseParser::onEntityAmpEnd(const char * eptr) {
    *m_cur++ = '&';
    return true;
}

//===========================================================================
inline bool XmlBaseParser::onEntityAposEnd(const char * eptr) {
    *m_cur++ = '\'';
    return true;
}

//===========================================================================
inline bool XmlBaseParser::onEntityGtEnd(const char * eptr) {
    *m_cur++ = '>';
    return true;
}

//===========================================================================
inline bool XmlBaseParser::onEntityLtEnd(const char * eptr) {
    *m_cur++ = '<';
    return true;
}

//===========================================================================
inline bool XmlBaseParser::onEntityOtherEnd(const char * eptr) {
    const char * amp = eptr - 1;
    while (*amp != '&')
        amp -= 1;
    std::string err{"unknown entity '"};
    err.append(amp, eptr - amp);
    err.append("'");
    return m_parser.fail(err.c_str());
}

//===========================================================================
inline bool XmlBaseParser::onEntityValueStart(const char * ptr) {
    m_base = ptr;
    m_cur = const_cast<char *>(ptr);
    return true;
}

//===========================================================================
inline bool XmlBaseParser::onEntityValueEnd(const char * eptr) {
    return true;
}

//===========================================================================
inline bool XmlBaseParser::onEntityQuotEnd(const char * eptr) {
    *m_cur++ = '"';
    return true;
}

//===========================================================================
inline bool XmlBaseParser::onNormalizableWsChar(char ch) {
    *m_cur++ = ' ';
    return true;
}

} // namespace