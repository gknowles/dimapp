// Copyright Glen Knowles 2017 - 2019.
// Distributed under the Boost Software License, Version 1.0.
//
// bitview.h - dim core
//
// View of bits over an array of uint64_t's, bits of view are in network
// (i.e. big-endian, MSB first) order.

#pragma once

#include "cppconf/cppconf.h"

#include <cstdint>

namespace Dim {


/****************************************************************************
*
*   BitView
*
***/

class BitView {
public:
    static constexpr size_t npos = (size_t) -1;
    static constexpr size_t kWordBits = sizeof uint64_t * 8;

public:
    BitView() = default;
    BitView(BitView const & from) = default;
    BitView(uint64_t * src, size_t srcLen);

    explicit operator bool() const { return !empty(); }
    bool operator==(BitView const & right) const;

    bool empty() const { return !m_size; }    // view across nothing
    size_t size() const { return m_size; }    // number of uint64_t's
    uint64_t * data() { return m_data; }
    uint64_t const * data() const { return m_data; }
    BitView & remove_prefix(size_t numUint64);
    BitView & remove_suffix(size_t numUint64);

    bool operator[](size_t pos) const;
    uint64_t get(size_t pos, size_t count) const;
    bool all() const;
    bool any() const;
    bool none() const;
    size_t bits() const { return kWordBits * m_size; } // size in bits
    size_t count() const;
    BitView & set();
    BitView & set(size_t pos);
    BitView & set(size_t pos, bool value);
    BitView & set(size_t pos, size_t count, uint64_t value);
    BitView & reset();
    BitView & reset(size_t pos);
    BitView & flip();
    BitView & flip(size_t pos);
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

    // Returns word that contains the bit
    uint64_t word(size_t bitpos) const;

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
