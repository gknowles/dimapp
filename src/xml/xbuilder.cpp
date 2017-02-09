// xbuilder.cpp - dim xml
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

enum TextType : char {
    kTextTypeInvalid = 0,
    kTextTypeNormal = 1,
    kTextTypeNull = 2,
    kTextTypeQuote = 3,
    kTextTypeAmp = 4,
    kTextTypeLess = 5,
    kTextTypeGreater = 6,
    kTextTypes,
};

const char * kTextEntityTable[] = {
    nullptr,
    nullptr,
    nullptr,
    "&quot;",
    "&amp;",
    "&lt;",
    "&gt;",
};
static_assert(size(kTextEntityTable) == kTextTypes, "");

// clang-format off
const char kTextTypeTable[256] = {
//  0  1  2  3  4  5  6  7  8  9  a  b  c  d  e  f
    2, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 1, 0, 0, // 0
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 1
    1, 1, 3, 1, 1, 1, 4, 1, 1, 1, 1, 1, 1, 1, 1, 1, // 2
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 5, 1, 6, 1, // 3
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, // 4
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, // 5
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, // 6
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, // 7
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, // 8
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, // 9
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, // a
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, // b
    0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, // c
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, // d
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, // e
    1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // f
};
// clang-format on

} // namespace


/****************************************************************************
*
*   IXBuilder
*
***/

enum IXBuilder::State {
    kStateFail,
    kStateDocIntro,      // _
    kStateElemName,      // <X_
    kStateAttrName,      // <X a_
    kStateAttrText,      // <X a="_
    kStateAttrEnd,       // <X a="1"_
    kStateText,          // >_
    kStateTextRBracket,  // >...]_
    kStateTextRBracket2, // >...]]_
    kStateDocEnd,        // </docRoot>_
};

//===========================================================================
IXBuilder::IXBuilder()
    : m_state(kStateDocIntro) {}

//===========================================================================
void IXBuilder::clear() {
    m_state = kStateDocIntro;
    m_stack.clear();
}

//===========================================================================
IXBuilder & IXBuilder::start(const char name[]) {
    switch (m_state) {
    default: return fail();
    case kStateAttrText:
        addRaw("\"");
        m_state = kStateAttrEnd;
        [[fallthrough]];
    case kStateElemName:
    case kStateAttrEnd:
        addRaw(">\n");
        m_state = kStateText;
        [[fallthrough]];
    case kStateDocIntro:
    case kStateText:
    case kStateTextRBracket:
    case kStateTextRBracket2: addRaw("<"); break;
    }
    size_t base = size();
    addRaw(name);
    m_stack.push_back({base, size() - base});
    m_state = kStateElemName;
    return *this;
}

//===========================================================================
IXBuilder & IXBuilder::end() {
    switch (m_state) {
    default: return fail();
    case kStateElemName:
    case kStateAttrEnd:
        addRaw("/>\n");
        m_state = kStateText;
        break;
    case kStateText:
    case kStateTextRBracket:
    case kStateTextRBracket2:
        addRaw("</");
        auto & top = m_stack.back();
        appendCopy(top.pos, top.len);
        addRaw(">\n");
        break;
    }
    m_stack.pop_back();
    if (m_stack.empty())
        m_state = kStateDocEnd;
    return *this;
}

//===========================================================================
IXBuilder & IXBuilder::startAttr(const char name[]) {
    switch (m_state) {
    default: return fail();
    case kStateElemName:
    case kStateAttrEnd: break;
    }
    addRaw(" ");
    addRaw(name);
    addRaw("=\"");
    m_state = kStateAttrText;
    return *this;
}

//===========================================================================
IXBuilder & IXBuilder::endAttr() {
    switch (m_state) {
    default: return fail();
    case kStateAttrText: break;
    }
    addRaw("\"");
    m_state = kStateAttrEnd;
    return *this;
}

//===========================================================================
IXBuilder & IXBuilder::elem(const char name[], const char val[]) {
    start(name);
    if (val && *val)
        text(val);
    return end();
}

//===========================================================================
IXBuilder & IXBuilder::attr(const char name[], const char val[]) {
    startAttr(name);
    text(val);
    return endAttr();
}

//===========================================================================
IXBuilder & IXBuilder::text(const char val[]) {
    switch (m_state) {
    default: return fail();
    case kStateElemName:
    case kStateAttrEnd:
        addRaw(">");
        m_state = kStateText;
        break;
    case kStateAttrText: addText<false>(val); return *this;
    case kStateText:
    case kStateTextRBracket:
    case kStateTextRBracket2: break;
    }
    addText<true>(val);
    return *this;
}

//===========================================================================
template <bool isContent> void IXBuilder::addText(const char val[]) {
    const char * base = val;
    for (;;) {
        TextType type = (TextType)kTextTypeTable[*val];
        switch (type) {
        case kTextTypeGreater:
            // in content, ">" must be escaped when following "]]"
            if (isContent) {
                if (val - base >= 2) {
                    if (val[-1] == ']' && val[-2] == ']')
                        break;
                } else if (val - base == 1) {
                    if (val[-1] == ']'
                        && (m_state == kStateTextRBracket
                            || m_state == kStateTextRBracket2)) {
                        break;
                    }
                } else {
                    if (m_state == kStateTextRBracket2)
                        break;
                }
            }
            [[fallthrough]];
        case kTextTypeNormal: val += 1; continue;
        case kTextTypeNull:
            if (size_t num = val - base) {
                addRaw(base, num);
                if (isContent) {
                    if (val[-1] != ']') {
                        m_state = kStateText;
                    } else {
                        if (num == 1) {
                            if (m_state == kStateText) {
                                m_state = kStateTextRBracket;
                            } else {
                                m_state = kStateTextRBracket2;
                            }
                        } else {
                            if (val[-2] != ']') {
                                m_state = kStateTextRBracket;
                            } else {
                                m_state = kStateTextRBracket2;
                            }
                        }
                    }
                }
            }
            return;
        case kTextTypeQuote:
            if (isContent) {
                val += 1;
                continue;
            }
            [[fallthrough]];
        case kTextTypeAmp : case kTextTypeLess : break;
        case kTextTypeInvalid: m_state = kStateFail; return;
        }

        addRaw(base, val - base);
        addRaw(kTextEntityTable[type]);
        base = ++val;
        if (isContent)
            m_state = kStateText;
    }
}

//===========================================================================
IXBuilder & IXBuilder::fail() {
    // Fail is called due to application malfeasence, such as trying to start
    // a new element while inside an attribute or writing invalid characters.
    assert(m_state == kStateFail);
    m_state = kStateFail;
    return *this;
}


/****************************************************************************
*
*   XBuilder
*
***/

//===========================================================================
void XBuilder::clear() {
    IXBuilder::clear();
    m_buf.clear();
}

//===========================================================================
void XBuilder::append(const char text[]) {
    m_buf.append(text);
}

//===========================================================================
void XBuilder::append(const char text[], size_t count) {
    m_buf.append(text, count);
}

//===========================================================================
void XBuilder::appendCopy(size_t pos, size_t count) {
    m_buf.append(m_buf, pos, count);
}

//===========================================================================
size_t XBuilder::size() const {
    return m_buf.size();
}


/****************************************************************************
*
*   Public API
*
***/

//===========================================================================
IXBuilder & Dim::operator<<(IXBuilder & out, int64_t val) {
    IntegralStr<int64_t> tmp(val);
    return out.text(tmp);
}

//===========================================================================
IXBuilder & Dim::operator<<(IXBuilder & out, uint64_t val) {
    IntegralStr<int64_t> tmp(val);
    return out.text(tmp);
}

//===========================================================================
IXBuilder & Dim::operator<<(IXBuilder & out, int val) {
    IntegralStr<int> tmp(val);
    return out.text(tmp);
}

//===========================================================================
IXBuilder & Dim::operator<<(IXBuilder & out, unsigned val) {
    IntegralStr<unsigned> tmp(val);
    return out.text(tmp);
}

//===========================================================================
IXBuilder & Dim::operator<<(IXBuilder & out, char val) {
    char str[] = {val, 0};
    return out.text(str);
}

//===========================================================================
IXBuilder & Dim::operator<<(IXBuilder & out, const char val[]) {
    return out.text(val);
}

//===========================================================================
IXBuilder & Dim::operator<<(IXBuilder & out, const std::string & val) {
    return out.text(val.c_str());
}

//===========================================================================
IXBuilder & Dim::operator<<(IXBuilder & out, const XNode & elem) {
    auto type = nodeType(&elem);
    switch (type) {
    default: assert(0 && "unknown xml node type"); return out;
    case XType::kText: out.text(elem.value); return out;
    case XType::kElement: break;
    }

    out.start(elem.name);
    for (auto && val : attrs(&elem)) {
        out.attr(val.name, val.value);
    }
    if (*elem.value)
        out.text(elem.value);
    for (auto && val : elems(&elem)) {
        out << val;
    }
    return out.end();
}
