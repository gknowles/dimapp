// Copyright Glen Knowles 2017 - 2021.
// Distributed under the Boost Software License, Version 1.0.
//
// algo.h - dim core
#pragma once

#include "cppconf/cppconf.h"

#include <iterator> // std::*_iterator_tag
#include <utility> // std::move

namespace Dim {


/****************************************************************************
*
*   Algorithms
*
***/

//===========================================================================
// The ranges must be sorted according to operator<
template<typename Input1, typename Input2, typename Add, typename Remove>
void for_each_diff(
    Input1 dst, Input1 edst,  // values to update to
    Input2 src, Input2 esrc,  // values to update from
    Add add,        // unary function called for each value in dst but not src
    Remove remove   // unary function called for each in src but not dst
) {
    if (src == esrc)
        goto REST_OF_DST;
    if (dst == edst)
        goto REST_OF_SRC;
    for (;;) {
        while (*dst < *src) {
        MORE_DST:
            add(*dst);
            if (++dst == edst)
                goto REST_OF_SRC;
        }
        while (*src < *dst) {
            remove(*src);
            if (++src == esrc)
                goto REST_OF_DST;
        }
        if (*dst < *src)
            goto MORE_DST;

        ++src, ++dst;
        if (src == esrc) {
            if (dst == edst)
                return;
            goto REST_OF_DST;
        }
        if (dst == edst)
            goto REST_OF_SRC;
    }

REST_OF_DST:
    for (;;) {
        add(*dst);
        if (++dst == edst)
            return;
    }

REST_OF_SRC:
    for (;;) {
        remove(*src);
        if (++src == esrc)
            return;
    }
}

//===========================================================================
template<typename Container, typename Pred>
void erase_unordered_if(Container c, Pred p) {
    auto i = c.begin(),
        ei = c.end();
    while (i != ei) {
        if (!p(*i)) {
            i += 1;
        } else {
            if (--ei == i)
                break;
            *i = std::move(*ei);
        }
    }
    c.erase(ei, c.end());
}


/****************************************************************************
*
*   Aligned intervals
*
***/

class AlignedIntervalGen {
public:
    class Iter;

    using value_type = std::pair<size_t, size_t>;
    using difference_type = ptrdiff_t;

public:
    AlignedIntervalGen(size_t start, size_t count, size_t interval);

    Iter begin() const;
    Iter end() const;

private:
    size_t m_start = 0;
    size_t m_count = 0;
    size_t m_interval = 0;
};

class AlignedIntervalGen::Iter {
public:
    using iterator_category = std::bidirectional_iterator_tag;
    using value_type = AlignedIntervalGen::value_type;
    using difference_type = AlignedIntervalGen::difference_type;
    using pointer = const value_type*;
    using reference = value_type;

public:
    Iter(const Iter & from) = default;
    Iter(const AlignedIntervalGen * cont, bool endmark);
    Iter & operator++();

    explicit operator bool() const { return (bool) m_value.second; }
    bool operator==(const Iter & other) const;
    value_type operator*() const { return m_value; }
    const value_type * operator->() const { return &m_value; }

private:
    const AlignedIntervalGen & m_cont;
    value_type m_value = {};
};

//===========================================================================
inline AlignedIntervalGen::AlignedIntervalGen(
    size_t start,
    size_t count,
    size_t interval
)
    : m_start(start)
    , m_count(count)
    , m_interval(interval)
{}

//===========================================================================
inline AlignedIntervalGen::Iter AlignedIntervalGen::begin() const {
    return Iter(this, false);
}

//===========================================================================
inline AlignedIntervalGen::Iter AlignedIntervalGen::end() const {
    return Iter(this, true);
}

//===========================================================================
inline AlignedIntervalGen::Iter::Iter(
    const AlignedIntervalGen * cont,
    bool endmark
)
    : m_cont(*cont)
{
    if (!endmark) {
        m_value.first = cont->m_start;
        m_value.second = std::min(
            cont->m_interval - cont->m_start % cont->m_interval,
            cont->m_count
        );
    }
}

//===========================================================================
inline AlignedIntervalGen::Iter & AlignedIntervalGen::Iter::operator++() {
    m_value.first += m_value.second;
    auto end = m_cont.m_start + m_cont.m_count;
    if (auto remaining = end - m_value.first) {
        m_value.second = std::min(m_cont.m_interval, remaining);
    } else {
        // convert to end sentinel
        m_value = {};
    }
    return *this;
}

//===========================================================================
inline bool AlignedIntervalGen::Iter::operator==(const Iter & other) const {
    return m_value == other.m_value;
}

} // namespace
