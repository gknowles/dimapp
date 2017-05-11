// Copyright Glen Knowles 2016 - 2017.
// Distributed under the Boost Software License, Version 1.0.
//
// jbuilder.cpp - dim json
#include "pch.h"
#pragma hdrstop

using namespace std;
using namespace Dim;


/****************************************************************************
*
*   IJBuilder
*
***/

enum IJBuilder::State : int {
    kStateFail,
    kStateFirstValue,
    kStateValue,    
    kStateFirstMember,
    kStateMember,
    kStateMemberValue,
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
    append("{");
    m_stack.push_back(true);
    return *this;
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
    append("[");
    m_stack.push_back(false);
    return *this;
}

//===========================================================================
IJBuilder & IJBuilder::end() {
    if (m_stack.empty())
        return fail();
    switch (m_state) {
    default: return fail();
    case kStateFirstValue:
    case kStateValue:
        append("\n]");
        break;
    case kStateFirstMember:
    case kStateMember:
        append("\n}");
        break;
    }
    m_state = m_stack.back() ? kStateMember : kStateValue;
    m_stack.pop_back();
    if (m_stack.empty()) {
        append("\n");
        m_state = kStateDocEnd;
    }
    return *this;
}

//===========================================================================
IJBuilder & IJBuilder::startMember(string_view name) {
    switch (m_state) {
    default: 
        return fail();
    case kStateMember:
        append(",\n");
    case kStateFirstMember:
        break;
    }
    m_state = kStateMemberValue;
    appendString(name);
    append(":");
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
    case kStateMemberValue:
        m_state = kStateMember;
        break;
    }
    appendString(val);
    return *this;
}

//===========================================================================
IJBuilder & IJBuilder::addValue(string_view val) {
    switch (m_state) {
    default: 
        return fail();
    case kStateValue:
        append(",\n");
        break;
    case kStateFirstValue:
        m_state = kStateValue;
        break;
    case kStateMemberValue:
        m_state = kStateMember;
        break;
    }
    append(val);
    return *this;
}

//===========================================================================
IJBuilder & IJBuilder::value(bool val) {
    return addValue(val ? "true" : "false");
}

//===========================================================================
IJBuilder & IJBuilder::value(double val) {
    return addValue("*double*");
}

//===========================================================================
IJBuilder & IJBuilder::value(int64_t val) {
    IntegralStr<int64_t> tmp(val);
    return addValue(tmp);
}

//===========================================================================
IJBuilder & IJBuilder::value(uint64_t val) {
    IntegralStr<uint64_t> tmp(val);
    return addValue(tmp);
}

//===========================================================================
IJBuilder & IJBuilder::value(std::nullptr_t) {
    return addValue("null");
}

//===========================================================================
void IJBuilder::appendString(string_view val) {
    // TODO: escape control characters, dquote and bslash
    append("\"");
    append(val);
    append("\"");
}

//===========================================================================
IJBuilder & IJBuilder::fail() {
    // Fail is called due to application malfeasence, such as trying to start
    // a new element while inside an attribute or writing invalid characters.
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
    m_buf.clear();
}

//===========================================================================
void JBuilder::append(string_view val) {
    m_buf.append(val);
}

//===========================================================================
size_t JBuilder::size() const {
    return m_buf.size();
}


/****************************************************************************
*
*   Public API
*
***/

//===========================================================================
IJBuilder & Dim::operator<<(IJBuilder & out, std::string_view val) {
    return out.value(val);
}

//===========================================================================
IJBuilder & Dim::operator<<(IJBuilder & out, int64_t val) {
    return out.value(val);
}

//===========================================================================
IJBuilder & Dim::operator<<(IJBuilder & out, uint64_t val) {
    return out.value(val);
}

//===========================================================================
IJBuilder & Dim::operator<<(IJBuilder & out, bool val) {
    return out.value(val);
}

//===========================================================================
IJBuilder & Dim::operator<<(IJBuilder & out, double val) {
    return out.value(val);
}

//===========================================================================
IJBuilder & Dim::operator<<(IJBuilder & out, nullptr_t) {
    return out.value(nullptr);
}
