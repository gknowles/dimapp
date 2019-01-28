// Copyright Glen Knowles 2019.
// Distributed under the Boost Software License, Version 1.0.
//
// Map from strings to other strings
//
// strtrie.h - dim core
#pragma once

#include "cppconf/cppconf.h"

#include <string>
#include <string_view>

namespace Dim {


/****************************************************************************
*
*   StrTrie
*
***/

class StrTrie {
public:
    using value_type = std::pair<std::string, std::string>;
    class Iterator;

public:
    bool insert(std::string_view key, std::string_view value);

    explicit operator bool() const { return !empty(); }

    bool find(std::string * out, std::string_view name) const;

    bool empty() const { return m_data.empty(); }
    Iterator begin() const;
    Iterator end() const;

    std::ostream & dump(std::ostream & os) const;

private:
    unsigned char * nodeAt(size_t pos);
    unsigned char const * nodeAt(size_t pos) const;

    std::string m_data;
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
