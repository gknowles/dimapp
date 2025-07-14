// Copyright Glen Knowles 2017 - 2024.
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
        mask = byteswap(mask);
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
    constexpr size_t kWordBits = sizeof(Word) * 8;
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
            apply<Op, Word>(ptr, count, kWordMax);
        } else {
            auto mask = ((Word) 1 << bitcount) - 1;
            mask <<= bits - bitcount;
            if constexpr (std::endian::native == std::endian::little)
                mask = byteswap(mask);
            apply<Op, Word>(ptr, count, mask);
        }
    } else {
        if (bits < kWordBits) {
            auto mask = ((Word) 1 << bits) - 1;
            if constexpr (std::endian::native == std::endian::little)
                mask = byteswap(mask);
            apply<Op, Word>(ptr, count, mask);
            bitcount -= bits;
            ptr += 1;
        }
        for (; bitcount >= kWordBits; ++ptr, bitcount -= kWordBits) {
            apply<Op, Word>(ptr, count, kWordMax);
        }
        if (bitcount) {
            auto mask = ((Word) 1 << bitcount) - 1;
            mask <<= kWordBits - bitcount;
            if constexpr (std::endian::native == std::endian::little)
                mask = byteswap(mask);
            apply<Op, Word>(ptr, count, mask);
        }
    }
}




/****************************************************************************
*
*   IBitView
*
***/

//===========================================================================
// static
void IBitView::copy(
    void * vdst,
    size_t dpos,
    void * vsrc,
    size_t spos,
    size_t cnt
) {
    if (!cnt)
        return;
    auto dst = (uint8_t *) vdst;
    if (dpos >= 8) {
        dst += dpos / 8;
        dpos %= 8;
    }
    auto src = (uint8_t *) vsrc;
    if (spos >= 8) {
        src += spos / 8;
        spos %= 8;
    }

    if (dpos + cnt <= 8) {
        // dst is only a single byte.
        auto mask = uint8_t(255 >> (8 - cnt) << (8 - dpos - cnt));
        if (spos + cnt <= 8) {
            // From one byte to another.
            auto val = *src;
            if (int d = int(spos - dpos)) {
                if (d > 0) {
                    val >>= d;
                } else {
                    val <<= -d;
                }
            }
            *dst = val & mask | *dst & ~mask;
            return;
        }
        // From two bytes to one.
        assert(spos > dpos);
        auto val = *src << (spos - dpos) | src[1] >> (8 - cnt + 8 - spos);
        *dst = val & mask | *dst & ~mask;
        return;
    }

    if (dpos == spos) {
        // src and dst are bit aligned.
        if (dpos) {
            auto mask = 255 >> dpos;
            auto val = *src++;
            *dst++ = val & mask | *dst & ~mask;
            cnt -= 8 - dpos;
        }
        if (cnt % 8 == 0) {
            // src and dst are fully *byte* aligned.
            if (cnt)
                memcpy(dst, src, cnt / 8);
            return;
        }
        if (cnt >= 8) {
            memcpy(dst, src, cnt / 8);
            src += cnt / 8;
            dst += cnt / 8;
            cnt %= 8;
        }
        auto mask = 255 << (8 - cnt);
        *dst = *src & mask | *dst & ~mask;
        return;
    }

    // src and dst start at unaligned bits.
    if (dpos > spos) {
        // From part of one byte to first dst byte.
        //   spos 2      dpos 4
        //   00aaaabb -> 0000aaaa
        auto mask = uint8_t(255 >> dpos);
        auto val = *src++ >> (dpos - spos);
        *dst++ = val & mask | *dst & ~mask;
        spos = 8 - (dpos - spos);
    } else {
        assert(dpos < spos);
        // From one byte and part of another to first dst byte.
        //   spos 5                 dpos 3
        //   00000aaa + bbcccccc -> 000aaabb
        auto mask = uint8_t(255 >> dpos);
        auto val = *src++ << (spos - dpos) | src[1] >> (8 - spos + dpos);
        *dst++ = val & mask | *dst & ~mask;
        spos = 8 - (spos - dpos);
    }
    cnt -= 8 - dpos;
    for (; cnt >= 8; cnt -= 8) {
        *dst++ = *src++ << spos | src[1] >> (8 - spos);
    }
    if (cnt) {
        auto mask = uint8_t(255 < (8 - cnt));
        if (cnt <= 8 - spos ) {
            // From part of one byte to front of last dst byte.
            auto val = *src << spos;
            *dst = val & mask | *dst & ~mask;
        } else {
            // From two bytes to last dst byte.
            auto val = *src << spos | src[1] >> (8 - spos);
            *dst = val & mask | *dst & ~mask;
        }
    }
}

//===========================================================================
bool IBitView::operator==(const IBitView & right) const {
    return size() == right.size()
        && memcmp(data(), right.data(), size()) == 0;
}

//===========================================================================
bool IBitView::operator[](size_t bitpos) const {
    auto pos = bitpos / kWordBits;
    if (pos >= size())
        return false;
    return data()[pos] & bitmask(bitpos);
}

//===========================================================================
uint64_t IBitView::getBits(size_t bitpos, size_t bitcount) const {
    assert(bitcount <= kWordBits);
    auto pos = bitpos / kWordBits;
    if (pos >= size())
        return 0;
    auto bit = bitpos % kWordBits;
    auto bits = kWordBits - bit; // bits in first word after bitpos
    auto word = ntoh64(data() + pos);
    if (bits >= bitcount) {
        if (bitcount == kWordBits) {
            return word;
        } else {
            auto mask = ((uint64_t) 1 << bitcount) - 1;
            return (word >> (bits - bitcount)) & mask;
        }
    } else {
        auto mask = ((uint64_t) 1 << bits) - 1;
        bits = bitcount - bits; // bits of value in second word
        auto out = (word & mask) << bits;
        if (++pos == size())
            return out;
        auto trailing = kWordBits - bits;
        auto out2 = ntoh64(data() + pos) >> trailing;
        return out | out2;
    }
}

//===========================================================================
size_t IBitView::copy(
    void * vdst,
    size_t dpos,
    size_t cnt,
    size_t pos //= 0
) const {
    auto dst = (uint8_t *) vdst;
    if (dpos > 7) {
        dst += dpos / 8;
        dpos %= 8;
    }
    auto bits = size() * kWordBits;
    if (!dpos && !pos) {
        if (pos < bits) {
            if (cnt < bits - pos) {
                if (cnt % 8 == 0) {
                    //memcpy(
                    //return cnt;
                }
            }
        }
    }
    assert(!"Not implemented");
    return 0;
}

//===========================================================================
bool IBitView::all() const {
    for (auto&& word : words()) {
        if (word != kWordMax)
            return false;
    }
    return true;
}

//===========================================================================
bool IBitView::any() const {
    return !none();
}

//===========================================================================
bool IBitView::none() const {
    for (auto&& word : words()) {
        if (word)
            return false;
    }
    return true;
}

//===========================================================================
size_t IBitView::count() const {
    auto words = IBitView::words();
    return accumulate(
        words.begin(),
        words.end(),
        0,
        [](auto a, auto b) { return a + popcount(b); }
    );
}

//===========================================================================
size_t IBitView::count(size_t bitpos, size_t bitcount) const {
    size_t count = 0;
    ::apply<kCount, const uint64_t>(data(), size(), &count, bitpos, bitcount);
    return count;
}

//===========================================================================
uint64_t IBitView::word(size_t bitpos) const {
    return *data(bitpos);
}

//===========================================================================
const uint64_t * IBitView::data(size_t bitpos) const {
    auto pos = bitpos / kWordBits;
    assert(pos < size());
    return data() + pos;
}

//===========================================================================
std::span<const uint64_t> IBitView::words() const {
    return {data(), size()};
}

//===========================================================================
static constexpr size_t leadingPos(size_t pos, uint64_t data) {
    return pos * BitView::kWordBits + countl_zero(data);
}

//===========================================================================
size_t IBitView::find(size_t bitpos) const {
    auto pos = bitpos / kWordBits;
    if (pos >= size())
        return npos;
    auto ptr = data() + pos;
    if (auto val = ntoh64(ptr) & (kWordMax >> bitpos % kWordBits))
        return leadingPos(pos, val);
    auto last = data() + size();
    while (++ptr != last) {
        if (auto val = *ptr)
            return leadingPos(ptr - data(), ntoh64(&val));
    }
    return npos;
}

//===========================================================================
size_t IBitView::findZero(size_t bitpos) const {
    auto pos = bitpos / kWordBits;
    if (pos >= size())
        return npos;
    auto ptr = data() + pos;
    if (auto val = ~ntoh64(ptr) & (kWordMax >> bitpos % kWordBits))
        return leadingPos(pos, val);
    auto last = data() + size();
    while (++ptr != last) {
        if (auto val = ~*ptr)
            return leadingPos(ptr - data(), ntoh64(&val));
    }
    return npos;
}

//===========================================================================
static constexpr size_t trailingPos(size_t pos, uint64_t data) {
    return ++pos * BitView::kWordBits - 1 - countr_zero(data);
}

//===========================================================================
size_t IBitView::rfind(size_t bitpos) const {
    auto pos = bitpos / kWordBits;
    auto last = data() + pos;
    if (pos < size()) {
        auto mask = kWordMax << (kWordBits - bitpos % kWordBits - 1);
        if (auto val = ntoh64(last) & mask)
            return trailingPos(pos, val);
    } else {
        last = data() + size();
    }
    while (data() != last--) {
        if (auto val = *last)
            return trailingPos(last - data(), ntoh64(&val));
    }
    return npos;
}

//===========================================================================
size_t IBitView::rfindZero(size_t bitpos) const {
    auto pos = bitpos / kWordBits;
    auto last = data() + pos;
    if (pos < size()) {
        auto mask = kWordMax << (kWordBits - bitpos % kWordBits - 1);
        if (auto val = ~ntoh64(last) & mask)
            return trailingPos(pos, val);
    } else {
        last = data() + size();
    }
    while (data() != last--) {
        if (auto val = ~*last)
            return trailingPos(last - data(), ntoh64(&val));
    }
    return npos;
}


/****************************************************************************
*
*   BitView
*
***/

//===========================================================================
BitView::BitView(const uint64_t * src, size_t srcLen)
    : m_data{src}
    , m_size{srcLen}
{}

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


/****************************************************************************
*
*   BitSpan
*
***/

//===========================================================================
BitSpan::BitSpan(uint64_t * src, size_t srcLen)
    : m_data{src}
    , m_size{srcLen}
{}

//===========================================================================
BitSpan BitSpan::subspan(size_t wordOffset, size_t wordCount) const {
    assert(wordOffset < m_size);
    wordCount = min(wordCount, m_size - wordOffset);
    return {m_data + wordOffset, wordCount};
}

//===========================================================================
BitSpan & BitSpan::remove_prefix(size_t wordCount) {
    assert(wordCount <= m_size);
    m_data += wordCount;
    m_size -= wordCount;
    return *this;
}

//===========================================================================
BitSpan & BitSpan::remove_suffix(size_t wordCount) {
    assert(wordCount <= m_size);
    m_size -= wordCount;
    return *this;
}

//===========================================================================
BitSpan & BitSpan::set() {
    memset(m_data, 0xff, m_size * sizeof *m_data);
    return *this;
}

//===========================================================================
BitSpan & BitSpan::set(size_t bitpos) {
    auto pos = bitpos / kWordBits;
    assert(pos < m_size);
    m_data[pos] |= bitmask(bitpos);
    return *this;
}

//===========================================================================
BitSpan & BitSpan::set(size_t bitpos, size_t bitcount) {
    ::apply<kSet, uint64_t>(m_data, m_size, nullptr, bitpos, bitcount);
    return *this;
}

//===========================================================================
BitSpan & BitSpan::set(size_t pos, const void * src, size_t spos, size_t cnt) {
    assert(!"BitSpan::set not implemented");
    return *this;
}

//===========================================================================
BitSpan & BitSpan::set(
    size_t pos,
    const IBitView &src,
    size_t spos,
    size_t cnt // = npos
) {
    assert(!"BitSpan::set not implemented");
    return *this;
}

//===========================================================================
BitSpan & BitSpan::setBits(size_t bitpos, size_t bitcount, uint64_t value) {
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
BitSpan & BitSpan::reset() {
    memset(m_data, 0, m_size * sizeof *m_data);
    return *this;
}

//===========================================================================
BitSpan & BitSpan::reset(size_t bitpos) {
    auto pos = bitpos / kWordBits;
    assert(pos < m_size);
    m_data[pos] &= ~bitmask(bitpos);
    return *this;
}

//===========================================================================
BitSpan & BitSpan::reset(size_t bitpos, size_t bitcount) {
    ::apply<kReset, uint64_t>(m_data, m_size, nullptr, bitpos, bitcount);
    return *this;
}

//===========================================================================
BitSpan & BitSpan::flip() {
    for (auto i = 0; (size_t) i < m_size; ++i)
        m_data[i] = ~m_data[i];
    return *this;
}

//===========================================================================
BitSpan & BitSpan::flip(size_t bitpos) {
    auto pos = bitpos / kWordBits;
    assert(pos < m_size);
    m_data[pos] ^= bitmask(bitpos);
    return *this;
}

//===========================================================================
BitSpan & BitSpan::flip(size_t bitpos, size_t bitcount) {
    ::apply<kFlip, uint64_t>(m_data, m_size, nullptr, bitpos, bitcount);
    return *this;
}

//===========================================================================
bool BitSpan::testAndSet(size_t bitpos) {
    auto pos = bitpos / kWordBits;
    assert(pos < m_size);
    auto mask = bitmask(bitpos);
    auto tmp = m_data[pos];
    m_data[pos] |= mask;
    return tmp & mask;
}

//===========================================================================
bool BitSpan::testAndReset(size_t bitpos) {
    auto pos = bitpos / kWordBits;
    assert(pos < m_size);
    auto mask = bitmask(bitpos);
    auto tmp = m_data[pos];
    m_data[pos] &= ~mask;
    return tmp & mask;
}

//===========================================================================
bool BitSpan::testAndFlip(size_t bitpos) {
    auto pos = bitpos / kWordBits;
    assert(pos < m_size);
    auto mask = bitmask(bitpos);
    auto tmp = m_data[pos];
    m_data[pos] ^= mask;
    return tmp & mask;
}

//===========================================================================
uint64_t * BitSpan::data(size_t bitpos) {
    auto pos = bitpos / kWordBits;
    assert(pos < m_size);
    return m_data + pos;
}
