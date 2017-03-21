// tokentable.cpp - dim services
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
    // use a prime-ish number of slots so a questionable hash function
    // (such as some low bits usually 0) is less likely to cluster.
    size_t num = 2 * count + 1;

    m_names.resize(num);
    if (num < 2) {
        if (num == 1) {
            Value & val = m_names.front();
            val.hash = hashStr(src->name);
            val.id = src->id;
            val.name = src->name;
            val.nameLen = (int)strlen(src->name);
            m_hashLen = val.nameLen;
            m_ids.push_back(val);
        }
        return;
    }

    m_hashLen = 0;
    const Token * eptr = src + count;
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

    Value * base = m_names.data();
    for (const Token * a = src; a != eptr; ++a) {
        Value val;
        val.hash = hashStr(a->name, m_hashLen);
        val.id = a->id;
        val.name = a->name;
        val.nameLen = (int)strlen(a->name);
        val.distance = 0;
        size_t pos = val.hash;
        for (;;) {
            pos %= num;
            Value & tmp = base[pos];
            if (!tmp.name) {
                tmp = val;
                break;
            }
            if (tmp.hash == val.hash && strcmp(tmp.name, val.name) == 0)
                break;
            if (val.distance > tmp.distance)
                swap(val, tmp);
            pos += 1;
            val.distance += 1;
        }
    }

    m_ids.resize(num);
    base = m_ids.data();
    for (const Token * a = src; a != eptr; ++a) {
        Value val;
        val.id = a->id;
        val.name = a->name;
        val.nameLen = (int) strlen(a->name);
        val.distance = 0;
        size_t pos = val.id;
        for (;;) {
            pos %= num;
            Value & tmp = base[pos];
            if (!tmp.name) {
                tmp = val;
                break;
            }
            if (val.id == a->id)
                break;
            if (val.distance > tmp.distance)
                swap(val, tmp);
            pos += 1;
            val.distance += 1;
        }
    }
}

//===========================================================================
bool TokenTable::find(int * out, const char name[], size_t nameLen) const {
    size_t num = size(m_names);
    size_t hash = hashStr(name, (int) min((size_t) m_hashLen, nameLen));
    size_t pos = hash % num;
    int distance = 0;
    for (;;) {
        const Value & val = m_names[pos];
        if (distance > val.distance)
            break;
        if (val.hash == hash 
            && val.nameLen <= nameLen
            && strncmp(val.name, name, nameLen) == 0
        ) {
            *out = val.id;
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
bool TokenTable::find(char const ** const out, int id) const {
    size_t num = size(m_ids);
    size_t pos = id % num;
    int distance = 0;
    for (;;) {
        const Value & val = m_ids[pos];
        if (distance > val.distance)
            break;
        if (val.id == id) {
            *out = val.name;
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
    return Iterator{nullptr};
}

//===========================================================================
TokenTable::Iterator TokenTable::end() const {
    return Iterator{nullptr};
}
