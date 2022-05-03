// Copyright Glen Knowles 2016 - 2022.
// Distributed under the Boost Software License, Version 1.0.
//
// jbuilder.cpp - dim json
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
    kTextTypeNormal = 32,
    kTextTypeDQuote = 33,
    kTextTypeBSlash = 34,
    kTextTypeInvalid = 35,
    kTextTypes,
};

const char * kTextEntityTable[] = {
    "\\u0000",
    "\\u0001",
    "\\u0002",
    "\\u0003",
    "\\u0004",
    "\\u0005",
    "\\u0006",
    "\\u0007",
    "\\b",
    "\\t",
    "\\n",
    "\\u000b",
    "\\f",
    "\\r",
    "\\u000e",
    "\\u000f",
    "\\u0010",
    "\\u0011",
    "\\u0012",
    "\\u0013",
    "\\u0014",
    "\\u0015",
    "\\u0016",
    "\\u0017",
    "\\u0018",
    "\\u0019",
    "\\u001a",
    "\\u001b",
    "\\u001c",
    "\\u001d",
    "\\u001e",
    "\\u001f",
    nullptr,
    "\\\"",
    "\\\\",
    nullptr,
};
static_assert(size(kTextEntityTable) == kTextTypes);

// clang-format off
const char kTextTypeTable[256] = {
//   0   1   2   3   4   5   6   7   8   9  a   b   c   d   e   f
     0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15, // 0
    16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, // 1
    32, 32, 33, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, // 2
    32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, // 3
    32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, // 4
    32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 34, 32, 32, 32, // 5
    32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, // 6
    32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, // 7
    32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, // 8
    32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, // 9
    32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, // a
    32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, // b
    35, 35, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, // c
    32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, // d
    32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, // e
    32, 32, 32, 32, 32, 35, 35, 35, 35, 35, 35, 35, 35, 35, 35, 35, // f
};
// clang-format on

} // namespace


/****************************************************************************
*
*   IJBuilder
*
***/

enum IJBuilder::State : int {
    kStateFail,
    kStateFirstValue,
    kStateValue,
    kStateValueText,
    kStateFirstMember,
    kStateMember,
    kStateMemberValue,
    kStateMemberValueText,
    kStateDocEnd,
};

//===========================================================================
IJBuilder::IJBuilder()
    : m_state(kStateFirstValue) {}

//===========================================================================
void IJBuilder::clear() {
    m_state = kStateFirstValue;
    m_stack.clear();
}

//===========================================================================
IJBuilder & IJBuilder::array() {
    switch (m_state) {
    default:
        return fail();
    case kStateValue:
        append(",\n");
        break;
    case kStateFirstValue:
    case kStateMemberValue:
        break;
    }
    m_state = kStateFirstValue;
    append('[');
    m_stack.push_back(false);
    return *this;
}

//===========================================================================
IJBuilder & IJBuilder::object() {
    switch (m_state) {
    default:
        return fail();
    case kStateValue:
        append(",\n");
        break;
    case kStateFirstValue:
    case kStateMemberValue:
        break;
    }
    m_state = kStateFirstMember;
    append('{');
    m_stack.push_back(true);
    return *this;
}

//===========================================================================
IJBuilder & IJBuilder::end() {
    if (m_stack.empty())
        return fail();
    switch (m_state) {
    default: return fail();
    case kStateFirstValue:
        assert(!m_stack.back());
        append(']');
        break;
    case kStateValue:
        assert(!m_stack.back());
        append("\n]");
        break;
    case kStateValueText:
        append('"');
        m_state = kStateValue;
        return *this;
    case kStateFirstMember:
        assert(m_stack.back());
        append('}');
        break;
    case kStateMember:
        assert(m_stack.back());
        append("\n}");
        break;
    case kStateMemberValueText:
        append('"');
        m_state = kStateMemberValue;
        return *this;
    }
    m_stack.pop_back();
    if (m_stack.empty()) {
        append('\n');
        m_state = kStateDocEnd;
    } else {
        m_state = m_stack.back() ? kStateMember : kStateValue;
    }
    return *this;
}

//===========================================================================
IJBuilder & IJBuilder::member(string_view name) {
    switch (m_state) {
    default:
        return fail();
    case kStateMember:
        append(",\n");
        break;
    case kStateFirstMember:
        break;
    }
    m_state = kStateMemberValue;
    append('"');
    addString(name);
    append("\":");
    return *this;
}

//===========================================================================
IJBuilder & IJBuilder::startValue() {
    switch (m_state) {
    default:
        return fail();
    case kStateValue:
        append(",\n");
        m_state = kStateValueText;
        break;
    case kStateFirstValue:
        m_state = kStateValueText;
        break;
    case kStateMemberValue:
        m_state = kStateMemberValueText;
        break;
    }
    append('"');
    return *this;
}

//===========================================================================
IJBuilder & IJBuilder::value(string_view val) {
    switch (m_state) {
    default:
        return fail();
    case kStateValue:
        append(",\n");
        break;
    case kStateFirstValue:
        m_state = kStateValue;
        break;
    case kStateValueText:
    case kStateMemberValueText:
        addString(val);
        return *this;
    case kStateMemberValue:
        m_state = kStateMember;
        break;
    }
    append('"');
    addString(val);
    append('"');
    return *this;
}

//===========================================================================
IJBuilder & IJBuilder::valueRaw(string_view val) {
    switch (m_state) {
    default:
        return fail();
    case kStateValue:
        append(",\n");
        break;
    case kStateFirstValue:
        m_state = kStateValue;
        break;
    case kStateValueText:
    case kStateMemberValueText:
        break;
    case kStateMemberValue:
        m_state = kStateMember;
        break;
    }
    append(val);
    return *this;
}

//===========================================================================
IJBuilder::StateReturn IJBuilder::state() const {
    switch (m_state) {
    case kStateFirstValue:
    case kStateValue:
        return {Type::kValue, Type::kArray};
    case kStateValueText:
        return {Type::kText, Type::kArray};
    case kStateFirstMember:
    case kStateMember:
        return {Type::kMember, Type::kObject};
    case kStateMemberValue:
        return {Type::kValue, Type::kObject};
    case kStateMemberValueText:
        return {Type::kText, Type::kObject};
    default:
        return {};
    }
}

//===========================================================================
IJBuilder & IJBuilder::value(const char val[]) {
    return value(string_view{val});
}

//===========================================================================
IJBuilder & IJBuilder::value(char val) {
    return value(string_view{&val, 1});
}

//===========================================================================
IJBuilder & IJBuilder::value(bool val) {
    return valueRaw(val ? "true" : "false");
}

//===========================================================================
IJBuilder & IJBuilder::value(double val) {
    if (isnan(val) || isinf(val))
        return fail();
    StrFrom tmp(val);
    return valueRaw(tmp.view());
}

//===========================================================================
IJBuilder & IJBuilder::value(int64_t val) {
    return valueRaw(StrFrom(val).view());
}

//===========================================================================
IJBuilder & IJBuilder::value(uint64_t val) {
    return valueRaw(StrFrom(val).view());
}

//===========================================================================
IJBuilder & IJBuilder::value(nullptr_t) {
    return valueRaw("null");
}

//===========================================================================
void IJBuilder::addString(string_view val) {
    auto ptr = val.data();
    auto count = val.size();
    auto base = ptr;
    for (; count; --count, ++ptr) {
        auto type = (TextType)kTextTypeTable[*ptr];
        switch (type) {
        case kTextTypeInvalid:
            m_state = kStateFail;
            return;
        case kTextTypeNormal:
            continue;
        default:
            break;
        }

        append({base, size_t(ptr - base)});
        append(kTextEntityTable[type]);
        base = ptr + 1;
    }

    if (size_t num = ptr - base)
        append({base, num});
}

//===========================================================================
IJBuilder & IJBuilder::fail() {
    // Fail is called due to application malfeasance, such as writing
    // invalid characters.
    assert(m_state == kStateFail);
    m_state = kStateFail;
    return *this;
}


/****************************************************************************
*
*   JBuilder
*
***/

//===========================================================================
void JBuilder::clear() {
    IJBuilder::clear();
    m_buf->clear();
}

//===========================================================================
void JBuilder::append(string_view val) {
    m_buf->append(val);
}

//===========================================================================
void JBuilder::append(char val) {
    m_buf->append(1, val);
}

//===========================================================================
size_t JBuilder::size() const {
    return m_buf->size();
}
