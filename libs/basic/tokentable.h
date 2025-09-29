// Copyright Glen Knowles 2016 - 2025.
// Distributed under the Boost Software License, Version 1.0.
//
// Maps between a predefined set of enums and their string representations.
//
// tokentable.h - dim basic
#pragma once

#include "cppconf/cppconf.h"

#include "basic/types.h"

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
    explicit TokenTable(std::initializer_list<Token> tokens)
        : TokenTable(std::data(tokens), std::size(tokens))
    {}

    explicit operator bool() const { return !empty(); }

    // Get key from first Token in table with matching name or id. Returns
    // false if no tokens match.
    bool find(int * out, std::string_view name) const;
    bool find(int * out, const char name[], size_t nameLen = -1) const;
    bool findName(const char ** const out, int id) const;

    // Get key from first Token in table with matching name or id. Returns
    // defName or defId if no tokens match.
    auto find(const char name[], auto defId) const;
    auto find(std::string_view name, auto defId) const;
    const char * findName(auto id, const char defName[] = nullptr) const;

    // Each bit of the flags is considered as a separate search, and the
    // combined results of the searches is returned.
    template<typename T>
    std::vector<std::string_view> findNames(EnumFlags<T> flags) const;

    bool empty() const { return m_values.empty(); }
    Iterator begin() const;
    Iterator end() const;

private:
    struct Value {
        Token token{0, nullptr};
        int nameLen{0};
        size_t hash{0};
    };
    // Ensure reinterpret_cast between Token and Value is safe.
    static_assert(std::is_standard_layout_v<Value>);
    static_assert(offsetof(Value, token) == 0);

    struct Index {
        // Position in m_values of value being indexed.
        int pos{0};
        // Distance from ideal position in index. -1 if no hash collision.
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
template<typename T>
inline std::vector<std::string_view> TokenTable::findNames(
    EnumFlags<T> flags
) const {
    auto out = std::vector<std::string_view>{};
    const char * name = nullptr;
    auto raw = flags.underlying();
    while (raw) {
        auto f = (decltype(raw)) (1 << std::countr_zero((uintmax_t) raw));
        raw = raw & ~f;
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
