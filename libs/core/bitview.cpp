// Copyright Glen Knowles 2017 - 2022.
// Distributed under the Boost Software License, Version 1.0.
//
// bitview.cpp - dim core
#include "pch.h"
#pragma hdrstop

using namespace std;
using namespace Dim;


/****************************************************************************
*
*   Declarations
*
***/

namespace {

enum OpType {
    kReset,
    kSet,
    kFlip,
    kCount,
};

} // namespace


/****************************************************************************
*
*   Helpers
*
***/

//===========================================================================
static constexpr uint64_t bitmask(size_t bitpos) {
    auto mask = 1ull << (BitView::kWordBits - bitpos % BitView::kWordBits - 1);
    if constexpr (endian::native == endian::little)
        mask = bswap64(mask);
    return mask;
}

//===========================================================================
template<OpType Op, typename Word>
static void apply(Word * ptr, size_t * count, Word mask) {
    if constexpr (Op == kReset) {
        *ptr &= ~mask;
    } else if constexpr (Op == kSet) {
        *ptr |= mask;
    } else if constexpr (Op == kFlip) {
        *ptr ^= mask;
    } else {
        static_assert(Op == kCount);
        *count += popcount(*ptr & mask);
    }
}

//===========================================================================
template<OpType Op, typename Word>
static void apply(
    Word * data,
    size_t dataLen,
    size_t * count,
    size_t bitpos,
    size_t bitcount
) {
    constexpr size_t kWordBits = sizeof (Word) * 8;
    constexpr Word kWordMax = numeric_limits<Word>::max();

    if (bitcount == dynamic_extent)
        bitcount = dataLen * kWordBits - bitpos + 1;
    auto pos = bitpos / kWordBits;
    assert(pos < dataLen);
    assert(bitcount <= (dataLen - pos) * kWordBits);
    auto ptr = data + pos;
    auto bit = bitpos % kWordBits;
    auto bits = kWordBits - bit;
    if (bits >= bitcount) {
        if (bitcount == kWordBits) {
            apply<Op>(ptr, count, kWordMax);
        } else {
            auto mask = ((Word) 1 << bitcount) - 1;
            mask <<= bits - bitcount;
            if constexpr (std::endian::native == std::endian::little)
                mask = bswap64(mask);
            apply<Op>(ptr, count, mask);
        }
    } else {
        if (bits < kWordBits) {
            auto mask = ((Word) 1 << bits) - 1;
            if constexpr (std::endian::native == std::endian::little)
                mask = bswap64(mask);
            apply<Op>(ptr, count, mask);
            bitcount -= bits;
            ptr += 1;
        }
        for (; bitcount >= kWordBits; ++ptr, bitcount -= kWordBits) {
            apply<Op>(ptr, count, kWordMax);
        }
        if (bitcount) {
            auto mask = ((Word) 1 << bitcount) - 1;
            mask <<= kWordBits - bitcount;
            if constexpr (std::endian::native == std::endian::little)
                mask = bswap64(mask);
            apply<Op>(ptr, count, mask);
        }
    }
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
BitView BitView::view(size_t wordOffset, size_t wordCount) const {
    assert(wordOffset < m_size);
    wordCount = min(wordCount, m_size - wordOffset);
    return {m_data + wordOffset, wordCount};
}

//===========================================================================
BitView & BitView::remove_prefix(size_t wordCount) {
    assert(wordCount <= m_size);
    m_data += wordCount;
    m_size -= wordCount;
    return *this;
}

//===========================================================================
BitView & BitView::remove_suffix(size_t wordCount) {
    assert(wordCount <= m_size);
    m_size -= wordCount;
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
uint64_t BitView::getBits(size_t bitpos, size_t bitcount) const {
    assert(bitcount <= kWordBits);
    auto pos = bitpos / kWordBits;
    if (pos >= m_size)
        return 0;
    auto bit = bitpos % kWordBits;
    auto bits = kWordBits - bit; // bits in first word after bitpos
    auto data = ntoh64(m_data + pos);
    if (bits >= bitcount) {
        if (bitcount == kWordBits) {
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
        [](auto a, auto b) { return a + popcount(b); }
    );
}

//===========================================================================
size_t BitView::count(size_t bitpos, size_t bitcount) const {
    size_t count = 0;
    ::apply<kCount, uint64_t>(m_data, m_size, &count, bitpos, bitcount);
    return count;
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
BitView & BitView::set(size_t bitpos, size_t bitcount) {
    ::apply<kSet, uint64_t>(m_data, m_size, nullptr, bitpos, bitcount);
    return *this;
}

//===========================================================================
BitView & BitView::setBits(size_t bitpos, size_t bitcount, uint64_t value) {
    auto pos = bitpos / kWordBits;
    assert(pos < m_size);
    assert(!bitcount || (bitpos + bitcount - 1) / kWordBits < m_size);
    assert(bitcount == kWordBits || value < ((uint64_t) 1 << bitcount));
    auto ptr = m_data + pos;
    auto bit = bitpos % kWordBits;
    auto bits = kWordBits - bit;
    if (bits >= bitcount) {
        if (bitcount == kWordBits) {
            hton64(ptr, value);
        } else {
            auto mask = ((uint64_t) 1 << bitcount) - 1;
            auto trailing = bits - bitcount;
            mask <<= trailing;
            value <<= trailing;
            hton64(ptr, ntoh64(ptr) & ~mask | value);
        }
    } else {
        auto mask = ((uint64_t) 1 << bits) - 1;
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
BitView & BitView::reset(size_t bitpos, size_t bitcount) {
    ::apply<kReset, uint64_t>(m_data, m_size, nullptr, bitpos, bitcount);
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
BitView & BitView::flip(size_t bitpos, size_t bitcount) {
    ::apply<kFlip, uint64_t>(m_data, m_size, nullptr, bitpos, bitcount);
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
    return *data(bitpos);
}

//===========================================================================
uint64_t * BitView::data(size_t bitpos) {
    auto pos = bitpos / kWordBits;
    assert(pos < m_size);
    return m_data + pos;
}

//===========================================================================
const uint64_t * BitView::data(size_t bitpos) const {
    return const_cast<BitView *>(this)->data(bitpos);
}

//===========================================================================
static constexpr size_t leadingPos(size_t pos, uint64_t data) {
    return pos * BitView::kWordBits + countl_zero(data);
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
    return ++pos * BitView::kWordBits - 1 - countr_zero(data);
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
