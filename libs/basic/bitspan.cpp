// Copyright Glen Knowles 2017 - 2025.
// Distributed under the Boost Software License, Version 1.0.
//
// bitview.cpp - dim basic
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
template<typename T, typename U>
constexpr static T combine(T dst, T src, U mask) {
    // Equivalent to: src & mask | dst & ~mask
    return (src ^ dst) & mask ^ dst;
}

//===========================================================================
static uint64_t ntoh64(const void * ptr, size_t bytes) {
    assert(bytes <= sizeof uint64_t);
    uint64_t out = 0;
    memcpy(&out, ptr, bytes);
    if constexpr (std::endian::native == std::endian::little)
        out = byteswap(out);
    return out;
}

//===========================================================================
static uint64_t ntoh64l(const void * ptr, size_t bytes) {
    assert(bytes <= sizeof uint64_t);
    uint64_t out = 0;
    memcpy((char *) &out + sizeof out - bytes, ptr, bytes);
    if constexpr (std::endian::native == std::endian::little)
        out = byteswap(out);
    return out;
}

//===========================================================================
// Only store some of the most significant bytes.
static void hton64(void * ptr, uint64_t val, size_t bytes) {
    assert(bytes <= sizeof val);
    if constexpr (std::endian::native == std::endian::little)
        val = byteswap(val);
    memcpy(ptr, &val, bytes);
}

//===========================================================================
// Only store some of the *least* significant bytes.
static void hton64l(void * ptr, uint64_t val, size_t bytes) {
    assert(bytes <= sizeof val);
    if constexpr (std::endian::native == std::endian::little)
        val = byteswap(val);
    memcpy(ptr, (char *) &val + sizeof val - bytes, bytes);
}

//===========================================================================
static void bitmoveAligned(
    unsigned char * dst,
    const unsigned char * src,
    size_t pos,
    size_t cnt
) {
    // The full bytes of src and dst are bit aligned, memcpy can be used.
    bool head = false;
    unsigned char hval = 0;
    unsigned char tval;
    if (pos) {
        // dst starts with a partial byte.
        if (pos + cnt < 8) {
            // dst is only a single byte.
            auto mask = UCHAR_MAX >> (8 - cnt) << (8 - pos - cnt);
            *dst = combine(*dst, *src, mask);
            return;
        }
        // Calculate partial byte head.
        head = true;
        hval = combine(*dst++, *src++, UCHAR_MAX >> pos);
        cnt -= 8 - pos;
    }
    if (cnt % 8 == 0) {
        // Remainder of src and dst are fully *byte* aligned.
        if (cnt) {
            auto bytes = cnt / 8;
            if (dst + bytes <= src || src + bytes <= dst) {
                memcpy(dst, src, bytes);
            } else {
                memmove(dst, src, bytes);
            }
        }
        if (head)
            dst[-1] = hval;
        return;
    }
    // Calculate partial byte tail.
    auto bytes = cnt / 8;
    tval = combine(dst[bytes], src[bytes], UCHAR_MAX << (8 - cnt % 8));
    if (bytes) {
        // Copy bytes that are bit aligned (the middle bytes).
        if (dst + bytes <= src || src + bytes <= dst) {
            memcpy(dst, src, bytes);
        } else {
            memmove(dst, src, bytes);
        }
    }
    // Store head (if present) and tail partial bytes.
    if (head)
        dst[-1] = hval;
    dst[cnt / 8] = tval;
    return;
}

//===========================================================================
[[maybe_unused]]
static void bitmove8(
    void * vdst,
    size_t dpos,
    const void * vsrc,
    size_t spos,
    size_t cnt
) {
    auto dst = (unsigned char *) vdst + dpos / 8;
    auto src = (const unsigned char *) vsrc + spos / 8;
    dpos %= 8;
    spos %= 8;

    if (spos == dpos)
        return bitmoveAligned(dst, src, spos, cnt);

    // Start at unaligned bits.
    if (dpos + cnt <= 8) {
        // dst is only a single byte.
        unsigned char mask = UCHAR_MAX >> (8 - cnt) << (8 - dpos - cnt);
        if (spos + cnt <= 8) {
            // From one byte to another.
            auto val = *src;
            if (spos > dpos) {
                val <<= spos - dpos;
            } else {
                assert(spos < dpos);
                val >>= dpos - spos;
            }
            *dst = combine(*dst, val, mask);
            return;
        }
        // From two bytes to one.
        assert(spos > dpos);
        auto a = *src << (spos - dpos);
        auto b = src[1] >> (dpos + 8 - spos);
        auto val = unsigned char(a | b);
        *dst = combine(*dst, val, mask);
        return;
    }

    // Multibyte starting at unaligned bits.
    unsigned char hval;
    unsigned char tval = 0;
    auto sval = *src;
    if (spos < dpos) {
        // From part of one byte to first dst byte.
        //   spos 2      dpos 4
        //   00111122 -> 00001111
        hval = sval >> (dpos - spos);
        spos = 8 - (dpos - spos);
    } else {
        assert(spos > dpos);
        // From one byte and part of another to first dst byte.
        //   spos 5                 dpos 3
        //   00000111 + 22333333 -> 00011122
        spos = spos - dpos;
        auto a = sval << spos;
        sval = *++src;
        auto b = sval >> (8 - spos);
        hval = unsigned char(a | b);
    }
    cnt -= 8 - dpos;
    auto bytes = cnt / 8;
    auto tcnt = cnt % 8;
    if (tcnt) {
        // From src to partial last byte of dst.
        tval = src[bytes] << spos;
        if (cnt % 8 > 8 - spos ) {
            // From two bytes instead of one to last dst byte.
            tval |= src[bytes + 1] >> (8 - spos);
        }
    }
    // Copy to middle bytes of dst.
    if (dst < src && src <= dst + bytes) {
        sval = src[bytes];
        for (auto i = bytes; i > 0; --i) {
            auto b = sval >> (8 - spos);
            sval = src[i - 1];
            auto a = sval << spos;
            auto val = unsigned char(a | b);
            dst[i] = val;
        }
    } else {
        for (auto i = 1; i <= bytes; ++i) {
            auto a = sval << spos;
            sval = src[i];
            auto b = sval >> (8 - spos);
            auto val = unsigned char(a | b);
            dst[i] = val;
        }
    }
    // Copy to head and tail bytes of dst.
    *dst = combine(*dst, hval, UCHAR_MAX >> dpos);
    if (tcnt) {
        dst[bytes + 1] = combine(
            dst[bytes + 1],
            tval,
            UCHAR_MAX << (8 - tcnt)
        );
    }
}

//===========================================================================
[[maybe_unused]]
static void bitcpy64(
    void * vdst,
    size_t vdpos,
    const void * vsrc,
    size_t vspos,
    size_t cnt
) {
    // Bytes must be 8 bit.
    static_assert(std::numeric_limits<unsigned char>::digits == 8);
    auto dst = (unsigned char *) vdst + vdpos / 8;
    auto src = (const unsigned char *) vsrc + vspos / 8;
    auto spos = vspos % 8;
    auto dpos = vdpos % 8;

    if (spos == dpos) {
        // The full bytes of src and dst are bit aligned, memcpy can be used.
        return bitmoveAligned(dst, src, spos, cnt);
    }

    // Starts at unaligned bits.
    if (dpos + cnt <= 64) {
        // Copying to whole or part of single 64-bit chunk of memory.
        auto dwidth = (dpos + cnt + 7) / 8;
        auto swidth = (spos + cnt + 7) / 8;
        auto mask = UINT64_MAX >> (64 - cnt) << (64 - dpos - cnt);
        if (spos + cnt <= 64) {
            // From one 64-bit to another.
            auto val = ntoh64(src, swidth);
            if (spos > dpos) {
                val <<= spos - dpos;
            } else {
                assert(spos < dpos);
                val >>= dpos - spos;
            }
            auto dval = ntoh64(dst, dwidth);
            dval = combine(dval, val, mask);
            hton64(dst, dval, dwidth);
            return;
        }
        // From 9 bytes to 64-bit.
        assert(spos > dpos);
        assert(swidth == 9);
        assert(dwidth == 8);
        auto a = ntoh64(src, 8) << (spos - dpos);
        auto b = src[8] >> (8 - (spos - dpos));
        auto val = a | b;
        auto dval = ntoh64(dst, 8);
        dval = combine(dval, val, mask);
        hton64(dst, dval, 8);
        return;
    }

    // Multi-qword starting at unaligned bits and maybe unaligned bytes too.

    // | 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 | ... | 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 |
    //          111111111122222222222222------------------
    //                        111111111122222222222222------------------

    auto daddr = ((uintptr_t) dst) % 8;
    auto saddr = ((uintptr_t) src) % 8;

    uint64_t sval = 0;
    auto sqpos = spos;
    if (!saddr) {
        sval = ntoh64(*(uint64_t *) src);
        src += 8;
    } else {
        if (vsrc <= src - saddr) {
            sval = ntoh64(*(uint64_t *) (src - saddr));
        } else {
            sval = ntoh64l(src, 8 - saddr);
        }
        src += 8 - saddr;
        sqpos += 8 * saddr;
    }
    uint64_t dval = uint64_t(*dst) << (56 - 8 * daddr);
    auto dqpos = dpos;
    if (daddr) {
        dqpos += 8 * daddr;
    }
    if (sqpos < dqpos) {
        // | 0| 8|16|24|32|40|48|56|  | 0| 8|16|24|32|40|48|56|
        //        111111112222222222  ------------                  src
        //                  11111111  2222222222------------        dst
        auto mask = UINT64_MAX >> dqpos;
        auto val = sval >> (dqpos - sqpos);
        dval = combine(dval, val, mask);
        sqpos = 64 - (dqpos - sqpos);
    } else {
        // | 0| 8|16|24|32|40|48|56|  | 0| 8|16|24|32|40|48|56|
        //                  11111111  2222222222------------        src
        //        111111112222222222  ------------                  dst
        assert(sqpos > dqpos);
        auto mask = UINT64_MAX >> dqpos;
        auto a = sval << (sqpos - dqpos);
        auto bcnt = (cnt - (64 - sqpos) + 7) / 8;
        sval = ntoh64(src, bcnt);
        src += bcnt;
        auto b = sval >> (64 - (sqpos - dpos));
        auto val = a | b;
        dval = combine(dval, val, mask);
        sqpos = dqpos - sqpos;
    }
    hton64l(dst, dval, 8 - daddr);
    dst += 8 - daddr;
    cnt -= 64 - dqpos;
    // Copy to middle qwords of dst.
    for (; cnt >= 64; cnt -= 64) {
        auto a = sval << sqpos;
        sval = ntoh64(src);
        src += 8;
        auto b = sval >> (64 - sqpos);
        dval = a | b;
        hton64(dst, dval);
        dst += 8;
    }
    if (cnt) {
        // Copy to partial last qword of dst.
        auto mask = UINT64_MAX << (64 - cnt);
        auto val = sval;
        if (cnt <= 64 - sqpos) {
            // From part of one qword to front of last dst qword.
            val = sval << sqpos;
        } else {
            // From two qwords to last dst qword.
            auto a = sval << sqpos;
            auto scnt = (cnt - (64 - sqpos) + 7) / 8;
            sval = ntoh64(src, scnt);
            auto b = sval >> (64 - sqpos);
            val = a | b;
        }
        auto dcnt = (cnt + 7) / 8;
        dval = uint64_t(dst[dcnt]) << (64 - 8 * dcnt);
        dval = combine(dval, val, mask);
        hton64(dst, dval, dcnt);
    }
}

//===========================================================================
// static
void IBitView::copy(
    void * vdst,
    size_t dpos,
    const void * vsrc,
    size_t spos,
    size_t cnt
) {
    bitmove8(vdst, dpos, vsrc, spos, cnt);
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
    size_t pos,
    size_t cnt
) const {
    assert(pos <= bits());
    cnt = min(cnt, bits() - pos);
    copy(vdst, dpos, data(), pos, cnt);
    return cnt;
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
constexpr BitView::BitView(const uint64_t * src, size_t srcLen)
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
constexpr BitSpan::BitSpan(uint64_t * src, size_t srcLen)
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
    assert(pos < bits());
    if (cnt > bits() - pos)
        cnt = bits() - pos;
    copy(data(), pos, src, spos, cnt);
    return *this;
}

//===========================================================================
BitSpan & BitSpan::set(
    size_t pos,
    const IBitView &src,
    size_t spos,
    size_t cnt // = npos
) {
    return set(pos, src.data(), spos, cnt);
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

//===========================================================================
void BitSpan::insertGap(size_t bitpos, size_t cnt) {
    assert(bitpos < bits());
    if (cnt < bits() - bitpos) {
        auto ocnt = bits() - bitpos - cnt;
        copy(data(), bitpos + cnt, bitpos, ocnt);
    } else {
        // Gap extends to last bit, so we're just discarding the tail and
        // there's no need to move anything.
    }
}

//===========================================================================
BitSpan & BitSpan::insert(
    size_t pos,
    const void * src,
    size_t spos,
    size_t cnt
) {
    insertGap(pos, cnt);
    set(pos, src, spos, cnt);
    return *this;
}

//===========================================================================
BitSpan & BitSpan::insert(
    size_t pos,
    const IBitView & src,
    size_t spos,
    size_t cnt
) {
    return insert(pos, src.data(), spos, cnt);
}

//===========================================================================
BitSpan & BitSpan::insert(size_t pos, size_t cnt, bool value) {
    insertGap(pos, cnt);
    return set(pos, cnt, value);
}

//===========================================================================
BitSpan & BitSpan::erase(size_t pos, size_t cnt) {
    assert(pos < bits());
    if (bits() - pos > cnt) {
        copy(data(), pos, data(), pos + cnt, bits() - pos - cnt);
    }
    return *this;
}

//===========================================================================
void BitSpan::replaceWithGap(size_t pos, size_t cnt, size_t scnt) {
    assert(pos < bits());
    cnt = min(cnt, bits() - pos);
    scnt = min(scnt, bits() - pos);
    if (cnt < scnt) {
        insertGap(pos + cnt, scnt - cnt);
    } else if (cnt > scnt) {
        erase(pos + scnt, cnt - scnt);
    }
}

//===========================================================================
BitSpan & BitSpan::replace(
    size_t pos,
    size_t cnt,
    const void * src,
    size_t spos,
    size_t scnt
) {
    replaceWithGap(pos, cnt, scnt);
    return set(pos, src, spos, scnt);
}

//===========================================================================
BitSpan & BitSpan::replace(
    size_t pos,
    size_t cnt,
    const IBitView & src,
    size_t spos,
    size_t scnt
) {
    return replace(pos, cnt, src.data(), spos, scnt);
}

//===========================================================================
BitSpan & BitSpan::replace(
    size_t pos,
    size_t cnt,
    size_t scnt,
    bool value
) {
    replaceWithGap(pos, cnt, scnt);
    return set(pos, cnt, value);
}
