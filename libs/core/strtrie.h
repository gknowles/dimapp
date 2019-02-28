// Copyright Glen Knowles 2019.
// Distributed under the Boost Software License, Version 1.0.
//
// Map from strings to other strings
//
// strtrie.h - dim core
#pragma once

#include "cppconf/cppconf.h"

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

class StrTrie {
public:
    static constexpr unsigned kPageSize = 256;
    using value_type = std::pair<std::string, std::string>;
    class Iterator;

public:
    bool insert(std::string_view key, std::string_view value);

    explicit operator bool() const { return !empty(); }

    bool find(std::string * out, std::string_view name) const;

    bool empty() const { return m_pages.empty(); }
    Iterator begin() const;
    Iterator end() const;

    std::ostream & dump(std::ostream & os) const;

private:
    bool pageEmpty() const;
    size_t pageRoot() const;
    size_t pageNew();

    uint8_t * nodeAppend(size_t pgno, uint8_t const * node);
    uint8_t * nodeAt(size_t pgno, size_t pos);
    uint8_t const * nodeAt(size_t pgno, size_t pos) const;

    // size and capacity, measured in nodes
    size_t size(size_t pgno) const;
    size_t capacity(size_t pgno) const;

    std::vector<std::unique_ptr<uint8_t[]>> m_pages;
};


/****************************************************************************
*
*   StrTrie::Iterator
*
***/

class StrTrie::Iterator {
    StrTrie::value_type m_current;
public:
    Iterator & operator++();
    bool operator!=(Iterator const & right) const;
    StrTrie::value_type const & operator*();
};

//===========================================================================
inline bool StrTrie::Iterator::operator!=(Iterator const & right) const {
    return m_current != right.m_current;
}

//===========================================================================
inline StrTrie::value_type const & StrTrie::Iterator::operator*() {
    return m_current;
}

} // namespace
