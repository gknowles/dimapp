// Copyright Glen Knowles 2016 - 2017.
// Distributed under the Boost Software License, Version 1.0.
//
// tokentable.h - dim core
#pragma once

#include "cppconf/cppconf.h"

#include <cassert>
#include <string_view>
#include <vector>

namespace Dim {


/****************************************************************************
*
*   TokenTable
*
***/

class TokenTable {
public:
    struct Token {
        int id;
        const char * name;
    };
    class Iterator;

public:
    TokenTable(const Token * ptr, size_t count);

    bool find(int * out, const char name[], size_t nameLen = -1) const;
    bool find(const char ** const out, int id) const;

    Iterator begin() const;
    Iterator end() const;

private:
    struct Value {
        Token token{0, nullptr};
        int nameLen{0};
        size_t hash{0};
    };
    struct Index {
        int pos{0};
        int distance{-1};
    };
    std::vector<Value> m_values;
    std::vector<Index> m_byName;
    std::vector<Index> m_byId;
    int m_hashLen;
};


/****************************************************************************
*
*   TokenTable::Iterator
*
***/

class TokenTable::Iterator {
    const Token * m_current{nullptr};
public:
    Iterator(const Token * ptr);
    Iterator & operator++();
    bool operator!=(const Iterator & right) const;
    const Token & operator*();
};

//===========================================================================
inline bool TokenTable::Iterator::operator!=(const Iterator & right) const { 
    return m_current != right.m_current; 
}

//===========================================================================
inline const TokenTable::Token & TokenTable::Iterator::operator*() { 
    return *m_current; 
}


/****************************************************************************
*
*   Free functions
*
***/

template <typename E>
E tokenTableGetEnum(const TokenTable & tbl, const char name[], E defId) {
    int id;
    return tbl.find(&id, name) ? (E)id : defId;
}

template <typename E>
E tokenTableGetEnum(const TokenTable & tbl, std::string_view name, E defId) {
    int id;
    return tbl.find(&id, name.data(), name.size()) ? (E)id : defId;
}

template <typename E>
const char * tokenTableGetName(
    const TokenTable & tbl,
    E id,
    const char defName[] = nullptr
) {
    const char * name;
    return tbl.find(&name, (int)id) ? name : defName;
}

} // namespace
