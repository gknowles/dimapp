// Copyright Glen Knowles 2016 - 2017.
// Distributed under the Boost Software License, Version 1.0.
//
// list.h - dim core
#pragma once

#include "cppconf/cppconf.h"

namespace Dim {


/****************************************************************************
*
*   Declarations
*
***/


/****************************************************************************
*
*   ListMemberLink
*
***/

template <typename T> 
class ListMemberLink {
public:
    ListMemberLink();
    ~ListMemberLink();
    void reset();

    bool linked() const;

    T * prev() const;
    T * next() const;

private:
    template <typename T, ListMemberLink<T> T::*P> friend class List;

    void construct();
    void detach();
    void linkBefore(T * thisNode, T * next);
    void linkAfter(T * thisNode, T * prev);

    ListMemberLink * getLink(
        const T * node, 
        const T * otherNode, 
        const ListMemberLink * otherLink
    ) const;

    ListMemberLink * m_prevLink;
    T * m_nextNode;
};

//===========================================================================
template<typename T>
ListMemberLink<T>::ListMemberLink () {
    construct();
}

//===========================================================================
template<typename T>
ListMemberLink<T>::~ListMemberLink () {
    detach();
}

//===========================================================================
template<typename T>
void ListMemberLink<T>::reset() {
    detach();
    construct();
}

//===========================================================================
template<typename T>
bool ListMemberLink<T>::linked() const {
    return m_prevLink != this;
}

//===========================================================================
template<typename T>
T * ListMemberLink<T>::prev() const {
    return m_prevLink->m_prevLink->next();
}

//===========================================================================
template<typename T>
T * ListMemberLink<T>::next() const {
    return ((uintptr_t) m_nextNode & 1) ? nullptr : m_nextNode;
}

//===========================================================================
template<typename T>
void ListMemberLink<T>::construct() {
    m_prevLink = this;
    m_nextNode = (T *) ((uintptr_t) this + 1);
}

//===========================================================================
template<typename T>
void ListMemberLink<T>::detach() {
    getLink(m_nextNode, m_prevLink->m_nextNode, this)->m_prevLink = m_prevLink;
    m_prevLink->m_nextNode = m_nextNode;
}

//===========================================================================
template<typename T>
void ListMemberLink<T>::linkBefore(T * thisNode, T * next) {
    detach();
    auto nextLink = getLink(next, thisNode, this);
    m_prevLink = nextLink->m_prevLink;
    m_nextNode = next;

    m_prevLink->m_nextNode = thisNode;
    nextLink->m_prevLink = this;
}

//===========================================================================
template<typename T>
void ListMemberLink<T>::linkAfter(T * thisNode, T * prev) {
    detach();
    m_prevLink = getLink(prev, thisNode, this);
    m_nextNode = m_prevLink.m_nextNode;

    m_prevLink->m_nextNode = thisNode;
    getLink(m_nextNode, thisNode, this)->m_prevLink = this;
}

//===========================================================================
template<typename T>
ListMemberLink<T> * ListMemberLink<T>::getLink(
    const T * node,
    const T * otherNode,
    const ListMemberLink * otherLink
) const {
    auto off = (uintptr_t) otherLink - ((uintptr_t) otherNode & ~1);
    return (ListMemberLink *) (((uintptr_t) node & ~1) + off);
}


/****************************************************************************
*
*   List
*
***/

template <typename T, ListMemberLink<T> T::*P>
class List {
public:
    List() {}
    List(List && from);
    ~List();

    List & operator=(List && from);
    bool operator==(const List & right) const;

    T & front();
    const T & front() const;
    T & back();
    const T & back() const;
    bool empty() const;
    size_t size() const;
    void clear();
    void insert(const T * pos, T * value);
    void insert(const T * pos, T * first, T * last);
    void insert(const T * pos, List && other);
    void insertAfter(const T * pos, T * value);
    void insertAfter(const T * pos, T * first, T * last);
    void insertAfter(const T * pos, List && other);
    void unlinkAll();
    void unlink(T * value);
    void unlink(T * first, T * last);
    void pushBack(T * value);
    void pushBack(List && other);
    T * popBack();
    void pushFront(T * value);
    void pushFront(List && other);
    T * popFront();
    void swap(List & other);

private:
    T * thisNode();

    ListMemberLink<T> m_base;
};


//===========================================================================
template <typename T, ListMemberLink<T> T::*P>
List<T,P>::List(List && from) {
    swap(from);
}

//===========================================================================
template <typename T, ListMemberLink<T> T::*P>
List<T,P>::~List() {
    unlinkAll();
}

//===========================================================================
template <typename T, ListMemberLink<T> T::*P>
List<T,P> & List<T,P>::operator=(List && from) {
    swap(from);
    from.unlinkAll();
}

//===========================================================================
template <typename T, ListMemberLink<T> T::*P>
bool List<T,P>::operator==(const List & right) const;

//===========================================================================
template <typename T, ListMemberLink<T> T::*P>
T & List<T,P>::front() {
    assert(!empty());
    return *m_base.next();
}

//===========================================================================
template <typename T, ListMemberLink<T> T::*P>
const T & List<T,P>::front() const {
    return const_cast<List *>(this)->front();
}

//===========================================================================
template <typename T, ListMemberLink<T> T::*P>
T & List<T,P>::back() {
    assert(!empty());
    return *m_base.prev();
}

//===========================================================================
template <typename T, ListMemberLink<T> T::*P>
const T & List<T,P>::back() const {
    return const_cast<List *>(this)->back();
}

//===========================================================================
template <typename T, ListMemberLink<T> T::*P>
bool List<T,P>::empty() const {
    return !m_base.linked();
}

//===========================================================================
template <typename T, ListMemberLink<T> T::*P>
size_t List<T,P>::size() const {
    size_t num = 0;
    auto cur = m_base.m_prevLink;
    for (; cur != &m_base; cur = cur->m_prevLink)
        num += 1;
    return num;
}

//===========================================================================
template <typename T, ListMemberLink<T> T::*P>
void List<T,P>::clear() {
    while (!empty())
        delete m_base.m_nextNode;
}

//===========================================================================
template <typename T, ListMemberLink<T> T::*P>
void List<T,P>::insert(const T * pos, T * value) {
    (value->*P).linkBefore(value, pos);
    return value;
}

//===========================================================================
template <typename T, ListMemberLink<T> T::*P>
void List<T,P>::insert(const T * pos, T * first, T * last);

//===========================================================================
template <typename T, ListMemberLink<T> T::*P>
void List<T,P>::insert(const T * pos, List && other);

//===========================================================================
template <typename T, ListMemberLink<T> T::*P>
void List<T,P>::insertAfter(const T * pos, T * value) {
    (value->*P).linkAfter(value, pos);
    return value;
}

//===========================================================================
template <typename T, ListMemberLink<T> T::*P>
void List<T,P>::insertAfter(const T * pos, T * first, T * last);

//===========================================================================
template <typename T, ListMemberLink<T> T::*P>
void List<T,P>::insertAfter(const T * pos, List && other);

//===========================================================================
template <typename T, ListMemberLink<T> T::*P>
void List<T,P>::unlinkAll() {
    while (!empty())
        m_base.m_prevLink->reset();
}

//===========================================================================
template <typename T, ListMemberLink<T> T::*P>
void List<T,P>::unlink(T * value) {
    (value->*P).reset();
}

//===========================================================================
template <typename T, ListMemberLink<T> T::*P>
void List<T,P>::unlink(T * first, T * last);

//===========================================================================
template <typename T, ListMemberLink<T> T::*P>
void List<T,P>::pushBack(T * value) {
    (value->*P).linkBefore(value, thisNode());
}

//===========================================================================
template <typename T, ListMemberLink<T> T::*P>
void List<T,P>::pushBack(List && other);

//===========================================================================
template <typename T, ListMemberLink<T> T::*P>
T * List<T,P>::popBack() {
    auto node = m_base.prev();
    if (node)
        m_base.m_prevLink->reset();
    return node;
}

//===========================================================================
template <typename T, ListMemberLink<T> T::*P>
void List<T,P>::pushFront(T * value) {
    (value->*P).linkAfter(value, thisNode());
}

//===========================================================================
template <typename T, ListMemberLink<T> T::*P>
void List<T,P>::pushFront(List && other);

//===========================================================================
template <typename T, ListMemberLink<T> T::*P>
T * List<T,P>::popFront() {
    auto node = m_base.next();
    if (node)
        (node->*P).reset();
    return node;
}

//===========================================================================
template <typename T, ListMemberLink<T> T::*P>
void List<T,P>::swap(List & other) {
    swap(m_base.prevLink().m_next, from.m_base.prevLink().m_next);
    swap(m_base.nextLink().m_prev, from.m_base.nextLink().m_prev);
    swap(m_base, from.m_base);
}

//===========================================================================
template <typename T, ListMemberLink<T> T::*P>
T * List<T,P>::thisNode() {
    return (T*) ((uintptr_t)&m_base - (size_t)&((T*)0->*P));
}

} // namespace
