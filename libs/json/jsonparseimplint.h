// Copyright Glen Knowles 2018 - 2019.
// Distributed under the Boost Software License, Version 1.0.
//
// jsonparseimplint.h - dim json

#include "jsonparseint.h"

#include <string>

namespace Dim::Detail {


/****************************************************************************
*
*   JsonParser
*
*   Implementation of events
*
***/

//===========================================================================
// array
//===========================================================================
inline bool JsonParser::onArrayStart () {
    auto name = m_name;
    m_name = {};
    return m_notify.startArray(name);
}

//===========================================================================
inline bool JsonParser::onArrayEnd () {
    return m_notify.endArray();
}

//===========================================================================
// object
//===========================================================================
inline bool JsonParser::onObjectStart () {
    return m_notify.startObject(m_name);
}

//===========================================================================
inline bool JsonParser::onObjectEnd () {
    return m_notify.endObject();
}

//===========================================================================
inline bool JsonParser::onMemberNameEnd () {
    m_name = std::string_view{m_base, size_t(m_cur - m_base)};
    m_base = nullptr;
    return true;
}

//===========================================================================
// simple values
//===========================================================================
inline bool JsonParser::onNullEnd () {
    return m_notify.value(m_name, nullptr);
}

//===========================================================================
inline bool JsonParser::onFalseEnd () {
    return m_notify.value(m_name, false);
}

//===========================================================================
inline bool JsonParser::onTrueEnd () {
    return m_notify.value(m_name, true);
}

//===========================================================================
// number
//===========================================================================
inline bool JsonParser::onNvalEnd () {
    double val = 0;
    if (m_minus) {
        m_minus = false;
        m_int = -m_int;
    }
    if (m_exp || m_frac) {
        if (m_expMinus) {
            m_expMinus = false;
            m_exp = -m_exp;
        }
        val = m_int * pow(10.0, m_exp - m_frac);
        m_exp = 0;
        m_frac = 0;
    } else {
        val = (float) m_int;
    }
    m_int = 0;

    return m_notify.value(m_name, val);
}

//===========================================================================
inline bool JsonParser::onExpMinusEnd () {
    m_expMinus = true;
    return true;
}

//===========================================================================
inline bool JsonParser::onExpNumChar (char ch) {
    m_exp = 10 * m_exp + (ch - '0');
    return true;
}

//===========================================================================
inline bool JsonParser::onFracNumChar (char ch) {
    assert(ch >= '0');
    m_int = 10 * m_int + (ch - '0');
    m_frac += 1;
    return true;
}

//===========================================================================
inline bool JsonParser::onIntNumChar (char ch) {
    m_int = 10 * m_int + (ch - '0');
    return true;
}

//===========================================================================
inline bool JsonParser::onIntMinusEnd () {
    m_minus = true;
    return true;
}

//===========================================================================
// string
//===========================================================================
inline bool JsonParser::onSvalEnd () {
    auto val = std::string_view{m_base, size_t(m_cur - m_base)};
    m_base = nullptr;
    return m_notify.value(m_name, val);
}

//===========================================================================
inline bool JsonParser::onStrTextStart (const char * ptr) {
    m_base = m_cur = (char *) ptr;
    return true;
}

//===========================================================================
inline bool JsonParser::onEBkspEnd () {
    return onUnescapedChar('\b');
}

//===========================================================================
inline bool JsonParser::onEBslashEnd () {
    return onUnescapedChar('\\');
}

//===========================================================================
inline bool JsonParser::onECrEnd () {
    return onUnescapedChar('\r');
}

//===========================================================================
inline bool JsonParser::onEDquoteEnd () {
    return onUnescapedChar('"');
}

//===========================================================================
inline bool JsonParser::onEFeedEnd () {
    return onUnescapedChar('\f');
}

//===========================================================================
inline bool JsonParser::onENlEnd () {
    return onUnescapedChar('\n');
}

//===========================================================================
inline bool JsonParser::onESlashEnd () {
    return onUnescapedChar('/');
}

//===========================================================================
inline bool JsonParser::onETabEnd () {
    return onUnescapedChar('\t');
}

//===========================================================================
inline bool JsonParser::onEscapeNumHexChar (char ch) {
    m_hexchar = 16 * m_hexchar + hexToNibbleUnsafe(ch);
    return true;
}

//===========================================================================
inline bool JsonParser::onEscapeNumberEnd () {
    m_cur = copy_char(m_hexchar, m_cur);
    m_hexchar = 0;
    return true;
}

//===========================================================================
inline bool JsonParser::onUnescapedChar (char ch) {
    *m_cur++ = ch;
    return true;
}

} // namespace
