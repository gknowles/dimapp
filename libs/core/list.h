// Copyright Glen Knowles 2016 - 2017.
// Distributed under the Boost Software License, Version 1.0.
//
// list.h - dim core
#pragma once

#include "cppconf/cppconf.h"

namespace Dim {

// forward declarations
template <typename T> class List;


/****************************************************************************
*
*   ListMemberLink
*
***/

template <typename T> class ListMemberLink {
public:
    ListMemberLink();
    ~ListMemberLink();
    void reset();

    bool linked() const;
    T * next() const;
    T * prev() const;

//private:
    friend class List<T>;
    void construct(ptrdiff_t offset);
    void unlink();
    void insert(T * node, ListMemberLink * nextLink);
    void insertAfter(T * node, ListMemberLink * prevLink);

    bool isLast() const;

    T * m_nextNode;
    ListMemberLink * m_prevLink;
};

//===========================================================================
template<typename T>
ListMemberLink<T>::ListMemberLink () {
    construct(0);
}

//===========================================================================
template<typename T>
ListMemberLink<T>::~ListMemberLink () {
    reset();
}

//===========================================================================
template<typename T>
void ListMemberLink<T>::reset() {
    unlink();
    construct(0);
}

//===========================================================================
template<typename T>
bool ListMemberLink<T>::linked() const {
    return m_prevLink != this;
}

//===========================================================================
template<typename T>
T * ListMemberLink<T>::next() const {
    return isLast() ? nullptr : m_nextNode;
}

//===========================================================================
template<typename T>
T * ListMemberLink<T>::prev() const {
    if (m_prevLink->isLast())
        return nullptr;
    uintptr_t offset = (uintptr_t) this - (uintptr_t) m_prevLink->m_nextNode;
    return (T *) ((uintptr_t) m_prevLink + offset);
}

//===========================================================================
template<typename T>
void ListMemberLink<T>::construct(ptrdiff_t offset) {
    m_next = (T *) ((uintptr_t) this + 1 - offset);
    m_prev = this;
}

//===========================================================================
template<typename T>
void ListMemberLink<T>::unlink() {
    uintptr_t offset = (uintptr_t) m_prevLink->m_nextNode - (uintptr_t) this;
    m_prevLink->m_nextNode = m_nextNode;
    auto nextLink = 
        (ListMemberLink *) ((uintptr_t) m_nextMode + offset - isLast());
    nextLink->m_prevLink = m_prevLink;
}

//===========================================================================
template<typename T>
void ListMemberLink<T>::insert(T * nextNode, ListMemberLink * nextLink) {
    unlink();
    m_prevLink = nextLink->m_prevLink;
    m_nextNode = nextNode;

    uintptr_t offset = (uintptr_t) nextNode - (uintptr_t) nextLink;
    m_prevLink->m_nextNode = 
        (T *) ((uintptr_t) this - offset + m_prevLink->isLast())
    nextLink->m_prevLink = this;
}

//===========================================================================
template<typename T>
void ListMemberLink<T>::insertAfter(T * prevNode, ListMemberLink * prevLink) {
    unlink();
    m_prevLink = prevLink;
    m_nextNode = prevLink->m_nextNode;

    uintptr_t offset = (uintptr_t) prevNode - (uintptr_t) prevLink;
    prevLink->m_nextNode = (T *) ((uintptr_t) this - offset);
    auto nextLink = 
        (ListMemberLink *) ((uintptr_t) m_nextNode + offset - isLast());
    nextLink->m_prevLink = this;
}

//===========================================================================
template<typename T>
bool ListMemberLink<T>::isLast() const {
    return (uintptr_t) m_nextNode & 1;
}


/****************************************************************************
*
*   List
*
***/

template <typename T> class List {
public:
    List();
    List(List && from);
    ~List();

    List & operator=(List && from);
    bool operator==(const List & right) const;

    T & front();
    const T & front() const;
    T & back();
    const T & back() const;
    bool empty() const;
    int size() const;
    void clear();
    T * insert(const T * pos, T * value);
    T * insert(const T * pos, T * first, T * last);
    T * insert(const T * pos, List && other);
    T * insertAfter(const T * pos, T * value);
    T * insertAfter(const T * pos, T * first, T * last);
    T * insertAfter(const T * pos, List && other);
    void removeAll();
    T * remove(T * value);
    T * remove(T * first, T * last);
    void pushBack(T * value);
    void pushBack(List && other);
    T * popBack();
    void pushFront(T * value);
    void pushFront(List && other);
    T * popFront();
    void swap(List & other);

private:
    ListMemberLink m_base;
};

} // namespace
