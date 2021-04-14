// Copyright Glen Knowles 2017 - 2021.
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
    template<typename T> class RevIterBase;
    class Iter;
    class RangeRange;
    class RangeIter;

    using value_type = unsigned;
    using reference = value_type&;
    using const_reference = const value_type&;
    using iterator = Iter;
    using reverse_iterator = typename RevIterBase<Iter>;
    using difference_type = ptrdiff_t;
    using size_type = size_t;
    using range_iterator = RangeIter;
    using reverse_range_iterator = typename RevIterBase<RangeIter>;

public:
    constexpr static unsigned kTypeBits = 4;
    constexpr static unsigned kDepthBits = 4;
    constexpr static unsigned kBaseBits = 32 - kTypeBits - kDepthBits;

    struct Node {
        enum Type : int;
        Type type : kTypeBits;
        unsigned depth : kDepthBits;
        unsigned base : kBaseBits;
        uint16_t numBytes;  // space allocated
        uint16_t numValues; // usage depends on node type
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
    UnsignedSet(unsigned start, size_t count);
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
    void assign(unsigned val);
    template <std::input_iterator InputIt>
        requires (std::is_convertible_v<decltype(*std::declval<InputIt>()),
            unsigned>)
        void assign(InputIt first, InputIt last);
    void assign(std::initializer_list<unsigned> il);
    void assign(UnsignedSet && from);
    void assign(const UnsignedSet & from);
    void assign(std::string_view src); // space separated ranges
    void assign(unsigned start, size_t count);
    bool insert(unsigned val); // returns true if inserted
    template <std::input_iterator InputIt>
        requires (std::is_convertible_v<decltype(*std::declval<InputIt>()),
            unsigned>)
        void insert(InputIt first, InputIt last);
    void insert(std::initializer_list<unsigned> il);
    void insert(UnsignedSet && other);
    void insert(const UnsignedSet & other);
    void insert(std::string_view src); // space separated ranges
    void insert(unsigned start, size_t count);
    bool erase(unsigned val); // returns true if erased
    void erase(iterator where);
    void erase(const UnsignedSet & other);
    void erase(unsigned start, size_t count);
    unsigned pop_back();
    unsigned pop_front();
    void intersect(UnsignedSet && other);
    void intersect(const UnsignedSet & other);
    void swap(UnsignedSet & other);

    // compare
    std::strong_ordering compare(const UnsignedSet & other) const;
    std::strong_ordering operator<=>(const UnsignedSet & other) const;
    bool operator==(const UnsignedSet & right) const;

    // search
    unsigned front() const;
    unsigned back() const;
    size_t count(unsigned val) const;
    size_t count(unsigned start, size_t count) const;
    iterator find(unsigned val) const;
    bool contains(unsigned val) const;
    bool contains(const UnsignedSet & other) const;
    bool intersects(const UnsignedSet & other) const;
    iterator findLessEqual(unsigned val) const;
    iterator lowerBound(unsigned val) const;
    iterator upperBound(unsigned val) const;
    std::pair<iterator, iterator> equalRange(unsigned val) const;

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
*   UnsignedSet::RevIterBase
*
***/

template<typename T>
class UnsignedSet::RevIterBase {
public:
    using iterator_type = T;
    using iterator_category = std::iterator_traits<T>::iterator_category;
    using value_type = std::iterator_traits<T>::value_type;
    using difference_type = std::iterator_traits<T>::difference_type;
    using pointer = std::iterator_traits<T>::pointer;
    using reference = std::iterator_traits<T>::reference;
    using thing = std::reverse_iterator<T>;

public:
    RevIterBase(T iter)
        requires std::derived_from<iterator_category,
            std::bidirectional_iterator_tag>
        : m_iter(iter)
    {
        --m_iter;
    }
    RevIterBase & operator++() { --m_iter; return *this; }
    RevIterBase & operator--() { ++m_iter; return *this; }
    explicit operator bool() const { return (bool) m_iter; }
    bool operator==(const RevIterBase & other) const = default;
    value_type operator*() const { return *m_iter; }
    const value_type * operator->() const { return &*m_iter; }

    constexpr T base() const { auto tmp = m_iter; ++tmp; return tmp; }

private:
    T m_iter;
};


/****************************************************************************
*
*   UnsignedSet::Iter
*
***/

class UnsignedSet::Iter {
public:
    using iterator_category = std::bidirectional_iterator_tag;
    using value_type = UnsignedSet::value_type;
    using difference_type = UnsignedSet::difference_type;
    using pointer = const value_type*;
    using reference = value_type;

public:
    static Iter makeEnd(const Node * node);
    static Iter makeFirst(const Node * node);
    static Iter makeFirst(const Node * node, value_type minValue);
    static Iter makeLast(const Node * node);
    static Iter makeLast(const Node * node, value_type maxValue);

public:
    Iter(const Iter & from) = default;
    Iter & operator++();
    Iter & operator--();
    explicit operator bool() const { return !m_endmark; }
    bool operator==(const Iter & right) const;
    value_type operator*() const { return m_value; }
    const value_type * operator->() const { return &m_value; }

    Iter firstContiguous() const;
    Iter lastContiguous() const;
    Iter end() const;

private:
    Iter(const Node * node);
    Iter(const Node * node, value_type val);

    const Node * m_node = nullptr;
    value_type m_value = 0;
    bool m_endmark = true;
};


/****************************************************************************
*
*   UnsignedSet::RangeIter
*
***/

class UnsignedSet::RangeIter {
public:
    using iterator_category = std::bidirectional_iterator_tag;
    using value_type =
        std::pair<UnsignedSet::value_type, UnsignedSet::value_type>;
    using difference_type = ptrdiff_t;
    using pointer = const value_type*;
    using reference = const value_type&;

public:
    RangeIter(const RangeIter & from) = default;
    RangeIter(iterator where);
    RangeIter & operator++();
    RangeIter & operator--();
    explicit operator bool() const { return (bool) m_low; }
    bool operator== (const RangeIter & right) const;
    const value_type & operator*() const { return m_value; }
    const value_type * operator->() const { return &m_value; }

private:
    iterator m_low;
    iterator m_high;
    value_type m_value;
};


/****************************************************************************
*
*   UnsignedSet::RangeRange
*
***/

class UnsignedSet::RangeRange {
public:
    using iterator = RangeIter;
    using reverse_iterator = RevIterBase<RangeIter>;

public:
    explicit RangeRange(Iter iter) : m_first{iter} {}
    iterator begin() const { return m_first; }
    iterator end() const { return m_first.end(); }
    auto rbegin() const { return reverse_iterator(end()); }
    auto rend() const { return reverse_iterator(begin()); }

private:
    Iter m_first;
};


} // namespace
