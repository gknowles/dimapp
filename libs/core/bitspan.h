// Copyright Glen Knowles 2017 - 2023.
// Distributed under the Boost Software License, Version 1.0.
//
// bitspan.h - dim core
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
*   IBitView - internal base class for BitView and BitSpan
*
*   Unless otherwise indicated methods taking 'pos' and 'count' apply to a
*   number of bits equal to:
*       1. bits() - pos + 1 (i.e. all following), if count equals
*          std::dynamic_extent
*       2. count, if pos + count <= bits()
*       3. otherwise undefined, asserts in debug builds
*
***/

class IBitView {
public:
    static constexpr size_t npos = (size_t) -1;
    static constexpr size_t kWordBits = sizeof uint64_t * 8;

public:
    virtual size_t size() const = 0;    // number of uint64_t's
    virtual const uint64_t * data() const = 0;

    explicit operator bool() const { return !empty(); }
    bool operator==(const IBitView & right) const;

    bool empty() const { return !size(); }    // view across nothing

    bool operator[](size_t pos) const;
    bool all() const;
    bool any() const;
    bool none() const;
    size_t bits() const { return kWordBits * size(); } // size in bits
    size_t count() const;
    size_t count(size_t pos, size_t count) const;

    size_t find(size_t bitpos, bool value) const;
    size_t find(size_t bitpos = 0) const;
    size_t findZero(size_t bitpos = 0) const;
    size_t rfind(size_t bitpos, bool value) const;
    size_t rfind(size_t bitpos = npos) const;
    size_t rfindZero(size_t bitpos = npos) const;

    // Get/set sequence of bits as/from an integer. 'count' must be less or
    // equal to 64 (the number of bits that uint64_t can hold).
    uint64_t getBits(size_t pos, size_t count) const;

    // Returns word, or pointer to word, that contains the bit.
    uint64_t word(size_t bitpos) const;
    const uint64_t * data(size_t bitpos) const;
    std::span<const uint64_t> words() const;

protected:
    static constexpr uint64_t kWordMax = (uint64_t) -1;
};

//===========================================================================
inline size_t IBitView::find(size_t pos, bool value) const {
    return value ? find(pos) : findZero(pos);
}

//===========================================================================
inline size_t IBitView::rfind(size_t pos, bool value) const {
    return value ? rfind(pos) : rfindZero(pos);
}


/****************************************************************************
*
*   BitView
*
***/

class BitView : public IBitView {
public:
    BitView() = default;
    BitView(const BitView & from) = default;
    BitView(const uint64_t * src, size_t srcLen);

    BitView view(
        size_t wordOffset,
        size_t wordCount = std::dynamic_extent
    ) const;
    BitView & remove_prefix(size_t wordCount);
    BitView & remove_suffix(size_t wordCount);

    size_t size() const final { return m_size; }
    const uint64_t * data() const final { return m_data; }

private:
    const uint64_t * m_data = nullptr;
    size_t m_size = 0;
};


/****************************************************************************
*
*   BitSpan
*
*   Unless otherwise indicated methods taking 'pos' and 'count' apply to a
*   number of bits equal to:
*       1. bits() - pos + 1 (i.e. all following), if count equals
*          std::dynamic_extent
*       2. count, if pos + count <= bits()
*       3. otherwise undefined, asserts in debug builds
*
***/

class BitSpan : public IBitView {
public:
    BitSpan() = default;
    BitSpan(const BitSpan & from) = default;
    BitSpan(uint64_t * src, size_t srcLen);

    size_t size() const final { return m_size; }
    const uint64_t * data() const final { return m_data; }
    uint64_t * data() { return m_data; }

    BitSpan subspan(
        size_t wordOffset,
        size_t wordCount = std::dynamic_extent
    ) const;
    BitSpan & remove_prefix(size_t wordCount);
    BitSpan & remove_suffix(size_t wordCount);

    BitSpan & set();
    BitSpan & set(size_t pos);
    BitSpan & set(size_t pos, bool value);
    BitSpan & set(size_t pos, size_t count);
    BitSpan & set(size_t pos, size_t count, bool value);
    BitSpan & reset();
    BitSpan & reset(size_t pos);
    BitSpan & reset(size_t pos, size_t count);
    BitSpan & flip();
    BitSpan & flip(size_t pos);
    BitSpan & flip(size_t pos, size_t count);
    bool testAndSet(size_t pos);
    bool testAndSet(size_t pos, bool value);
    bool testAndReset(size_t pos);
    bool testAndFlip(size_t pos);

    // Get/set sequence of bits as/from an integer. 'count' must be less or
    // equal to 64 (the number of bits that uint64_t can hold).
    BitSpan & setBits(size_t pos, size_t count, uint64_t value);

    // Returns pointer to word that contains the bit.
    uint64_t * data(size_t bitpos);

private:
    uint64_t * m_data = nullptr;
    size_t m_size = 0;
};

//===========================================================================
inline BitSpan & BitSpan::set(size_t pos, bool value) {
    return value ? set(pos) : reset(pos);
}

//===========================================================================
inline BitSpan & BitSpan::set(size_t pos, size_t count, bool value) {
    return value ? set(pos, count) : reset(pos, count);
}

//===========================================================================
inline bool BitSpan::testAndSet(size_t pos, bool value) {
    return value ? testAndSet(pos) : testAndReset(pos);
}

} // namespace
