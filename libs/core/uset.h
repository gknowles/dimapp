// Copyright Glen Knowles 2017.
// Distributed under the Boost Software License, Version 1.0.
//
// uset.h - dim core
#pragma once

#include "cppconf/cppconf.h"

#include <cstdint>
#include <initializer_list>

namespace Dim {


/****************************************************************************
*
*   UnsignedSet
*
***/

class UnsignedSet {
public:
    class NodeIterator;
    struct NodeRange;
    class Iterator;

    using value_type = unsigned;
    using reference = value_type&;
    using const_reference = const value_type&;
    using iterator = Iterator;
    using const_iterator = Iterator;
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
    UnsignedSet(UnsignedSet && from);
    ~UnsignedSet();

    UnsignedSet & operator=(UnsignedSet && from);

    // iterators
    iterator begin();
    iterator end();
    const_iterator begin() const;
    const_iterator end() const;

    NodeRange nodes();

    // capacity
    bool empty() const;
    size_t size() const;

    // modify
    void clear();
    void insert(unsigned value);
    template<typename InputIt> void insert(InputIt first, InputIt last);
    void insert(std::initializer_list<unsigned> il);
    void insert(UnsignedSet && other);
    void insert(const UnsignedSet & other);
    void erase(unsigned value);
    void erase(const UnsignedSet & other);
    void intersect(UnsignedSet && other);
    void intersect(const UnsignedSet & other);
    void swap(UnsignedSet & other);

    // compare
    int compare(const UnsignedSet & right) const;
    bool includes(const UnsignedSet & other) const;
    bool intersects(const UnsignedSet & other) const;
    bool operator==(const UnsignedSet & right) const;
    bool operator!=(const UnsignedSet & right) const;
    bool operator<(const UnsignedSet & right) const;
    bool operator>(const UnsignedSet & right) const;
    bool operator<=(const UnsignedSet & right) const;
    bool operator>=(const UnsignedSet & right) const;

    // search
    size_t count(unsigned val) const;

    iterator find(unsigned val);
    const_iterator find(unsigned val) const;
    std::pair<iterator, iterator> equalRange(unsigned val);
    std::pair<const_iterator, const_iterator> equalRange(unsigned val) const;
    iterator lowerBound(unsigned val);
    const_iterator lowerBound(unsigned val) const;
    iterator upperBound(unsigned val);
    const_iterator upperBound(unsigned val) const;

private:
    void iInsert(const unsigned * first, const unsigned * last);

    Node m_node;
};

//===========================================================================
template<typename InputIt> 
inline void UnsignedSet::insert(InputIt first, InputIt last) {
    for (; first != last; ++first)
        insert(*first);
}

//===========================================================================
template<> 
inline void UnsignedSet::insert<unsigned *>(
    unsigned * first, 
    unsigned * last
) {
    iInsert(first, last);
}

//===========================================================================
template<> 
inline void UnsignedSet::insert<const unsigned *>(
    const unsigned * first, 
    const unsigned * last
) {
    iInsert(first, last);
}

//===========================================================================
inline void UnsignedSet::insert(std::initializer_list<unsigned> il) {
    iInsert(il.begin(), il.end());
}


/****************************************************************************
*
*   UnsignedSet::NodeIterator
*
***/

class UnsignedSet::NodeIterator {
public:
    using iterator_category = std::input_iterator_tag;
    using value_type = const Node;
    using difference_type = ptrdiff_t;
    using pointer = value_type*;
    using reference = value_type&;
public:
    NodeIterator() {}
    NodeIterator(const Node * node);
    NodeIterator & operator++();
    explicit operator bool() const { return m_node; }
    bool operator!= (const NodeIterator & right) const;
    const Node & operator*() const { return *m_node; }
    const Node * operator->() const { return m_node; }
private:
    const Node * m_node{nullptr};
};


/****************************************************************************
*
*   UnsignedSet::NodeRange
*
***/

struct UnsignedSet::NodeRange {
    NodeIterator m_first;
    NodeIterator begin() { return m_first; }
    NodeIterator end() { return {}; }
};


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
    using pointer = value_type*;
    using reference = value_type&;
public:
    Iterator() {}
    Iterator(const Node * node, value_type value = 0);
    Iterator & operator++();
    explicit operator bool() const { return !!m_node; }
    bool operator!= (const Iterator & right) const;
    unsigned operator*() const { return m_value; }
    const unsigned * operator->() const { return &m_value; }
private:
    NodeIterator m_node;
    value_type m_value{0};
};


} // namespace
