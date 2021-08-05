// Copyright Glen Knowles 2017 - 2021.
// Distributed under the Boost Software License, Version 1.0.
//
// uset.h - dim core
#pragma once

#include "cppconf/cppconf.h"

#include "core/types.h"

#include <compare>
#include <concepts>
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
*   IntegralSet
*
***/

template <std::integral T, typename A = std::allocator<T>>
class IntegralSet {
public:
    template<typename Iter> class RevIterBase;
    class Iter;
    class RangeRange;
    class RangeIter;
    class Impl;

    using allocator_type = A;

    using value_type = T;
    using reference = value_type &;
    using const_reference = const value_type &;
    using iterator = Iter;
    using reverse_iterator = typename RevIterBase<Iter>;
    using difference_type = ptrdiff_t;
    using size_type = size_t;
    using range_iterator = RangeIter;
    using reverse_range_iterator = typename RevIterBase<RangeIter>;

public:
    constexpr static size_t kBitWidth = CHAR_BIT * sizeof T;
    constexpr static size_t kTypeBits = 4;
    constexpr static size_t kDepthBits = 4;
    constexpr static size_t kBaseBits = kBitWidth - kTypeBits - kDepthBits;

    struct Node : NoCopy {
        int type : kTypeBits;
        unsigned depth : kDepthBits;
        unsigned base : kBaseBits;
        uint16_t numBytes;  // space allocated
        uint16_t numValues; // usage depends on node type
        union {
            value_type * values;
            Node * nodes;
            value_type localValues[sizeof (value_type *) / sizeof value_type];
        };

        Node();
    };

public:
    IntegralSet();
    explicit IntegralSet(const A & alloc);
    IntegralSet(IntegralSet && from) noexcept;
    IntegralSet(const IntegralSet & from);
    IntegralSet(std::initializer_list<value_type> from, const A & alloc = {});
    IntegralSet(std::string_view from, const A & alloc = {});
    IntegralSet(value_type start, size_t len, const A & alloc = {});
    ~IntegralSet();
    explicit operator bool() const { return !empty(); }

    IntegralSet & operator=(IntegralSet && from) noexcept;
    IntegralSet & operator=(const IntegralSet & from);

    allocator_type get_allocator() const;

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
    void assign(IntegralSet && from);
    void assign(const IntegralSet & from);
    void assign(value_type val);
    template <std::input_iterator InputIt>
        requires (std::is_convertible_v<decltype(*std::declval<InputIt>()),
            IntegralSet<T, A>::value_type>)
        void assign(InputIt first, InputIt last);
    void assign(std::initializer_list<value_type> il);
    void assign(value_type start, size_t len);
    void assign(std::string_view src); // space separated ranges
    void insert(IntegralSet && other);
    void insert(const IntegralSet & other);
    bool insert(value_type val); // returns true if inserted
    template <std::input_iterator InputIt>
        requires (std::is_convertible_v<decltype(*std::declval<InputIt>()),
            IntegralSet<T, A>::value_type>)
        void insert(InputIt first, InputIt last);
    void insert(std::initializer_list<value_type> il);
    void insert(value_type start, size_t len);
    void insert(std::string_view src); // space separated ranges
    bool erase(value_type val); // returns true if erased
    void erase(iterator where);
    void erase(const IntegralSet & other);
    void erase(value_type start, size_t len);
    value_type pop_back();
    value_type pop_front();
    void intersect(IntegralSet && other);
    void intersect(const IntegralSet & other);
    void swap(IntegralSet & other);

    // compare
    std::strong_ordering compare(const IntegralSet & other) const;
    std::strong_ordering operator<=>(const IntegralSet & other) const;
    bool operator==(const IntegralSet & right) const;

    // search
    value_type front() const;
    value_type back() const;
    size_t count() const; // alias for size()
    size_t count(value_type val) const;
    size_t count(value_type start, size_t len) const;
    iterator find(value_type val) const;
    bool contains(value_type val) const;
    bool contains(const IntegralSet & other) const;
    bool intersects(const IntegralSet & other) const;
    iterator findLessEqual(value_type val) const;
    iterator lowerBound(value_type val) const;
    iterator upperBound(value_type val) const;
    std::pair<iterator, iterator> equalRange(value_type val) const;

    // firstContiguous and lastContiguous search backwards and forwards
    // respectively, for as long as consecutive values are present. For
    // example, given the set { 1, 2, 4, 5, 6, 7, 9 } and starting at 2, first
    // and last contiguous are 1 and 2, but starting at 5 they are 4 and 7.
    iterator firstContiguous(iterator where) const;
    iterator lastContiguous(iterator where) const;

private:
    friend std::ostream & operator<<(
        std::ostream & os,
        const IntegralSet & right
    );

private:
    void iInsert(const value_type * first, const value_type * last);

    Node m_node;
    allocator_type m_alloc;
};

//===========================================================================
template <std::integral T, typename A>
template <std::input_iterator InputIt>
    requires (std::is_convertible_v<decltype(*std::declval<InputIt>()),
        typename IntegralSet<T, A>::value_type>)
inline void IntegralSet<T, A>::assign(InputIt first, InputIt last) {
    clear();
    insert(first, last);
}

//===========================================================================
template <std::integral T, typename A>
template<std::input_iterator InputIt>
    requires (std::is_convertible_v<decltype(*std::declval<InputIt>()),
        typename IntegralSet<T, A>::value_type>)
inline void IntegralSet<T, A>::insert(InputIt first, InputIt last) {
    if constexpr (std::is_convertible_v<InputIt, const value_type *>) {
        iInsert(first, last);
    } else {
        for (; first != last; ++first)
            insert(*first);
    }
}


/****************************************************************************
*
*   IntegralSet::RevIterBase
*
***/

template <std::integral T, typename A>
template<typename IterBase>
class IntegralSet<T, A>::RevIterBase {
public:
    using iterator_type = IterBase;
    using traits = std::iterator_traits<iterator_type>;
    using iterator_category = traits::iterator_category;
    using value_type = traits::value_type;
    using difference_type = traits::difference_type;
    using pointer = traits::pointer;
    using reference = traits::reference;
    using thing = std::reverse_iterator<iterator_type>;

public:
    RevIterBase(iterator_type iter)
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
    const value_type & operator*() const { return *m_iter; }
    const value_type * operator->() const { return &*m_iter; }

    constexpr iterator_type base() const {
        auto tmp = m_iter;
        ++tmp;
        return tmp;
    }

private:
    iterator_type m_iter;
};


/****************************************************************************
*
*   IntegralSet::Iter
*
***/

template <std::integral T, typename A>
class IntegralSet<T, A>::Iter {
public:
    using iterator_category = std::bidirectional_iterator_tag;
    using value_type = IntegralSet::value_type;
    using difference_type = IntegralSet::difference_type;
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
    const value_type & operator*() const { return m_value; }
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
*   IntegralSet::RangeIter
*
***/

template <std::integral T, typename A>
class IntegralSet<T, A>::RangeIter {
public:
    using iterator_category = std::bidirectional_iterator_tag;
    using value_type =
        std::pair<IntegralSet::value_type, IntegralSet::value_type>;
    using difference_type = ptrdiff_t;
    using pointer = const value_type*;
    using reference = const value_type&;

public:
    RangeIter(const RangeIter & from) = default;
    RangeIter(IntegralSet::iterator where);
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
*   IntegralSet::RangeRange
*
***/

template <std::integral T, typename A>
class IntegralSet<T, A>::RangeRange {
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


/****************************************************************************
*
*   Aliases
*
***/

using UnsignedSet = IntegralSet<unsigned>;

extern template class IntegralSet<unsigned>;

} // namespace
