// Copyright Glen Knowles 2019.
// Distributed under the Boost Software License, Version 1.0.
//
// Map from strings to other strings
//
// strtrie.h - dim core
#pragma once

#include "cppconf/cppconf.h"

#include "pageheap.h"

#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace Dim {


/****************************************************************************
*
*   StrTrie
*
***/

class StrTrieBase {
public:
    using value_type = std::pair<std::string, std::string>;
    class Iterator;

public:
    StrTrieBase (IPageHeap & heap) : m_heap{heap} {}

    bool insert(std::string_view key);

    explicit operator bool() const { return !empty(); }

    bool contains(std::string_view name) const;
    bool lowerBound(std::string * out, std::string_view name) const;

    bool empty() const { return m_heap.empty(); }
    Iterator begin() const;
    Iterator end() const;

    std::ostream & dump(std::ostream & os) const;

private:
    uint8_t * nodeAppend(size_t pgno, uint8_t const * node);
    uint8_t * nodeAt(size_t pgno, size_t pos);
    uint8_t const * nodeAt(size_t pgno, size_t pos) const;

    // size and capacity, measured in nodes
    size_t size(size_t pgno) const;
    size_t capacity(size_t pgno) const;

    IPageHeap & m_heap;
};

class StrTrie : public StrTrieBase {
public:
    StrTrie () : StrTrieBase(m_heapImpl) {}

private:
    PageHeap<256> m_heapImpl;
};


/****************************************************************************
*
*   StrTrieBase::Iterator
*
***/

class StrTrieBase::Iterator {
    using value_type = StrTrieBase::value_type;
    value_type m_current;
public:
    Iterator & operator++();
    bool operator!=(Iterator const & right) const;
    value_type const & operator*();
};

//===========================================================================
inline bool StrTrieBase::Iterator::operator!=(
    Iterator const & right
) const {
    return m_current != right.m_current;
}

//===========================================================================
inline auto StrTrieBase::Iterator::operator*() 
    -> value_type const & 
{
    return m_current;
}

} // namespace
