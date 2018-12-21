// Copyright Glen Knowles 2017.
// Distributed under the Boost Software License, Version 1.0.
//
// bitview.cpp - dim core
#include "pch.h"
#pragma hdrstop

using namespace std;
using namespace Dim;


/****************************************************************************
*
*   BitView
*
***/

//===========================================================================
BitView::BitView(uint64_t * src, size_t srcLen)
    : m_data{src}
    , m_size{srcLen}
{}

//===========================================================================
bool BitView::operator==(BitView const & right) const {
    return m_size == right.m_size
        && memcmp(m_data, right.m_data, m_size) == 0;
}

//===========================================================================
BitView & BitView::remove_prefix(size_t numUint64) {
    assert(numUint64 <= m_size);
    m_data += numUint64;
    m_size -= numUint64;
    return *this;
}

//===========================================================================
BitView & BitView::remove_suffix(size_t numUint64) {
    assert(numUint64 <= m_size);
    m_size -= numUint64;
    return *this;
}

//===========================================================================
bool BitView::operator[](size_t bitpos) const {
    auto pos = bitpos / kWordBits;
    if (pos >= m_size)
        return false;
    return m_data[pos] & ((uint64_t) 1 << bitpos % kWordBits);
}

//===========================================================================
bool BitView::all() const {
    for (auto i = 0; (size_t) i < m_size; ++i) {
        if (m_data[i] != kWordMax)
            return false;
    }
    return true;
}

//===========================================================================
bool BitView::any() const {
    return !none();
}

//===========================================================================
bool BitView::none() const {
    for (auto i = 0; (size_t) i < m_size; ++i) {
        if (m_data[i])
            return false;
    }
    return true;
}

//===========================================================================
size_t BitView::count() const {
    return accumulate(
        m_data,
        m_data + m_size,
        0,
        [](auto a, auto b) { return a + hammingWeight(b); }
    );
}

//===========================================================================
BitView & BitView::set() {
    memset(m_data, 0xff, m_size * sizeof(*m_data));
    return *this;
}

//===========================================================================
BitView & BitView::set(size_t bitpos) {
    auto pos = bitpos / kWordBits;
    assert(pos < m_size);
    m_data[pos] |= ((uint64_t) 1 << bitpos % kWordBits);
    return *this;
}

//===========================================================================
BitView & BitView::set(size_t bitpos, size_t bitcount, uint64_t value) {
    auto pos = bitpos / kWordBits;
    assert((bitpos + bitcount) / kWordBits < m_size);
    assert(bitcount == kWordBits || value < ((uint64_t) 1 << bitcount));
    auto bit = bitpos % kWordBits;
    auto bits = kWordBits - bit;
    if (bits >= bitcount) {
        if (bits == kWordBits) {
            m_data[pos] = value;
        } else {
            auto mask = ((uint64_t) 1 << bitcount) - 1;
            mask <<= bit;
            m_data[pos] = m_data[pos] & ~mask | (value << bit) & mask;
        }
    } else {
        auto mask = ((uint64_t) 1 << bits) - 1;
        mask <<= bit;
        m_data[pos] = m_data[pos] & ~mask | value & mask;
        value >>= bit;
        mask = ~mask;
        m_data[pos + 1] = m_data[pos + 1] & mask | value & ~mask;
    }
    return *this;
}

//===========================================================================
BitView & BitView::reset() {
    memset(m_data, 0, m_size * sizeof(*m_data));
    return *this;
}

//===========================================================================
BitView & BitView::reset(size_t bitpos) {
    auto pos = bitpos / kWordBits;
    assert(pos < m_size);
    m_data[pos] &= ~((uint64_t) 1 << bitpos % kWordBits);
    return *this;
}

//===========================================================================
BitView & BitView::flip() {
    for (auto i = 0; (size_t) i < m_size; ++i)
        m_data[i] = ~m_data[i];
    return *this;
}

//===========================================================================
BitView & BitView::flip(size_t bitpos) {
    auto pos = bitpos / kWordBits;
    assert(pos < m_size);
    m_data[pos] ^= ((uint64_t) 1 << bitpos % kWordBits);
    return *this;
}

//===========================================================================
size_t BitView::find(size_t bitpos) {
    auto pos = bitpos / kWordBits;
    if (pos >= m_size)
        return npos;
    auto ptr = m_data + pos;
    if (auto val = *ptr & (kWordMax << bitpos % kWordBits))
        return pos * kWordBits + trailingZeroBits(val);
    auto last = m_data + m_size;
    while (++ptr != last) {
        if (auto val = *ptr)
            return (ptr - m_data) * kWordBits + trailingZeroBits(val);
    }
    return npos;
}

//===========================================================================
size_t BitView::findZero(size_t bitpos) {
    auto pos = bitpos / kWordBits;
    if (pos >= m_size)
        return npos;
    auto ptr = m_data + pos;
    if (auto val = ~*ptr & (kWordMax << bitpos % kWordBits))
        return pos * kWordBits + trailingZeroBits(val);
    auto last = m_data + m_size;
    while (++ptr != last) {
        if (auto val = ~*ptr)
            return (ptr - m_data) * kWordBits + trailingZeroBits(val);
    }
    return npos;
}

//===========================================================================
size_t BitView::rfind(size_t bitpos) {
    auto pos = bitpos / kWordBits;
    if (pos < m_size) {
        if (auto val = m_data[pos] & ~(kWordMax << bitpos % kWordBits))
            return pos * kWordBits + leadingZeroBits(val);
    } else {
        pos = m_size;
    }
    auto last = m_data + pos;
    while (m_data != last--) {
        if (auto val = *last)
            return (last - m_data) * kWordBits + leadingZeroBits(val);
    }
    return npos;
}

//===========================================================================
size_t BitView::rfindZero(size_t bitpos) {
    auto pos = bitpos / kWordBits;
    if (pos < m_size) {
        if (auto val = ~m_data[pos] & ~(kWordMax << bitpos % kWordBits))
            return pos * kWordBits + leadingZeroBits(val);
    } else {
        pos = m_size;
    }
    auto last = m_data + pos;
    while (m_data != last--) {
        if (auto val = ~*last)
            return (last - m_data) * kWordBits + leadingZeroBits(val);
    }
    return npos;
}
