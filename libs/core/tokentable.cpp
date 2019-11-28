// Copyright Glen Knowles 2016 - 2019.
// Distributed under the Boost Software License, Version 1.0.
//
// tokentable.cpp - dim core
#include "pch.h"
#pragma hdrstop

using namespace std;
using namespace Dim;


/****************************************************************************
*
*   TokenTable
*
***/

//===========================================================================
TokenTable::TokenTable(const Token * src, size_t count) {
    if (!count)
        return;

    const Token * eptr = src + count;
    m_hashLen = 0;
    for (const Token * a = src + 1; a != eptr; ++a) {
        for (const Token * b = src; b != a; ++b) {
            int pos = 0;
            while (a->name[pos] == b->name[pos]) {
                if (!a->name[pos])
                    goto NEXT_A;
                pos += 1;
            }
            if (pos >= m_hashLen)
                m_hashLen = pos + 1;
        }
    NEXT_A:;
    }

    m_values.resize(count);
    auto vals = m_values.data();
    for (auto ptr = src; ptr != eptr; ++ptr, ++vals) {
        vals->hash = hashStr(ptr->name, m_hashLen);
        vals->token.id = ptr->id;
        vals->token.name = ptr->name;
        vals->nameLen = (int)strlen(ptr->name);
    }

    // use a semi-prime number of slots so a questionable hash function
    // (such as some low bits usually 0) is less likely to cluster.
    size_t num = 2 * count + 1;

    m_byName.resize(num);
    Index * base = m_byName.data();
    for (auto i = 0; (size_t) i < m_values.size(); ++i) {
        auto & val = m_values[i];
        size_t pos = val.hash % num;
        Index ndx = { i, 0 };
        for (;;) {
            Index & tmp = base[pos];
            if (tmp.distance == -1) {
                tmp = ndx;
                break;
            }
            auto & tval = m_values[tmp.pos];
            if (tval.hash == val.hash
                && strcmp(tval.token.name, val.token.name) == 0
            ) {
                break;
            }
            if (ndx.distance > tmp.distance)
                swap(ndx, tmp);
            if (++pos == num)
                pos = 0;
            ndx.distance += 1;
        }
    }

    m_byId.resize(num);
    base = m_byId.data();
    for (auto i = 0; (size_t) i < m_values.size(); ++i) {
        auto & val = m_values[i];
        size_t pos = val.token.id;
        Index ndx = { i, 0 };
        for (;;) {
            pos %= num;
            Index & tmp = base[pos];
            if (tmp.distance == -1) {
                tmp = ndx;
                break;
            }
            auto & tval = m_values[tmp.pos];
            if (tval.token.id == val.token.id)
                break;
            if (ndx.distance > tmp.distance)
                swap(ndx, tmp);
            pos += 1;
            ndx.distance += 1;
        }
    }
}

//===========================================================================
bool TokenTable::find(int * out, std::string_view name) const {
    return find(out, name.data(), name.size());
}

//===========================================================================
bool TokenTable::find(int * out, const char name[], size_t nameLen) const {
    size_t num = size(m_byName);
    size_t hash = hashStr(name, (int) min((size_t) m_hashLen, nameLen));
    size_t pos = hash % num;
    int distance = 0;
    for (;;) {
        const Index & ndx = m_byName[pos];
        if (distance > ndx.distance)
            break;
        const Value & val = m_values[ndx.pos];
        if (val.hash == hash
            && (size_t) val.nameLen <= nameLen
            && strncmp(val.token.name, name, nameLen) == 0
        ) {
            *out = val.token.id;
            return true;
        }
        if (++pos == num)
            pos = 0;
        distance += 1;
    }
    *out = numeric_limits<remove_reference<decltype(*out)>::type>::max();
    return false;
}

//===========================================================================
bool TokenTable::find(const char ** const out, int id) const {
    size_t num = size(m_byId);
    size_t pos = id % num;
    int distance = 0;
    for (;;) {
        const Index & ndx = m_byId[pos];
        if (distance > ndx.distance)
            break;
        const Value & val = m_values[ndx.pos];
        if (val.token.id == id) {
            *out = val.token.name;
            return true;
        }
        if (++pos == num)
            pos = 0;
        distance += 1;
    }
    *out = nullptr;
    return false;
}

//===========================================================================
TokenTable::Iterator TokenTable::begin() const {
    return Iterator{&m_values.begin()->token};
}

//===========================================================================
TokenTable::Iterator TokenTable::end() const {
    return Iterator{&m_values.end()->token};
}


/****************************************************************************
*
*   TokenTable::Iterator
*
***/

//===========================================================================
TokenTable::Iterator::Iterator(const Token * ptr)
    : m_current{ptr}
{}

//===========================================================================
TokenTable::Iterator & TokenTable::Iterator::operator++() {
    auto ptr = reinterpret_cast<const Value *>(m_current);
    m_current = reinterpret_cast<const Token *>(ptr + 1);
    return *this;
}
