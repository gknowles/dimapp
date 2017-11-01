// Copyright Glen Knowles 2015 - 2017.
// Distributed under the Boost Software License, Version 1.0.
//
// handle.cpp - dim core
#include "pch.h"
#pragma hdrstop

using namespace std;
using namespace Dim;


/****************************************************************************
*
*   HamdleMap
*
***/

//===========================================================================
HandleMapBase::HandleMapBase() {
    m_values.push_back({nullptr, 0});
}

//===========================================================================
HandleMapBase::~HandleMapBase() {
    assert(empty());
}

//===========================================================================
bool HandleMapBase::empty() const {
    return !m_numUsed;
}

//===========================================================================
HandleContent * HandleMapBase::find(HandleBase handle) {
    assert(handle.pos >= 0);
    return handle.pos <= 0 || handle.pos >= (int)m_values.size() 
        ? nullptr
        : m_values[handle.pos].value;
}

//===========================================================================
HandleBase HandleMapBase::insert(HandleContent * value) {
    int pos;
    if (!m_firstFree) {
        m_values.push_back({value, 0});
        pos = (int)m_values.size() - 1;
    } else {
        pos = m_firstFree;
        Node & node = m_values[pos];
        m_firstFree = node.next;
        node = {value, 0};
    }

    m_numUsed += 1;
    return {pos};
}

//===========================================================================
HandleContent * HandleMapBase::release(HandleBase handle) {
    assert(handle.pos >= 0);
    if (handle.pos <= 0 || handle.pos >= (int)m_values.size()) 
        return nullptr;
    Node & node = m_values[handle.pos];
    if (!node.value)
        return nullptr;

    m_numUsed -= 1;
    auto value = node.value;
    node = {nullptr, m_firstFree};
    m_firstFree = handle.pos;
    return value;
}
