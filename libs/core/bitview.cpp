// Copyright Glen Knowles 2017 - 2019.
// Distributed under the Boost Software License, Version 1.0.
//
// bitview.cpp - dim core
#include "pch.h"
#pragma hdrstop

using namespace std;
using namespace Dim;


/****************************************************************************
*
*   Helpers
*
***/

//===========================================================================
static constexpr uint64_t bitmask(size_t bitpos) {
    return bswap64(
        1ull << (BitView::kWordBits - bitpos % BitView::kWordBits - 1)
    );
}


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
bool BitView::operator==(const BitView & right) const {
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
    return m_data[pos] & bitmask(bitpos);
}

//===========================================================================
uint64_t BitView::get(size_t bitpos, size_t bitcount) const {
    assert(bitcount <= kWordBits);
    auto pos = bitpos / kWordBits;
    if (pos >= m_size)
        return 0;
    auto bit = bitpos % kWordBits;
    auto bits = kWordBits - bit; // bits in first word after bitpos
    auto data = ntoh64(m_data + pos);
    if (bits >= bitcount) {
        if (bits == kWordBits) {
            return data;
        } else {
            auto mask = ((uint64_t) 1 << bitcount) - 1;
            return (data >> (bits - bitcount)) & mask;
        }
    } else {
        auto mask = ((uint64_t) 1 << bits) - 1;
        bits = bitcount - bits; // bits of value in second word
        auto out = (data & mask) << bits;
        if (++pos == m_size)
            return out;
        auto trailing = kWordBits - bits;
        auto out2 = ntoh64(m_data + pos) >> trailing;
        return out | out2;
    }
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
    memset(m_data, 0xff, m_size * sizeof *m_data);
    return *this;
}

//===========================================================================
BitView & BitView::set(size_t bitpos) {
    auto pos = bitpos / kWordBits;
    assert(pos < m_size);
    m_data[pos] |= bitmask(bitpos);
    return *this;
}

//===========================================================================
BitView & BitView::set(size_t bitpos, size_t bitcount, uint64_t value) {
    auto pos = bitpos / kWordBits;
    assert(pos < m_size);
    assert(!bitcount || (bitpos + bitcount - 1) / kWordBits < m_size);
    assert(bitcount == kWordBits || value < ((uint64_t) 1 << bitcount));
    auto bit = bitpos % kWordBits;
    auto bits = kWordBits - bit;
    if (bits >= bitcount) {
        if (bits == kWordBits) {
            hton64(m_data + pos, value);
        } else {
            auto mask = ((uint64_t) 1 << bitcount) - 1;
            auto trailing = bits - bitcount;
            mask <<= trailing;
            value <<= trailing;
            auto ptr = m_data + pos;
            hton64(ptr, ntoh64(ptr) & ~mask | value);
        }
    } else {
        auto mask = ((uint64_t) 1 << bits) - 1;
        auto ptr = m_data + pos;
        hton64(ptr, ntoh64(ptr) & ~mask | (value >> bit) & mask);
        bits = bitcount - bits;
        mask = ((uint64_t) 1 << bits) - 1;
        auto trailing = kWordBits - bits;
        mask <<= trailing;
        value <<= trailing;
        ptr += 1;
        hton64(ptr, ntoh64(ptr) & ~mask | value & mask);
    }
    return *this;
}

//===========================================================================
BitView & BitView::reset() {
    memset(m_data, 0, m_size * sizeof *m_data);
    return *this;
}

//===========================================================================
BitView & BitView::reset(size_t bitpos) {
    auto pos = bitpos / kWordBits;
    assert(pos < m_size);
    m_data[pos] &= ~bitmask(bitpos);
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
    m_data[pos] ^= bitmask(bitpos);
    return *this;
}

//===========================================================================
bool BitView::testAndSet(size_t bitpos) {
    auto pos = bitpos / kWordBits;
    assert(pos < m_size);
    auto mask = bitmask(bitpos);
    auto tmp = m_data[pos];
    m_data[pos] |= mask;
    return tmp & mask;
}

//===========================================================================
bool BitView::testAndReset(size_t bitpos) {
    auto pos = bitpos / kWordBits;
    assert(pos < m_size);
    auto mask = bitmask(bitpos);
    auto tmp = m_data[pos];
    m_data[pos] &= ~mask;
    return tmp & mask;
}

//===========================================================================
bool BitView::testAndFlip(size_t bitpos) {
    auto pos = bitpos / kWordBits;
    assert(pos < m_size);
    auto mask = bitmask(bitpos);
    auto tmp = m_data[pos];
    m_data[pos] ^= mask;
    return tmp & mask;
}

//===========================================================================
uint64_t BitView::word(size_t bitpos) const {
    auto pos = bitpos / kWordBits;
    assert(pos < m_size);
    return m_data[pos];
}

//===========================================================================
static constexpr size_t leadingPos(size_t pos, uint64_t data) {
    return pos * BitView::kWordBits + leadingZeroBits(data);
}

//===========================================================================
size_t BitView::find(size_t bitpos) const {
    auto pos = bitpos / kWordBits;
    if (pos >= m_size)
        return npos;
    auto ptr = m_data + pos;
    if (auto val = ntoh64(ptr) & (kWordMax >> bitpos % kWordBits))
        return leadingPos(pos, val);
    auto last = m_data + m_size;
    while (++ptr != last) {
        if (auto val = *ptr)
            return leadingPos(ptr - m_data, ntoh64(&val));
    }
    return npos;
}

//===========================================================================
size_t BitView::findZero(size_t bitpos) const {
    auto pos = bitpos / kWordBits;
    if (pos >= m_size)
        return npos;
    auto ptr = m_data + pos;
    if (auto val = ~ntoh64(ptr) & (kWordMax >> bitpos % kWordBits))
        return leadingPos(pos, val);
    auto last = m_data + m_size;
    while (++ptr != last) {
        if (auto val = ~*ptr)
            return leadingPos(ptr - m_data, ntoh64(&val));
    }
    return npos;
}

//===========================================================================
static constexpr size_t trailingPos(size_t pos, uint64_t data) {
    return ++pos * BitView::kWordBits - 1 - trailingZeroBits(data);
}

//===========================================================================
size_t BitView::rfind(size_t bitpos) const {
    auto pos = bitpos / kWordBits;
    auto last = m_data + pos;
    if (pos < m_size) {
        auto mask = kWordMax << (kWordBits - bitpos % kWordBits - 1);
        if (auto val = ntoh64(last) & mask)
            return trailingPos(pos, val);
    } else {
        last = m_data + m_size;
    }
    while (m_data != last--) {
        if (auto val = *last)
            return trailingPos(last - m_data, ntoh64(&val));
    }
    return npos;
}

//===========================================================================
size_t BitView::rfindZero(size_t bitpos) const {
    auto pos = bitpos / kWordBits;
    auto last = m_data + pos;
    if (pos < m_size) {
        if (auto val = ~ntoh64(last) & ~(kWordMax >> bitpos % kWordBits))
            return trailingPos(pos, val);
    } else {
        last = m_data + m_size;
    }
    while (m_data != last--) {
        if (auto val = ~*last)
            return trailingPos(last - m_data, ntoh64(&val));
    }
    return npos;
}
