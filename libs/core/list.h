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

template <typename T> struct ListMemberLinkBase {};

template <typename T, ListMemberLinkBase<T> T::*M> 
class List;


/****************************************************************************
*
*   ListMemberLink
*
***/

template <typename T, ListMemberLinkBase<T> T::*M> 
class ListMemberLink : public ListMemberLinkBase<T> {
public:
    ListMemberLink(decltype(M) ignored);
    ~ListMemberLink();
    void reset();

    bool linked() const;
    T * prev() const;
    T * next() const;

private:
    friend class List<T,M>;

    static ListMemberLink & getLink(T * node);

    void construct();
    void detach();
    void link(T * thisNode, T * next);
    void linkAfter(T * thisNode, T * prev);

    ListMemberLink & prevLink() const;
    ListMemberLink & nextLink() const;
    bool isFirst() const;
    bool isLast() const;

    union { T * node; ListMemberLink * link; } m_next;
    union { T * node; ListMemberLink * link; } m_prev;
};

//===========================================================================
// static
template<typename T, ListMemberLinkBase<T> T::*M>
ListMemberLink<T,M> & ListMemberLink<T,M>::getLink(T * node) {
    return ((uintptr_t) node & 1) 
        ? *(ListMemberLink *)((uintptr_t) node - 1)
        : static_cast<ListMemberLink &>(node->*M);
}

//===========================================================================
template<typename T, ListMemberLinkBase<T> T::*M>
ListMemberLink<T,M>::ListMemberLink (decltype(M) ignored) {
    construct();
}

//===========================================================================
template<typename T, ListMemberLinkBase<T> T::*M>
ListMemberLink<T,M>::~ListMemberLink () {
    detach();
}

//===========================================================================
template<typename T, ListMemberLinkBase<T> T::*M>
void ListMemberLink<T,M>::reset() {
    detach();
    construct();
}

//===========================================================================
template<typename T, ListMemberLinkBase<T> T::*M>
bool ListMemberLink<T,M>::linked() const {
    return prevLink() != this;
}

//===========================================================================
template<typename T, ListMemberLinkBase<T> T::*M>
T * ListMemberLink<T,M>::prev() const {
    return isFirst() ? nullptr : m_prev.node;
}

//===========================================================================
template<typename T, ListMemberLinkBase<T> T::*M>
T * ListMemberLink<T,M>::next() const {
    return isLast() ? nullptr : m_next.node;
}

//===========================================================================
template<typename T, ListMemberLinkBase<T> T::*M>
void ListMemberLink<T,M>::construct() {
    m_prev.link = m_next.link = (ListMemberLink *) ((uintptr_t) this + 1);
}

//===========================================================================
template<typename T, ListMemberLinkBase<T> T::*M>
void ListMemberLink<T,M>::detach() {
    prevLink().m_next = m_next;
    nextLink().m_prev = m_prev;
}

//===========================================================================
template<typename T, ListMemberLinkBase<T> T::*M>
void ListMemberLink<T,M>::link(T * thisNode, T * next) {
    detach();
    m_prev = next->m_prev;
    m_next.node = next;

    prevLink().m_next.node = thisNode;
    nextLink().m_prev.node = thisNode;
}

//===========================================================================
template<typename T, ListMemberLinkBase<T> T::*M>
void ListMemberLink<T,M>::linkAfter(T * thisNode, T * prev) {
    detach();
    m_prev.node = prev;
    m_next = prev->m_next;

    prevLink().m_next.node = thisNode;
    nextLink().m_prev.node = thisNode;
}

//===========================================================================
template<typename T, ListMemberLinkBase<T> T::*M>
ListMemberLink<T,M> & ListMemberLink<T,M>::prevLink() const {
    return getLink(m_prev.node);
}

//===========================================================================
template<typename T, ListMemberLinkBase<T> T::*M>
ListMemberLink<T,M> & ListMemberLink<T,M>::nextLink() const {
    return getLink(m_next.node);
}

//===========================================================================
template<typename T, ListMemberLinkBase<T> T::*M>
bool ListMemberLink<T,M>::isFirst() const {
    return (uintptr_t) m_prev.link & 1;
}

//===========================================================================
template<typename T, ListMemberLinkBase<T> T::*M>
bool ListMemberLink<T,M>::isLast() const {
    return (uintptr_t) m_next.link & 1;
}


/****************************************************************************
*
*   List
*
***/

template <typename T, ListMemberLinkBase<T> T::*M>
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
    int size() const;
    void clear();
    T * link(const T * pos, T * value);
    T * link(const T * pos, T * first, T * last);
    T * link(const T * pos, List && other);
    T * linkAfter(const T * pos, T * value);
    T * linkAfter(const T * pos, T * first, T * last);
    T * linkAfter(const T * pos, List && other);
    void unlinkAll();
    void unlink(T * value);
    void unlink(T * first, T * last);
    void linkBack(T * value);
    void linkBack(List && other);
    T * unlinkBack();
    void linkFront(T * value);
    void linkFront(List && other);
    T * unlinkFront();
    void swap(List & other);

private:
    ListMemberLink<T,M> m_base;
};


//===========================================================================
template <typename T, ListMemberLinkBase<T> T::*M>
List<T,M>::List(List && from) {
    swap(from);
}

//===========================================================================
template <typename T, ListMemberLinkBase<T> T::*M>
List<T,M>::~List() {
    unlinkAll();
}

//===========================================================================
template <typename T, ListMemberLinkBase<T> T::*M>
List<T,M> & List<T,M>::operator=(List && from) {
    swap(from);
    from.unlinkAll();
}

//===========================================================================
template <typename T, ListMemberLinkBase<T> T::*M>
bool List<T,M>::operator==(const List & right) const;

//===========================================================================
template <typename T, ListMemberLinkBase<T> T::*M>
T & List<T,M>::front() {
    assert(!empty());
    return *m_base.m_next.node;
}

//===========================================================================
template <typename T, ListMemberLinkBase<T> T::*M>
const T & List<T,M>::front() const {
    assert(!empty());
    return *m_base.m_next.node;
}

//===========================================================================
template <typename T, ListMemberLinkBase<T> T::*M>
T & List<T,M>::back() {
    assert(!empty());
    return *m_base.m_prev.node;
}

//===========================================================================
template <typename T, ListMemberLinkBase<T> T::*M>
const T & List<T,M>::back() const {
    assert(!empty());
    return *m_base.m_prev.node;
}

//===========================================================================
template <typename T, ListMemberLinkBase<T> T::*M>
bool List<T,M>::empty() const {
    return !m_base.linked();
}

//===========================================================================
template <typename T, ListMemberLinkBase<T> T::*M>
int List<T,M>::size() const;

//===========================================================================
template <typename T, ListMemberLinkBase<T> T::*M>
void List<T,M>::clear() {
    while (!empty())
        delete m_base.m_next.node;
}

//===========================================================================
template <typename T, ListMemberLinkBase<T> T::*M>
T * List<T,M>::link(const T * pos, T * value) {
    m_base.getLink(value).link(value, pos);
    return value;
}

//===========================================================================
template <typename T, ListMemberLinkBase<T> T::*M>
T * List<T,M>::link(const T * pos, T * first, T * last);

//===========================================================================
template <typename T, ListMemberLinkBase<T> T::*M>
T * List<T,M>::link(const T * pos, List && other);

//===========================================================================
template <typename T, ListMemberLinkBase<T> T::*M>
T * List<T,M>::linkAfter(const T * pos, T * value) {
    m_base.getLink(value).linkAfter(value, pos);
    return value;
}

//===========================================================================
template <typename T, ListMemberLinkBase<T> T::*M>
T * List<T,M>::linkAfter(const T * pos, T * first, T * last);

//===========================================================================
template <typename T, ListMemberLinkBase<T> T::*M>
T * List<T,M>::linkAfter(const T * pos, List && other);

//===========================================================================
template <typename T, ListMemberLinkBase<T> T::*M>
void List<T,M>::unlinkAll() {
    while (!empty())
        m_base.getLink(m_base.m_next.node).reset();
}

//===========================================================================
template <typename T, ListMemberLinkBase<T> T::*M>
void List<T,M>::unlink(T * value) {
    m_base.getLink(value).reset();
}

//===========================================================================
template <typename T, ListMemberLinkBase<T> T::*M>
void List<T,M>::unlink(T * first, T * last);

//===========================================================================
template <typename T, ListMemberLinkBase<T> T::*M>
void List<T,M>::linkBack(T * value) {
    m_base.getLink(value).link(value, this);
}

//===========================================================================
template <typename T, ListMemberLinkBase<T> T::*M>
void List<T,M>::linkBack(List && other);

//===========================================================================
template <typename T, ListMemberLinkBase<T> T::*M>
T * List<T,M>::unlinkBack() {
    if (empty()) 
        return nullptr;
    auto node = m_base.m_prev.node;
    m_base.getLink(node).reset();
    return node;
}

//===========================================================================
template <typename T, ListMemberLinkBase<T> T::*M>
void List<T,M>::linkFront(T * value) {
    (value->*M).linkAfter(value, this);
}

//===========================================================================
template <typename T, ListMemberLinkBase<T> T::*M>
void List<T,M>::linkFront(List && other);

//===========================================================================
template <typename T, ListMemberLinkBase<T> T::*M>
T * List<T,M>::unlinkFront() {
    if (empty()) 
        return nullptr;
    auto node = m_base.m_next.node;
    m_base.getLink(node).reset();
    return node;
}

//===========================================================================
template <typename T, ListMemberLinkBase<T> T::*M>
void List<T,M>::swap(List & other) {
    swap(m_base.prevLink().m_next, from.m_base.prevLink().m_next);
    swap(m_base.nextLink().m_prev, from.m_base.nextLink().m_prev);
    swap(m_base, from.m_base);
}


} // namespace
