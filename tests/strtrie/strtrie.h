// Copyright Glen Knowles 2019 - 2021.
// Distributed under the Boost Software License, Version 1.0.
//
// Map from strings to other strings
//
// strtrie.h - dim core
#pragma once

#include "cppconf/cppconf.h"

#include "core/pageheap.h"

#include <string>
#include <string_view>

namespace Dim {


/****************************************************************************
*
*   StrTrie
*
***/

class StrTrieBase {
public:
    using value_type = std::string;
    class Iterator;
    struct Node;

public:
    StrTrieBase (IPageHeap & heap) : m_heap{heap} {}

    // returns whether key was inserted (didn't already exist).
    bool insert(std::string_view val);

    // returns whether key was deleted, false if it wasn't present
    bool erase(std::string_view val);

    explicit operator bool() const { return !empty(); }

    bool contains(std::string_view key) const;
    bool lowerBound(std::string * out, std::string_view val) const;
    bool upperBound(std::string * out, std::string_view val) const;
    bool equalRange(
        std::string * lower,
        std::string * upper,
        std::string_view val
    ) const;

    bool empty() const { return m_heap.empty(); }
    Iterator begin() const;
    Iterator end() const;

    std::ostream & dump(std::ostream & os) const;

private:
    IPageHeap & m_heap;
};

class StrTrie : public StrTrieBase {
public:
    StrTrie () : StrTrieBase(m_heapImpl) {}

    void clear() { m_heapImpl.clear(); }
    void swap(StrTrie & other);

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
    bool operator==(const Iterator & right) const;
    value_type const & operator*();
};

//===========================================================================
inline bool StrTrieBase::Iterator::operator==(
    const Iterator & other
) const {
    return m_current == other.m_current;
}

//===========================================================================
inline auto StrTrieBase::Iterator::operator*()
    -> value_type const &
{
    return m_current;
}

} // namespace
