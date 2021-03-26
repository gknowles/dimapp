// Copyright Glen Knowles 2017 - 2020.
// Distributed under the Boost Software License, Version 1.0.
//
// uset.h - dim core
#pragma once

#include "cppconf/cppconf.h"

#include <compare>
#include <cstdint>
#include <initializer_list>
#include <iterator>
#include <ostream>
#include <string_view>
#include <type_traits> // std::is_convertible_v
#include <utility> // std::declval, std::pair

namespace Dim {


/****************************************************************************
*
*   UnsignedSet
*
***/

class UnsignedSet {
public:
    class RangeIterator;
    class RangeRange;
    class Iterator;

    using value_type = unsigned;
    using reference = value_type&;
    using const_reference = const value_type&;
    using iterator = Iterator;
    using reverse_iterator = std::reverse_iterator<iterator>;
    using difference_type = ptrdiff_t;
    using size_type = size_t;

public:
    constexpr static unsigned kTypeBits = 4;
    constexpr static unsigned kDepthBits = 4;
    constexpr static unsigned kBaseBits = 32 - kTypeBits - kDepthBits;

    constexpr static unsigned kDepthEnd = (1 << kDepthBits) - 1;

    struct Node {
        enum Type : int;
        Type type : kTypeBits;
        unsigned depth : kDepthBits;
        unsigned base : kBaseBits;
        uint16_t numBytes;     // space allocated
        uint16_t numValues;
        union {
            unsigned * values;
            Node * nodes;
        };

        Node();
    };

public:
    UnsignedSet();
    UnsignedSet(UnsignedSet && from) noexcept;
    UnsignedSet(const UnsignedSet & from);
    UnsignedSet(std::initializer_list<unsigned> from);
    UnsignedSet(std::string_view from);
    UnsignedSet(unsigned low, unsigned high);
    ~UnsignedSet();
    explicit operator bool() const { return !empty(); }

    UnsignedSet & operator=(UnsignedSet && from) noexcept;
    UnsignedSet & operator=(const UnsignedSet & from);

    // iterators
    iterator begin() const;
    iterator end() const;
    reverse_iterator rbegin() const;
    reverse_iterator rend() const;

    RangeRange ranges() const;

    // capacity
    bool empty() const;
    size_t size() const;
    size_t max_size() const;

    // modify
    void clear();
    void fill();
    void assign(unsigned value);
    template <std::input_iterator InputIt>
        requires (std::is_convertible_v<decltype(*std::declval<InputIt>()),
            unsigned>)
        void assign(InputIt first, InputIt last);
    void assign(std::initializer_list<unsigned> il);
    void assign(UnsignedSet && from);
    void assign(const UnsignedSet & from);
    void assign(std::string_view src); // space separated ranges
    void assign(unsigned low, unsigned high);
    bool insert(unsigned value); // returns true if inserted
    template <std::input_iterator InputIt>
        requires (std::is_convertible_v<decltype(*std::declval<InputIt>()),
            unsigned>)
        void insert(InputIt first, InputIt last);
    void insert(std::initializer_list<unsigned> il);
    void insert(UnsignedSet && other);
    void insert(const UnsignedSet & other);
    void insert(std::string_view src); // space separated ranges
    void insert(unsigned low, unsigned high);
    bool erase(unsigned value); // returns true if erased
    void erase(iterator where);
    void erase(const UnsignedSet & other);
    void erase(unsigned low, unsigned high);
    unsigned pop_back();
    unsigned pop_front();
    void intersect(UnsignedSet && other);
    void intersect(const UnsignedSet & other);
    void swap(UnsignedSet & other);

    // compare
    std::strong_ordering compare(const UnsignedSet & right) const;
    std::strong_ordering operator<=>(const UnsignedSet & right) const;
    bool operator==(const UnsignedSet & right) const;

    // search
    unsigned front() const;
    unsigned back() const;
    size_t count(unsigned val) const;
    iterator find(unsigned val) const;
    bool contains(unsigned val) const;
    bool contains(const UnsignedSet & other) const;
    bool intersects(const UnsignedSet & other) const;
    std::pair<iterator, iterator> equalRange(unsigned val) const;
    iterator lowerBound(unsigned val) const;
    iterator upperBound(unsigned val) const;

    // firstContiguous and lastContiguous search backwards and forwards
    // respectively, for as long as consecutive values are present. For
    // example, given the set { 1, 2, 4, 5, 6, 7, 9 } and starting at 2, first
    // and last contiguous are 1 and 2, where at 5 they are 4 and 7.
    iterator firstContiguous(iterator where) const;
    iterator lastContiguous(iterator where) const;

private:
    friend std::ostream & operator<<(
        std::ostream & os,
        const UnsignedSet & right
    );

private:
    void iInsert(const unsigned * first, const unsigned * last);

    Node m_node;
};

//===========================================================================
inline UnsignedSet::UnsignedSet(std::initializer_list<unsigned> il) {
    insert(il);
}

//===========================================================================
template<std::input_iterator InputIt>
requires (std::is_convertible_v<decltype(*std::declval<InputIt>()), unsigned>)
inline void UnsignedSet::assign(InputIt first, InputIt last) {
    clear();
    insert(first, last);
}

//===========================================================================
inline void UnsignedSet::assign(std::initializer_list<unsigned> il) {
    clear();
    insert(il);
}

//===========================================================================
template<std::input_iterator InputIt>
requires (std::is_convertible_v<decltype(*std::declval<InputIt>()), unsigned>)
inline void UnsignedSet::insert(InputIt first, InputIt last) {
    if constexpr (std::is_convertible_v<InputIt, const unsigned *>) {
        iInsert(first, last);
    } else {
        for (; first != last; ++first)
            insert(*first);
    }
}

//===========================================================================
inline void UnsignedSet::insert(std::initializer_list<unsigned> il) {
    iInsert(il.begin(), il.end());
}


/****************************************************************************
*
*   UnsignedSet::Iterator
*
***/

class UnsignedSet::Iterator {
public:
    using iterator_category = std::bidirectional_iterator_tag;
    using value_type = UnsignedSet::value_type;
    using difference_type = UnsignedSet::difference_type;
    using pointer = const value_type*;
    using reference = value_type;

public:
    Iterator(const Iterator & from) = default;
    Iterator(const Node * node);
    Iterator(const Node * node, value_type value, unsigned minDepth);
    Iterator & operator++();
    Iterator & operator--();
    explicit operator bool() const { return m_minDepth != kDepthEnd; }
    bool operator==(const Iterator & right) const;
    value_type operator*() const { return m_value; }
    const value_type * operator->() const { return &m_value; }

    Iterator firstContiguous() const;
    Iterator lastContiguous() const;

private:
    friend UnsignedSet;
    Iterator end() const;

    const Node * m_node = nullptr;
    value_type m_value = 0;
    unsigned m_minDepth = kDepthEnd;
};


/****************************************************************************
*
*   UnsignedSet::RangeIterator
*
***/

class UnsignedSet::RangeIterator {
public:
    using iterator_category = std::bidirectional_iterator_tag;
    using value_type =
        std::pair<UnsignedSet::value_type, UnsignedSet::value_type>;
    using difference_type = ptrdiff_t;
    using pointer = const value_type*;
    using reference = const value_type&;

public:
    RangeIterator(const RangeIterator & from) = default;
    RangeIterator(Iterator where);
    RangeIterator & operator++();
    RangeIterator & operator--();
    explicit operator bool() const { return m_value != kEndValue; }
    bool operator== (const RangeIterator & right) const;
    const value_type & operator*() const { return m_value; }
    const value_type * operator->() const { return &m_value; }

private:
    static constexpr value_type kEndValue{1, 0};
    Iterator m_iter;
    value_type m_value{kEndValue};
};


/****************************************************************************
*
*   UnsignedSet::RangeRange
*
***/

class UnsignedSet::RangeRange {
public:
    using iterator = RangeIterator;
    using reverse_iterator = std::reverse_iterator<RangeIterator>;

public:
    explicit RangeRange(Iterator iter) : m_first{iter} {}
    RangeIterator begin() const { return m_first; }
    RangeIterator end() const { return m_first.end(); }
    auto rbegin() const { return reverse_iterator(end()); }
    auto rend() const { return reverse_iterator(begin()); }

private:
    Iterator m_first;
};


} // namespace
