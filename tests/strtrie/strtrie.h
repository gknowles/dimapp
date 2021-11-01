// Copyright Glen Knowles 2019 - 2021.
// Distributed under the Boost Software License, Version 1.0.
//
// Map from strings to other strings
//
// strtrie.h - dim core
#pragma once

#include "cppconf/cppconf.h"

#include "core/pageheap.h"

#include <algorithm>
#include <span>
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
    class Iter;
    struct Node;

    using value_type = std::string;
    using iterator = Iter;

public:
    StrTrieBase (IPageHeap * pages) : m_pages{pages} {}
    virtual ~StrTrieBase () = default;

    // Returns whether key was inserted (didn't already exist).
    bool insert(std::string_view val);

    // Returns whether key was deleted, false if it wasn't present.
    bool erase(std::string_view val);

    explicit operator bool() const { return !empty(); }

    bool contains(std::string_view key) const;
    iterator lowerBound(std::string_view val) const;
    bool lowerBound(std::string * out, std::string_view val) const;
    bool upperBound(std::string * out, std::string_view val) const;
    bool equalRange(
        std::string * lower,
        std::string * upper,
        std::string_view val
    ) const;

    bool empty() const { return m_pages->empty(); }
    iterator begin() const;
    iterator end() const;

    virtual std::ostream * const debugStream() const { return nullptr; }

private:
    bool m_verbose = false;
    IPageHeap * m_pages;
};

class StrTrie : public StrTrieBase {
public:
    StrTrie () : StrTrieBase(&m_heapImpl) {}

    void clear();

    void debug(bool enable = true) { m_debug = enable; }
    std::ostream * const debugStream() const override;

private:
    bool m_debug = false;
    PageHeap<256> m_heapImpl;
};


/****************************************************************************
*
*   StrTrieBase::Iter
*
***/

class StrTrieBase::Iter {
    using value_type = StrTrieBase::value_type;
    value_type m_current;
public:
    Iter & operator++();
    bool operator==(const Iter & right) const;
    const value_type & operator*();
};

//===========================================================================
inline bool StrTrieBase::Iter::operator==(
    const Iter & other
) const {
    return m_current == other.m_current;
}

//===========================================================================
inline auto StrTrieBase::Iter::operator*()
    -> const value_type &
{
    return m_current;
}

} // namespace
