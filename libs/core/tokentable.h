// Copyright Glen Knowles 2016 - 2021.
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
        const char * name;
    };
    class Iterator;

public:
    TokenTable() : TokenTable(nullptr, 0) {}
    TokenTable(const Token * ptr, size_t count);
    template<typename T>
    explicit TokenTable(const T & tokens)
        : TokenTable(std::data(tokens), std::size(tokens))
    {}

    explicit operator bool() const { return !empty(); }

    bool find(int * out, std::string_view name) const;
    bool find(int * out, const char name[], size_t nameLen = -1) const;
    bool findName(const char ** const out, int id) const;

    auto find(const char name[], auto defId) const;
    auto find(std::string_view name, auto defId) const;
    const char * findName(auto id, const char defName[] = nullptr) const;
    std::vector<std::string_view> findNames(auto flags) const;

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

//===========================================================================
inline auto TokenTable::find(const char name[], auto defId) const {
    int id;
    return find(&id, name) ? (decltype(defId)) id : defId;
}

//===========================================================================
inline auto TokenTable::find(std::string_view name, auto defId) const {
    int id;
    return find(&id, name.data(), name.size()) ? (decltype(defId)) id : defId;
}

//===========================================================================
inline const char * TokenTable::findName(
    auto id, 
    const char defName[]
) const {
    const char * name;
    return findName(&name, (int)id) ? name : defName;
}

//===========================================================================
inline std::vector<std::string_view> TokenTable::findNames(auto flags) const {
    auto out = std::vector<std::string_view>{};
    const char * name = nullptr;
    while (flags) {
        auto f = (decltype(flags)) (1 << trailingZeroBits(flags));
        flags = flags & ~f;
        if (findName(&name, (int) f))
            out.push_back(name);
    }
    return out;
}


/****************************************************************************
*
*   TokenTable::Iterator
*
***/

class TokenTable::Iterator {
    const Token * m_current{};
public:
    Iterator(const Token * ptr);
    Iterator & operator++();
    bool operator==(const Iterator & right) const;
    const Token & operator*();
};

//===========================================================================
inline bool TokenTable::Iterator::operator==(const Iterator & right) const {
    return m_current == right.m_current;
}

//===========================================================================
inline const TokenTable::Token & TokenTable::Iterator::operator*() {
    return *m_current;
}


} // namespace
