// Copyright Glen Knowles 2017.
// Distributed under the Boost Software License, Version 1.0.
//
// uset.h - dim core
#pragma once

#include "cppconf/cppconf.h"

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
    using iterator = UnsignedSetIterator;

public:
    UnsignedSet() {}
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
    void swap(UnsignedSet & other);

    size_t count(unsigned val) const;
    //find
    //equalRange
    //lowerBound
    //upperBound

private:
    
};


//===========================================================================
UnsignedSet::UnsignedSet(UnsignedSet && from) {
    swap(from);
}

//===========================================================================
UnsignedSet::~UnsignedSet() {
    clear();
}

//===========================================================================
UnsignedSet & UnsignedSet::operator=(UnsignedSet && from) {
    clear();
    swap(from);
    return *this;
}

//===========================================================================
bool UnsignedSet::operator==(const UnsignedSet & right) const;

//===========================================================================
bool UnsignedSet::empty() const {
    return true;
}

//===========================================================================
size_t UnsignedSet::size() const {
    size_t num = 0;
    return num;
}

//===========================================================================
void UnsignedSet::clear() {
}

//===========================================================================
void UnsignedSet::insert(unsigned value) {
}

//===========================================================================
void UnsignedSet::erase(unsigned value) {
}

//===========================================================================
void UnsignedSet::swap(UnsignedSet & other) {
}

} // namespace
