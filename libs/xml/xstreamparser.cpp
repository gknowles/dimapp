// Copyright Glen Knowles 2016 - 2017.
// Distributed under the Boost Software License, Version 1.0.
//
// xstreamparser.cpp - dim xml
#include "pch.h"
#pragma hdrstop

using namespace std;
using namespace Dim;


/****************************************************************************
*
*   BaseParserNotify
*
***/

class BaseParserNotify : public Detail::IXmlBaseParserNotify {
public:
    BaseParserNotify(XStreamParser & parser);

private:
    bool onStart() final { return true; }
    bool onEnd() final { return true; }
    bool onAttrCopyChar(char ch);
    bool onAttrInPlaceEnd(const char * eptr);
    bool onAttrNameStart(const char * ptr);
    bool onAttrNameEnd(const char * eptr);
    bool onAttrValueStart(const char * ptr);
    bool onAttrValueEnd(const char * eptr);
    bool onCDataWithEndStart(const char * ptr);
    bool onCDataWithEndEnd(const char * eptr);
    bool onCharDataChar(char ch);
    bool onCharRefStart(const char * ptr);
    bool onCharRefEnd(const char * eptr);
    bool onCharRefDigitChar(char ch);
    bool onCharRefHexdigChar(char ch);
    bool onElemNameStart(const char * ptr);
    bool onElemNameEnd(const char * eptr);
    bool onElemTextStart(const char * ptr);
    bool onElemTextEnd(const char * eptr);
    bool onElementEnd(const char * eptr);
    bool onEntityAmpEnd(const char * eptr);
    bool onEntityAposEnd(const char * eptr);
    bool onEntityGtEnd(const char * eptr);
    bool onEntityLtEnd(const char * eptr);
    bool onEntityOtherEnd(const char * eptr);
    bool onEntityQuotEnd(const char * eptr);
    bool onEntityValueStart(const char * ptr);
    bool onEntityValueEnd(const char * eptr);
    bool onNormalizableWsChar(char ch);

    XStreamParser & m_parser;
    IXStreamParserNotify & m_notify;
    const char * m_base{nullptr};
    char * m_cur{nullptr};
    const char * m_attr{nullptr};
    size_t m_attrLen{0};
    unsigned m_char{0};
};

//===========================================================================
BaseParserNotify::BaseParserNotify(XStreamParser & parser)
    : m_parser(parser)
    , m_notify(parser.notify()) {}

//===========================================================================
bool BaseParserNotify::onAttrCopyChar(char ch) {
    *m_cur++ = ch;
    return true;
}

//===========================================================================
bool BaseParserNotify::onAttrInPlaceEnd(const char * eptr) {
    m_cur = const_cast<char *>(eptr - 1);
    return true;
}

//===========================================================================
bool BaseParserNotify::onAttrNameStart(const char * ptr) {
    m_attr = ptr;
    return true;
}

//===========================================================================
bool BaseParserNotify::onAttrNameEnd(const char * eptr) {
    m_attrLen = eptr - m_attr - 1;
    return true;
}

//===========================================================================
bool BaseParserNotify::onAttrValueStart(const char * ptr) {
    m_base = ptr + 1;
    m_cur = const_cast<char *>(m_base);
    return true;
}

//===========================================================================
bool BaseParserNotify::onAttrValueEnd(const char * eptr) {
    m_notify.attr(m_attr, m_attrLen, m_base, m_cur - m_base);
    return true;
}

//===========================================================================
bool BaseParserNotify::onCDataWithEndStart(const char * ptr) {
    m_base = ptr;
    return true;
}

//===========================================================================
bool BaseParserNotify::onCDataWithEndEnd(const char * eptr) {
    // leave off trailing "]]>"
    m_notify.text(m_base, eptr - m_base - 3);
    return true;
}

//===========================================================================
bool BaseParserNotify::onCharDataChar(char ch) {
    *m_cur++ = ch;
    return true;
}

//===========================================================================
bool BaseParserNotify::onCharRefStart(const char * ptr) {
    m_char = 0;
    return true;
}

//===========================================================================
bool BaseParserNotify::onCharRefEnd(const char * eptr) {
    if (m_char < 0x80) {
        if (m_char < 0x20 && m_char != '\t' && m_char != '\n'
            && m_char != '\r') {
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
bool BaseParserNotify::onCharRefDigitChar(char ch) {
    m_char = m_char * 10 + ch - '0';
    return true;
}

//===========================================================================
bool BaseParserNotify::onCharRefHexdigChar(char ch) {
    m_char = m_char * 16 + hexToUnsigned(ch);
    return true;
}

//===========================================================================
bool BaseParserNotify::onElemNameStart(const char * ptr) {
    m_base = ptr;
    return true;
}

//===========================================================================
bool BaseParserNotify::onElemNameEnd(const char * eptr) {
    m_notify.startElem(m_base, eptr - m_base - 1);
    return true;
}

//===========================================================================
bool BaseParserNotify::onElemTextStart(const char * ptr) {
    m_base = ptr;
    m_cur = const_cast<char *>(ptr);
    return true;
}

//===========================================================================
bool BaseParserNotify::onElemTextEnd(const char * eptr) {
    m_notify.text(m_base, m_cur - m_base);
    return true;
}

//===========================================================================
bool BaseParserNotify::onElementEnd(const char * eptr) {
    m_notify.endElem();
    return true;
}

//===========================================================================
bool BaseParserNotify::onEntityAmpEnd(const char * eptr) {
    *m_cur++ = '&';
    return true;
}

//===========================================================================
bool BaseParserNotify::onEntityAposEnd(const char * eptr) {
    *m_cur++ = '\'';
    return true;
}

//===========================================================================
bool BaseParserNotify::onEntityGtEnd(const char * eptr) {
    *m_cur++ = '>';
    return true;
}

//===========================================================================
bool BaseParserNotify::onEntityLtEnd(const char * eptr) {
    *m_cur++ = '<';
    return true;
}

//===========================================================================
bool BaseParserNotify::onEntityOtherEnd(const char * eptr) {
    const char * amp = eptr - 1;
    while (*amp != '&')
        amp -= 1;
    string err = "unknown entity '"s + string(amp, eptr - amp) + "'";
    return m_parser.fail(err.c_str());
}

//===========================================================================
bool BaseParserNotify::onEntityValueStart(const char * ptr) {
    m_base = ptr;
    m_cur = const_cast<char *>(ptr);
    return true;
}

//===========================================================================
bool BaseParserNotify::onEntityValueEnd(const char * eptr) {
    return true;
}

//===========================================================================
bool BaseParserNotify::onEntityQuotEnd(const char * eptr) {
    *m_cur++ = '"';
    return true;
}

//===========================================================================
bool BaseParserNotify::onNormalizableWsChar(char ch) {
    *m_cur++ = ' ';
    return true;
}


/****************************************************************************
*
*   XStreamParser
*
***/

//===========================================================================
XStreamParser::XStreamParser(IXStreamParserNotify & notify)
    : m_notify(notify) {}

//===========================================================================
XStreamParser::~XStreamParser() {}

//===========================================================================
void XStreamParser::clear() {
    m_line = 0;
    m_errmsg = nullptr;
}

//===========================================================================
bool XStreamParser::parse(char src[]) {
    m_line = 0;
    m_errmsg = nullptr;
    m_heap.clear();
    auto * baseNotify = m_heap.emplace<BaseParserNotify>(*this);
    m_base = m_heap.emplace<Detail::XmlBaseParser>(baseNotify);
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

#if 0
//===========================================================================
static bool twoByte(unsigned char *& ptr, unsigned * ch) {
    // first byte is: c2 - df
    if ((ptr[1] & 0xc0) == 0x80) {
        *ch = ((*ptr & 0x1f) << 6) + (ptr[1] & 0x3f);
        ptr += 2;
        if (*ch >= 0x80)
            return true;
    }
    *ch = 0;
    return false;
}

//===========================================================================
static bool threeByte(unsigned char *& ptr, unsigned * ch) {
    // first byte is: e0 - ef
    if ((ptr[1] & 0xc0) == 0x80 && (ptr[2] & 0xc0) == 0x80) {
        *ch =
            ((*ptr & 0x0f) << 12) + ((ptr[1] & 0x3f) << 6) + (ptr[2] & 0x3f);
        ptr += 3;
        if (*ch >= 0x800)
            return true;
    }
    *ch = 0;
    return false;
}

//===========================================================================
static bool fourByte(unsigned char *& ptr, unsigned * ch) {
    // first byte is: f0 - f4
    if ((ptr[1] & 0xc0) == 0x80 && (ptr[2] & 0xc0) == 0x80 &&
        (ptr[3] & 0xc0) == 0x80) {
        *ch = ((*ptr & 0x07) << 18) + ((ptr[1] & 0x3f) << 12) +
                ((ptr[2] & 0x3f) << 6) + (ptr[3] & 0x3f);
        ptr += 4;
        if (*ch >= 0x10000)
            return true;
    }
    *ch = 0;
    return false;
}

//===========================================================================
static bool isNameStartChar(unsigned ch) {
    // list of half-open unicode character ranges allowed to start an
    // element name.
    const unsigned ranges[] = {
        0,      0x3a,   0x3b, // :
        0x41,   0x5b,         // A-Z
        0x5f,   0x60,         // _
        0x61,   0x7b,         // a-z
        0xc0,   0xd7,         // A-O w/diacritic
        0xd8,   0xf7,         // O-o w/diacritic
        0xf8,   0x300,  0x370,  0x37e,  0x37f,   0x2000,  0x200c,
        0x200e, 0x2070, 0x2190, 0x2c00, 0x2ff0,  0x3001,  0xd800,
        0xf900, 0xfdd0, 0xfdf0, 0xfffe, 0x10000, 0xf0000,
    };
    auto * range = lower_bound(ranges, ::end(ranges), ch);
    return (range - ranges) % 2 == 1;
}

//===========================================================================
static bool isNameChar(unsigned ch) {
    // list of half-open unicode character ranges allowed to start an
    // element name.
    const unsigned ranges[] = {
        0,      0x2d,   0x2f, // - .
        0x30,   0x3b,         // 0-9 :
        0x41,   0x5b,         // A-Z
        0x5f,   0x60,         // _
        0x61,   0x7b,         // a-z
        0xb7,   0xb8,         // Middle dot
        0xc0,   0xd7,         // A-O w/diacritic
        0xd8,   0xf7,         // O-o w/diacritic
        0xf8,   0x37e,        // o-y w/diacritic,
        0x37f,  0x2000, 0x200c, 0x200e, 0x203f,  0x2041,
        0x2070, 0x2190, 0x2c00, 0x2ff0, 0x3001,  0xd800,
        0xf900, 0xfdd0, 0xfdf0, 0xfffe, 0x10000, 0xf0000,
    };
    auto * range = lower_bound(ranges, ::end(ranges), ch);
    return (range - ranges) % 2 == 1;
}
#endif


/****************************************************************************
*
*   Public API
*
***/
