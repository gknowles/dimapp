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
*   Declarations
*
***/

class UnsignedSetIterator {
};


/****************************************************************************
*
*   UnsignedSet
*
***/

class UnsignedSet {
public:
    struct Node {
        union {
            unsigned * values;
            Node * nodes;
        };
        unsigned type : 3;
        unsigned depth : 5;
        unsigned base : 24;
        unsigned numBytes : 16;
        unsigned numValues : 16;

        Node();
    };
    
    using iterator = UnsignedSetIterator;

public:
    UnsignedSet();
    UnsignedSet(UnsignedSet && from);
    ~UnsignedSet();

    UnsignedSet & operator=(UnsignedSet && from);
    bool operator==(const UnsignedSet & right) const;

    iterator begin();
    iterator end();

    bool empty() const;
    size_t size() const;
    size_t maxSize() const;

    void clear();
    void insert(unsigned value);
    template<typename InputIt> void insert(InputIt first, InputIt last);
    void insert(std::initializer_list<unsigned> il);
    void insert(UnsignedSet && other);
    void insert(const UnsignedSet & other);
    void erase(unsigned value);
    void erase(unsigned first, unsigned last);
    void erase(const UnsignedSet & other);
    void intersect(UnsignedSet && other);
    void intersect(const UnsignedSet & other);
    bool includes(const UnsignedSet & other);
    void swap(UnsignedSet & other);

    size_t count(unsigned val) const;
    //find
    //equalRange
    //lowerBound
    //upperBound

private:
    Node m_node;
};

} // namespace
