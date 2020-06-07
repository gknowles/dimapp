// Copyright Glen Knowles 2017 - 2019.
// Distributed under the Boost Software License, Version 1.0.
//
// uset.h - dim core
#pragma once

#include "cppconf/cppconf.h"

#include <compare>
#include <cstdint>
#include <initializer_list>
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
    struct RangeRange;
    class Iterator;

    using value_type = unsigned;
    using reference = value_type&;
    using const_reference = const value_type&;
    using iterator = Iterator;
    using difference_type = ptrdiff_t;
    using size_type = size_t;

public:
    struct Node {
        unsigned type : 4;
        unsigned depth : 4;
        unsigned base : 24;
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
    ~UnsignedSet();
    explicit operator bool() const { return !empty(); }

    UnsignedSet & operator=(UnsignedSet && from) noexcept;
    UnsignedSet & operator=(const UnsignedSet & from);

    // iterators
    iterator begin() const;
    iterator end() const;

    RangeRange ranges() const;

    // capacity
    bool empty() const;
    size_t size() const;

    // modify
    void clear();
    void fill();
    void assign(unsigned value);
    template <typename InputIt>
        requires (std::is_convertible_v<*std::declval<InputIt>(), unsigned>)
        void assign(InputIt first, InputIt last);
    void assign(std::initializer_list<unsigned> il);
    void assign(UnsignedSet && from);
    void assign(const UnsignedSet & from);
    void assign(std::string_view src); // space separated ranges
    bool insert(unsigned value); // returns true if inserted
    template <typename InputIt>
        requires (std::is_convertible_v<*std::declval<InputIt>(), unsigned>)
        void insert(InputIt first, InputIt last);
    void insert(std::initializer_list<unsigned> il);
    void insert(UnsignedSet && other);
    void insert(const UnsignedSet & other);
    void insert(std::string_view src); // space separated ranges
    void insert(unsigned low, unsigned high);
    bool erase(unsigned value); // returns true if erased
    void erase(iterator where);
    void erase(unsigned low, unsigned high);
    void erase(const UnsignedSet & other);
    unsigned pop_back();
    unsigned pop_front();
    void intersect(UnsignedSet && other);
    void intersect(const UnsignedSet & other);
    void swap(UnsignedSet & other);

    // compare
    std::strong_ordering compare(const UnsignedSet & right) const;
    bool includes(const UnsignedSet & other) const;
    bool intersects(const UnsignedSet & other) const;
    std::strong_ordering operator<=>(const UnsignedSet & right) const;
    bool operator==(const UnsignedSet & right) const;

    // search
    size_t count(unsigned val) const;
    iterator find(unsigned val) const;
    std::pair<iterator, iterator> equalRange(unsigned val) const;
    iterator lowerBound(unsigned val) const;
    iterator upperBound(unsigned val) const;
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
template<typename InputIt>
requires (std::is_convertible_v<*std::declval<InputIt>(), unsigned>)
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
template<typename InputIt>
requires (std::is_convertible_v<*std::declval<InputIt>(), unsigned>)
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
    using iterator_category = std::input_iterator_tag;
    using value_type = UnsignedSet::value_type;
    using difference_type = UnsignedSet::difference_type;
    using pointer = const value_type*;
    using reference = value_type;
public:
    Iterator() {}
    Iterator(const Iterator & from) = default;
    Iterator(const Node * node);
    Iterator(const Node * node, value_type value, unsigned minDepth);
    Iterator & operator++();
    explicit operator bool() const { return (bool) m_node; }
    bool operator== (const Iterator & right) const;
    value_type operator*() const { return m_value; }
    const value_type * operator->() const { return &m_value; }

    Iterator lastContiguous() const;
private:
    friend class UnsignedSet;
    const Node * m_node = nullptr;
    value_type m_value = 0;
    unsigned m_minDepth = 0;
};


/****************************************************************************
*
*   UnsignedSet::RangeIterator
*
***/

class UnsignedSet::RangeIterator {
public:
    using iterator_category = std::input_iterator_tag;
    using value_type =
        std::pair<UnsignedSet::value_type, UnsignedSet::value_type>;
    using difference_type = ptrdiff_t;
    using pointer = const value_type*;
    using reference = const value_type&;
public:
    RangeIterator() {}
    RangeIterator(const RangeIterator & from) = default;
    RangeIterator(Iterator where);
    RangeIterator & operator++();
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

struct UnsignedSet::RangeRange {
    Iterator m_first;
    RangeIterator begin() { return m_first; }
    RangeIterator end() { return {}; }
};


} // namespace
