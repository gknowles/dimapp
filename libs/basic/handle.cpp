// Copyright Glen Knowles 2015 - 2026.
// Distributed under the Boost Software License, Version 1.0.
//
// handle.cpp - dim basic
#include "pch.h"
#pragma hdrstop

using namespace std;
using namespace Dim;


/****************************************************************************
*
*   HandleStore
*
***/

//===========================================================================
HandleStore::HandleStore() {
    m_values.push_back({nullptr, 0});
}

//===========================================================================
HandleStore::~HandleStore() {
    assert(doEmpty());
}

//===========================================================================
bool HandleStore::doEmpty() const {
    return !m_numUsed;
}

//===========================================================================
pair<HandleBase, HandleContent *> HandleStore::doNext(HandleBase h) {
    h.pos += 1;
    if (h.pos >= m_values.size()) {
        return {};
    } else {
        return {h, m_values[h.pos].value};
    }
}

//===========================================================================
HandleContent * HandleStore::doFind(HandleBase handle) {
    assert(handle.pos >= 0);
    return handle.pos <= 0 || handle.pos >= (int)m_values.size()
        ? nullptr
        : m_values[handle.pos].value;
}

//===========================================================================
HandleBase HandleStore::doInsert(HandleContent * value) {
    int pos;
    if (!m_firstFree) {
        m_values.push_back({value, 0});
        pos = (int)m_values.size() - 1;
    } else {
        pos = m_firstFree;
        HandleMapNode & node = m_values[pos];
        m_firstFree = node.next;
        node = {value, 0};
    }

    m_numUsed += 1;
    return {pos};
}

//===========================================================================
HandleContent * HandleStore::doRelease(HandleBase handle) {
    assert(handle.pos >= 0);
    if (handle.pos <= 0 || handle.pos >= (int)m_values.size())
        return nullptr;
    HandleMapNode & node = m_values[handle.pos];
    if (!node.value)
        return nullptr;

    m_numUsed -= 1;
    auto value = node.value;
    node = {nullptr, m_firstFree};
    m_firstFree = handle.pos;
    return value;
}


/****************************************************************************
*
*   HandleStoreAsync
*
***/

//===========================================================================
bool HandleStoreAsync::doEmpty() const {
    shared_lock lk{m_mut};
    return s.doEmpty();;
}

//===========================================================================
pair<HandleBase, HandleContent *> HandleStoreAsync::doNext(HandleBase h) {
    shared_lock lk{m_mut};
    return s.doNext(h);
}

//===========================================================================
HandleContent * HandleStoreAsync::doFind(HandleBase handle) {
    shared_lock lk{m_mut};
    return s.doFind(handle);
}

//===========================================================================
HandleBase HandleStoreAsync::doInsert(HandleContent * value) {
    unique_lock lk{m_mut};
    return s.doInsert(value);
}

//===========================================================================
HandleContent * HandleStoreAsync::doRelease(HandleBase handle) {
    unique_lock lk{m_mut};
    return s.doRelease(handle);
}
