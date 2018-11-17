// Copyright Glen Knowles 2016 - 2018.
// Distributed under the Boost Software License, Version 1.0.
//
// Maps between a predefined set of enums and their string representations.
//
// tokentable.h - dim core
#pragma once

#include "cppconf/cppconf.h"

#include <cassert>
#include <iterator>
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
        char const * name;
    };
    class Iterator;

public:
    TokenTable() : TokenTable(nullptr, 0) {}
    TokenTable(Token const * ptr, size_t count);
    template<typename T>
    explicit TokenTable(T const & tokens)
        : TokenTable(std::data(tokens), std::size(tokens))
    {}

    explicit operator bool() const { return !empty(); }

    bool find(int * out, std::string_view name) const;
    bool find(int * out, char const name[], size_t nameLen = -1) const;
    bool find(char const ** const out, int id) const;

    bool empty() const { return m_values.empty(); }
    Iterator begin() const;
    Iterator end() const;

private:
    struct Value {
        Token token{0, nullptr};
        int nameLen{0};
        size_t hash{0};
    };
    // Allow reinterpret_cast to/from Token
    static_assert(std::is_standard_layout_v<Value>);
    static_assert(offsetof(Value, token) == 0);

    struct Index {
        int pos{0};
        int distance{-1};
    };
    std::vector<Value> m_values;
    std::vector<Index> m_byName;
    std::vector<Index> m_byId;
    int m_hashLen{0};
};


/****************************************************************************
*
*   TokenTable::Iterator
*
***/

class TokenTable::Iterator {
    Token const * m_current{};
public:
    Iterator(Token const * ptr);
    Iterator & operator++();
    bool operator!=(Iterator const & right) const;
    Token const & operator*();
};

//===========================================================================
inline bool TokenTable::Iterator::operator!=(Iterator const & right) const {
    return m_current != right.m_current;
}

//===========================================================================
inline TokenTable::Token const & TokenTable::Iterator::operator*() {
    return *m_current;
}


/****************************************************************************
*
*   Free functions
*
***/

//===========================================================================
template <typename E>
E tokenTableGetEnum(TokenTable const & tbl, char const name[], E defId) {
    int id;
    return tbl.find(&id, name) ? (E)id : defId;
}

//===========================================================================
template <typename E>
E tokenTableGetEnum(TokenTable const & tbl, std::string_view name, E defId) {
    int id;
    return tbl.find(&id, name.data(), name.size()) ? (E)id : defId;
}

//===========================================================================
template <typename E>
char const * tokenTableGetName(
    TokenTable const & tbl,
    E id,
    char const defName[] = nullptr
) {
    char const * name;
    return tbl.find(&name, (int)id) ? name : defName;
}

//===========================================================================
template <typename E>
std::vector<std::string_view> tokenTableGetFlagNames(
    TokenTable const & tbl,
    E flags
) {
    auto out = std::vector<std::string_view>{};
    char const * name = nullptr;
    while (flags) {
        auto f = E(1 << trailingZeroBits(flags));
        flags = flags & ~f;
        if (tbl.find(&name, f))
            out.push_back(name);
    }
    return out;
}

} // namespace
