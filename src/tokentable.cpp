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
    size_t num = pow2Ceil(2 * count);
    m_names.resize(num);
    if (num < 2) {
        if (num == 1) {
            Value & val = m_names.front();
            m_hashLen = (int)strlen(src->name);
            val.hash = hashStr(src->name);
            val.id = src->id;
            val.name = src->name;
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
                    goto next_a;
                pos += 1;
            }
            if (pos >= m_hashLen)
                m_hashLen = pos + 1;
        }
    next_a:;
    }

    Value * base = m_names.data();
    for (const Token * a = src; a != eptr; ++a) {
        size_t hash = hashStr(a->name, m_hashLen);
        size_t pos = hash;
        for (;;) {
            pos %= num;
            Value & val = base[pos];
            if (!val.name) {
                val.hash = hash;
                val.id = a->id;
                val.name = a->name;
                break;
            }
            if (val.hash == hash && !strcmp(val.name, a->name))
                break;
            pos += 1;
        }
    }
}

//===========================================================================
bool TokenTable::find(int * out, const char name[]) const {
    size_t hash = hashStr(name, m_hashLen);
    size_t pos = hash;
    size_t num = size(m_names);
    for (;;) {
        pos %= num;
        const Value & val = m_names[pos];
        if (!val.name)
            break;
        if (val.hash == hash && !strcmp(val.name, name)) {
            *out = val.id;
            return true;
        }
        pos += 1;
    }
    *out = numeric_limits<remove_reference<decltype(*out)>::type>::max();
    return false;
}

//===========================================================================
bool TokenTable::find(const char * const * out, int id) const {
    assert(0);
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
