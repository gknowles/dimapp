// Copyright Glen Knowles 2017 - 2024.
// Distributed under the Boost Software License, Version 1.0.
//
// intset.h - dim core
#pragma once

#include "cppconf/cppconf.h"

#include "core/algo.h"
#include "core/types.h"

#include <compare>
#include <concepts>
#include <cstdint>
#include <initializer_list>
#include <iterator>
#include <memory_resource>
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
    class Iter;
    class RangeRange;
    class RangeIter;
    class Impl;

    using allocator_type = A;
    using storage_type = std::make_unsigned_t<T>;

    using value_type = T;
    using reference = value_type &;
    using const_reference = const value_type &;
    using iterator = Iter;
    using reverse_iterator = typename reverse_circular_iterator<Iter>;
    using difference_type = ptrdiff_t;
    using size_type = size_t;
    using range_iterator = RangeIter;
    using reverse_range_iterator = typename reverse_circular_iterator<RangeIter>;

    static const value_type npos = std::numeric_limits<value_type>::max();

public:
    static const size_t kBitWidth = CHAR_BIT * sizeof (T);
    static const size_t kTypeBits = 4;
    static const size_t kDepthBits = 4;
    static const size_t kBaseBits = kBitWidth - kTypeBits - kDepthBits;

    static_assert(std::numeric_limits<T>::radix == 2);
    static_assert(std::numeric_limits<storage_type>::digits == kBitWidth);

    enum NodeType : int {
        kEmpty,         // contains no values
        kFull,          // contains all values in node's domain
        kSmVector,      // small vector of values embedded in node struct
        kVector,        // vector of values
        kBitmap,        // bitmap covering all of node's possible values
        kMeta,          // vector of nodes
        kNodeTypes,
        kMetaParent,    // link to parent meta node
    };
    static_assert(kMetaParent < (1 << kTypeBits));
    struct Node : NoCopy {
        enum NodeType type : kTypeBits;
        unsigned depth : kDepthBits;
        unsigned base : kBaseBits;
        uint16_t numBytes;  // space allocated
        uint16_t numValues; // usage depends on node type
        union {
            storage_type * values;
            Node * nodes;
            storage_type localValues[
                sizeof (storage_type *) / sizeof (storage_type)];
        };

        Node();
    };

public:
    IntegralSet() = default;
    explicit IntegralSet(const A & alloc) noexcept;
    IntegralSet(IntegralSet && from) noexcept;
    IntegralSet(IntegralSet && from, const A & alloc);
    IntegralSet(const IntegralSet & from);
    IntegralSet(const IntegralSet & from, const A & alloc);
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
        requires (std::is_convertible_v<decltype(*std::declval<InputIt>()), T>)
        void assign(InputIt first, InputIt last);
    void assign(std::initializer_list<value_type> il);
    void assign(value_type start, size_t len);
    void assign(std::string_view src); // space separated ranges
    void insert(IntegralSet && other);
    void insert(const IntegralSet & other);
    bool insert(value_type val); // returns true if inserted
    template <std::input_iterator InputIt>
        requires (std::is_convertible_v<decltype(*std::declval<InputIt>()), T>)
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
    bool contains(value_type val) const;
    bool contains(const IntegralSet & other) const;
    bool intersects(const IntegralSet & other) const;
    iterator find(value_type val) const;
    iterator findLess(value_type val) const;
    iterator findLessEqual(value_type val) const;
    iterator lowerBound(value_type val) const;  // greaterEqual
    iterator upperBound(value_type val) const;  // greater
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
    ) {
        if (auto v = right.ranges().begin()) {
            for (;;) {
                os << v->first;
                if (auto len = v->second - v->first) {
                    if constexpr (std::is_signed_v<T>) {
                        if (len > 1) {
                            os << "..";
                        } else {
                            os << ' ';
                        }
                    } else {
                        os << '-';
                    }
                    os << v->second;
                }
                if (!++v)
                    break;
                os << ' ';
            }
        }
        return os;
    }

private:
    void iInsert(const value_type * first, const value_type * last);

    Node m_node;
    NO_UNIQUE_ADDRESS allocator_type m_alloc;
};

//===========================================================================
template <std::integral T, typename A>
template <std::input_iterator InputIt>
    requires (std::is_convertible_v<decltype(*std::declval<InputIt>()), T>)
inline void IntegralSet<T, A>::assign(InputIt first, InputIt last) {
    clear();
    insert(first, last);
}

//===========================================================================
template <std::integral T, typename A>
template<std::input_iterator InputIt>
    requires (std::is_convertible_v<decltype(*std::declval<InputIt>()), T>)
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
    Iter & operator=(const Iter & from) = default;
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
    using reverse_iterator = reverse_circular_iterator<RangeIter>;

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

extern template class IntegralSet<int>;
extern template class IntegralSet<unsigned>;

extern template class IntegralSet<unsigned,
    std::pmr::polymorphic_allocator<unsigned>>;

} // namespace
