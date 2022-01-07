// Copyright Glen Knowles 2019 - 2022.
// Distributed under the Boost Software License, Version 1.0.
//
// Set of strings
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
    using difference_type = ptrdiff_t;
    using iterator = Iter;

public:
    StrTrieBase (IPageHeap * pages) noexcept;
    virtual ~StrTrieBase () = default;
    explicit operator bool() const { return !empty(); }

    // iterators
    iterator begin() const;
    iterator end() const;

    // capacity
    bool empty() const { return m_pages->empty(); }

    // modify    
    bool insert(std::string_view val);
    bool erase(std::string_view val);

    // search
    value_type front() const;
    value_type back() const;
    bool contains(std::string_view val) const;
    iterator find(std::string_view val) const;
    iterator findLess(std::string_view val) const;
    iterator findLessEqual(std::string_view val) const;
    iterator lowerBound(std::string_view val) const;
    iterator upperBound(std::string_view val) const;
    std::pair<iterator, iterator> equalRange(std::string_view val) const;

    // debug
    virtual std::ostream * const debugStream() const { return nullptr; }

protected:
    StrTrieBase () = default;
    void construct(IPageHeap * pages);

private:
    IPageHeap * m_pages = nullptr;
};

class StrTrie : public StrTrieBase {
public:
    StrTrie ();

    void clear();

    void debug(bool enable = true) { m_debug = enable; }
    std::ostream * const debugStream() const override;
    void dumpStats(std::ostream & os);

private:
    bool m_debug = false;
    PageHeap<512> m_heapImpl;
};


/****************************************************************************
*
*   StrTrieBase::Iter
*
***/

class StrTrieBase::Iter {
public:
    struct Impl;
    using iterator_category = std::bidirectional_iterator_tag;
    using value_type = StrTrieBase::value_type;
    using difference_type = StrTrieBase::difference_type;
    using pointer = const value_type *;
    using reference = value_type;

public:
    explicit Iter(const StrTrieBase * cont);
    explicit Iter(std::shared_ptr<Iter::Impl> impl);
    Iter(const Iter & from) noexcept = default;
    Iter & operator=(const Iter & from) noexcept = default;
    explicit operator bool() const;

    bool operator==(const Iter & right) const;
    const value_type & operator*() const;
    const value_type * operator->() const;
    Iter & operator++();
    Iter & operator--();

private:
    std::shared_ptr<Impl> m_impl;
};

} // namespace
