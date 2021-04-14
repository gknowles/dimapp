// Copyright Glen Knowles 2017 - 2021.
// Distributed under the Boost Software License, Version 1.0.
//
// bitview.h - dim core
//
// View of bits over an array of uint64_t's, bits of view are in network
// byte order (i.e. big-endian, MSB first).

#pragma once

#include "cppconf/cppconf.h"

#include <cstdint>
#include <span>

namespace Dim {


/****************************************************************************
*
*   BitView
*
*   Unless otherwise indicated methods taking 'pos' and 'count' apply to a
*   number of bits equal to:
*       1. bits() - pos + 1, if count equals std::dynamic_extent
*       2. count, if pos + count <= bits()
*       3. otherwise undefined, asserts in debug builds
*
***/

class BitView {
public:
    static constexpr size_t npos = (size_t) -1;
    static constexpr size_t kWordBits = sizeof uint64_t * 8;

public:
    BitView() = default;
    BitView(const BitView & from) = default;
    BitView(uint64_t * src, size_t srcLen);

    explicit operator bool() const { return !empty(); }
    bool operator==(const BitView & right) const;

    bool empty() const { return !m_size; }    // view across nothing
    size_t size() const { return m_size; }    // number of uint64_t's
    uint64_t * data() { return m_data; }
    const uint64_t * data() const { return m_data; }
    BitView view(
        size_t wordOffset,
        size_t wordCount = std::dynamic_extent
    ) const;
    BitView & remove_prefix(size_t wordCount);
    BitView & remove_suffix(size_t wordCount);

    bool operator[](size_t pos) const;
    bool all() const;
    bool any() const;
    bool none() const;
    size_t bits() const { return kWordBits * m_size; } // size in bits
    size_t count() const;
    BitView & set();
    BitView & set(size_t pos);
    BitView & set(size_t pos, bool value);
    BitView & set(size_t pos, size_t count);
    BitView & set(size_t pos, size_t count, bool value);
    BitView & reset();
    BitView & reset(size_t pos);
    BitView & reset(size_t pos, size_t count);
    BitView & flip();
    BitView & flip(size_t pos);
    BitView & flip(size_t pos, size_t count);
    bool testAndSet(size_t pos);
    bool testAndSet(size_t pos, bool value);
    bool testAndReset(size_t pos);
    bool testAndFlip(size_t pos);

    size_t find(size_t bitpos, bool value) const;
    size_t find(size_t bitpos = 0) const;
    size_t findZero(size_t bitpos = 0) const;
    size_t rfind(size_t bitpos, bool value) const;
    size_t rfind(size_t bitpos = npos) const;
    size_t rfindZero(size_t bitpos = npos) const;

    // Get/set sequence of bits as/from an integer. 'count' must be less or
    // equal to 64 (the number of bits that uint64_t).
    uint64_t getBits(size_t pos, size_t count) const;
    BitView & setBits(size_t pos, size_t count, uint64_t value);

    // Returns word, or pointer to word, that contains the bit.
    uint64_t word(size_t bitpos) const;
    uint64_t * data(size_t bitpos);
    const uint64_t * data(size_t bitpos) const;

private:
    uint64_t * m_data = nullptr;
    size_t m_size = 0;

    static constexpr uint64_t kWordMax = (uint64_t) -1;
};

//===========================================================================
inline BitView & BitView::set(size_t pos, bool value) {
    return value ? set(pos) : reset(pos);
}

//===========================================================================
inline BitView & BitView::set(size_t pos, size_t count, bool value) {
    return value ? set(pos, count) : reset(pos, count);
}

//===========================================================================
inline bool BitView::testAndSet(size_t pos, bool value) {
    return value ? testAndSet(pos) : testAndReset(pos);
}

//===========================================================================
inline size_t BitView::find(size_t pos, bool value) const {
    return value ? find(pos) : findZero(pos);
}

//===========================================================================
inline size_t BitView::rfind(size_t pos, bool value) const {
    return value ? rfind(pos) : rfindZero(pos);
}

} // namespace
