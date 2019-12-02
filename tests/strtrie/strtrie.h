// Copyright Glen Knowles 2019.
// Distributed under the Boost Software License, Version 1.0.
//
// Map from strings to other strings
//
// strtrie.h - dim core
#pragma once

#include "cppconf/cppconf.h"

//#include "pageheap.h"
#include "core/pageheap.h"

#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <variant>
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
    struct Node;
    struct SearchState;

public:
    StrTrieBase (IPageHeap & heap) : m_heap{heap} {}

    // returns whether key was inserted (didn't already exist).
    bool insert(std::string_view key);

    explicit operator bool() const { return !empty(); }

    bool contains(std::string_view name) const;
    bool lowerBound(std::string * out, std::string_view name) const;

    bool empty() const { return m_heap.empty(); }
    Iterator begin() const;
    Iterator end() const;

    std::ostream & dump(std::ostream & os) const;

private:
    bool insertAtFork (SearchState * ss);
    bool insertAtSeg (SearchState * ss);
    bool splitSegInPlace(
        SearchState * ss,
        uint8_t spos,
        uint8_t slen,
        uint8_t sval
    );
    void addInPlaceSegs(SearchState * ss);
    bool splitSegToOverflow(
        SearchState * ss,
        uint8_t spos,
        uint8_t slen,
        uint8_t sval
    );
    void addOverflowSegs(SearchState * ss);

    Node * nodeAppend(size_t pgno, const Node * node);
    Node * nodeAt(size_t pgno, size_t pos);
    const Node * nodeAt(size_t pgno, size_t pos) const;

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
    bool operator==(const Iterator & right) const;
    value_type const & operator*();
};

//===========================================================================
inline bool StrTrieBase::Iterator::operator==(
    const Iterator & right
) const {
    return m_current == right.m_current;
}

//===========================================================================
inline auto StrTrieBase::Iterator::operator*()
    -> value_type const &
{
    return m_current;
}

} // namespace
