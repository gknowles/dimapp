// xstreamparser.cpp - dim xml
#include "pch.h"
#pragma hdrstop

using namespace std;
using namespace Dim;


/****************************************************************************
*
*   Private
*
***/

/****************************************************************************
*
*   XStreamParser
*
***/

//===========================================================================
XStreamParser::XStreamParser(IXStreamParserNotify & notify)
    : m_notify(notify)
{
    auto * baseNotify = new Detail::IXmlBaseParserNotify;
    m_base = make_unique<Detail::XmlBaseParser>(baseNotify);
}

//===========================================================================
XStreamParser::~XStreamParser() {
    auto * notify = m_base->notify(nullptr);
    delete notify;
}

//===========================================================================
bool XStreamParser::parse(char src[]) {
    m_line = 0;
    m_failed = false;
    return m_base->parse(src);
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

//===========================================================================
void XStreamParser::clear() {
    m_line = 0;
    m_failed = false;
}


/****************************************************************************
*
*   Public API
*
***/

