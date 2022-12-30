// Copyright Glen Knowles 2016 - 2020.
// Distributed under the Boost Software License, Version 1.0.
//
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
static_assert(size(kTextEntityTable) == kTextTypes);

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

enum IXBuilder::State : int {
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
IXBuilder & IXBuilder::start(string_view name) {
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
    case kStateTextRBracket2:
        addRaw("<");
        break;
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
IXBuilder & IXBuilder::startAttr(string_view name) {
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
IXBuilder & IXBuilder::elem(string_view name) {
    start(name);
    return end();
}

//===========================================================================
IXBuilder & IXBuilder::elem(string_view name, string_view val) {
    start(name);
    if (!val.empty())
        text(val);
    return end();
}

//===========================================================================
IXBuilder & IXBuilder::attr(string_view name, string_view val) {
    startAttr(name);
    text(val);
    return endAttr();
}

//===========================================================================
IXBuilder & IXBuilder::text(string_view val) {
    switch (m_state) {
    default: return fail();
    case kStateElemName:
    case kStateAttrEnd:
        addRaw(">");
        m_state = kStateText;
        break;
    case kStateAttrText:
        addText<false>(val);
        return *this;
    case kStateText:
    case kStateTextRBracket:
    case kStateTextRBracket2:
        break;
    }
    addText<true>(val);
    return *this;
}

//===========================================================================
template <bool isContent>
void IXBuilder::addText(string_view val) {
    auto ptr = val.data();
    auto base = val.data();
    auto count = val.size();
    for (; count; --count, ++ptr) {
        TextType type = (TextType)kTextTypeTable[*ptr];
        switch (type) {
        case kTextTypeGreater:
            // in content, ">" must be escaped when following "]]"
            if constexpr (isContent) {
                if (ptr - base >= 2) {
                    if (ptr[-1] == ']' && ptr[-2] == ']')
                        break;
                } else if (ptr - base == 1) {
                    if (ptr[-1] == ']'
                        && (m_state == kStateTextRBracket
                            || m_state == kStateTextRBracket2)) {
                        break;
                    }
                } else {
                    if (m_state == kStateTextRBracket2)
                        break;
                }
            }
            continue;
        case kTextTypeNormal:
            continue;
        case kTextTypeNull:
            goto DONE;
        case kTextTypeQuote:
            if constexpr (isContent)
                continue;
            break;
        case kTextTypeAmp:
        case kTextTypeLess:
            break;
        case kTextTypeInvalid:
            m_state = kStateFail;
            return;
        default:
            assert(!"invalid XML text type enum value");
        }

        addRaw({base, size_t(ptr - base)});
        addRaw(kTextEntityTable[type]);
        base = ptr + 1;
        if constexpr (isContent)
            m_state = kStateText;
    }

DONE:
    size_t num = ptr - base;
    if (!num)
        return;
    addRaw({base, num});
    if constexpr (isContent) {
        if (ptr[-1] != ']') {
            m_state = kStateText;
        } else {
            if (num == 1) {
                if (m_state == kStateText) {
                    m_state = kStateTextRBracket;
                } else {
                    m_state = kStateTextRBracket2;
                }
            } else {
                if (ptr[-2] != ']') {
                    m_state = kStateTextRBracket;
                } else {
                    m_state = kStateTextRBracket2;
                }
            }
        }
    }
}

//===========================================================================
IXBuilder & IXBuilder::fail() {
    // Fail is called due to application malfeasance, such as trying to start
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
void XBuilder::append(string_view text) {
    m_buf.append(text);
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
namespace Dim {
IXBuilder & operator<<(IXBuilder & out, const XNode & elem);
}
IXBuilder & Dim::operator<<(IXBuilder & out, const XNode & elem) {
    auto type = nodeType(&elem);
    switch (type) {
    default: assert(!"unknown XML node type"); return out;
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

//===========================================================================
string Dim::toString(const XNode & elem) {
    CharBuf buf;
    XBuilder bld(&buf);
    bld << elem;
    return toString(buf);
}
