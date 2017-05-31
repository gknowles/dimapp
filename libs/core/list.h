// Copyright Glen Knowles 2016 - 2017.
// Distributed under the Boost Software License, Version 1.0.
//
// list.h - dim core
#pragma once

#include "cppconf/cppconf.h"

#include <algorithm>

namespace Dim {


/****************************************************************************
*
*   Declarations
*
***/

struct LinkDefault;


/****************************************************************************
*
*   ListBaseLink
*
***/

template <typename T, typename Tag = LinkDefault> 
class ListBaseLink {
public:
    ListBaseLink();
    ~ListBaseLink();

protected:
    void unlink();
    bool linked() const;

private:
    template <typename T, typename Tag> friend class List;

    void construct();
    void detach();

    ListBaseLink * m_prevLink;
    ListBaseLink * m_nextLink;
};

//===========================================================================
template<typename T, typename Tag>
ListBaseLink<T, Tag>::ListBaseLink () {
    construct();
}

//===========================================================================
template<typename T, typename Tag>
ListBaseLink<T, Tag>::~ListBaseLink () {
    detach();
}

//===========================================================================
template<typename T, typename Tag>
void ListBaseLink<T, Tag>::unlink() {
    assert(linked());
    detach();
    construct();
}

//===========================================================================
template<typename T, typename Tag>
bool ListBaseLink<T, Tag>::linked() const {
    return m_prevLink != this;
}

//===========================================================================
template<typename T, typename Tag>
void ListBaseLink<T, Tag>::construct() {
    m_prevLink = m_nextLink = this;
}

//===========================================================================
template<typename T, typename Tag>
void ListBaseLink<T, Tag>::detach() {
    m_nextLink->m_prevLink = m_prevLink;
    m_prevLink->m_nextLink = m_nextLink;
}


/****************************************************************************
*
*   ListIterator
*
***/

template <typename List, typename T>
class ListIterator {
    List * m_container{nullptr};
    T * m_current{nullptr};
public:
    ListIterator() {}
    ListIterator(List * container, T * node);
    bool operator!=(const ListIterator & right);
    ListIterator & operator++();
    T & operator*();
    T * operator->();
};

//===========================================================================
template <typename List, typename T>
ListIterator<List, T>::ListIterator(List * container, T * node) 
    : m_container{container}
    , m_current{node} 
{}

//===========================================================================
template <typename List, typename T>
bool ListIterator<List, T>::operator!=(const ListIterator & right) {
    return m_current != right.m_current;
}

//===========================================================================
template <typename List, typename T>
ListIterator<List, T> & ListIterator<List, T>::operator++() {
    m_current = m_container->next(m_current);
    return *this;
}

//===========================================================================
template <typename List, typename T>
T & ListIterator<List, T>::operator*() {
    assert(m_current);
    return *m_current;
}

//===========================================================================
template <typename List, typename T>
T * ListIterator<List, T>::operator->() { 
    return m_current; 
}


/****************************************************************************
*
*   List
*
***/

template <typename T, typename Tag = LinkDefault>
class List {
public:
    using iterator = ListIterator<List, T>;
    using const_iterator = ListIterator<const List, const T>;

public:
    List() {}
    List(List && from);
    ~List();
    List & operator=(const List & from) = delete;

    bool operator==(const List & right) const;

    bool empty() const;
    size_t size() const;
    void clear();

    iterator begin();
    iterator end();
    const_iterator begin() const;
    const_iterator end() const;
    
    T * front();
    const T * front() const;
    T * back();
    const T * back() const;
    T * next(T * node);
    const T * next(const T * node) const;
    T * prev(T * node);
    const T * prev(const T * node) const;
    void link(T * pos, T * value);
    void link(T * pos, T * first, T * last);
    void link(T * pos, List && other);
    void linkAfter(T * pos, T * value);
    void linkAfter(T * pos, T * first, T * last);
    void linkAfter(T * pos, List && other);
    void unlinkAll();
    void unlink(T * value);
    void pushBack(T * value);
    void pushBack(List && other);
    T * popBack();
    void pushFront(T * value);
    void pushFront(List && other);
    T * popFront();
    void swap(List & other);

private:
    void linkBase(
        ListBaseLink<T, Tag> * pos, 
        ListBaseLink<T, Tag> * first,
        ListBaseLink<T, Tag> * last
    );
    void linkBaseAfter(
        ListBaseLink<T, Tag> * pos, 
        ListBaseLink<T, Tag> * first,
        ListBaseLink<T, Tag> * last
    );

    ListBaseLink<T, Tag> m_base;
};

//===========================================================================
template <typename T, typename Tag>
List<T, Tag>::List(List && from) {
    swap(from);
}

//===========================================================================
template <typename T, typename Tag>
List<T, Tag>::~List() {
    unlinkAll();
}

//===========================================================================
template <typename T, typename Tag>
bool List<T, Tag>::operator==(const List & right) const {
    return std::equal(begin(), end(), right.begin(), right.end());
}

//===========================================================================
template <typename T, typename Tag>
bool List<T, Tag>::empty() const {
    return !m_base.linked();
}

//===========================================================================
template <typename T, typename Tag>
size_t List<T, Tag>::size() const {
    size_t num = 0;
    auto cur = m_base.m_nextLink;
    for (; cur != &m_base; cur = cur->m_nextLink)
        num += 1;
    return num;
}

//===========================================================================
template <typename T, typename Tag>
void List<T, Tag>::clear() {
    while (auto ptr = front())
        delete ptr;
}

//===========================================================================
template <typename T, typename Tag>
auto List<T, Tag>::begin() -> iterator {
    return iterator{this, front()};
}

//===========================================================================
template <typename T, typename Tag>
auto List<T, Tag>::end() -> iterator {
    return iterator{};
}

//===========================================================================
template <typename T, typename Tag>
auto List<T, Tag>::begin() const -> const_iterator {
    return const_iterator{this, front()};
}

//===========================================================================
template <typename T, typename Tag>
auto List<T, Tag>::end() const -> const_iterator {
    return const_iterator{};
}

//===========================================================================
template <typename T, typename Tag>
bool List<T, Tag>::operator==(const List & right) const;

//===========================================================================
template <typename T, typename Tag>
T * List<T, Tag>::front() {
    return m_base.linked() ? static_cast<T *>(m_base.m_nextLink) : nullptr;
}

//===========================================================================
template <typename T, typename Tag>
const T * List<T, Tag>::front() const {
    return const_cast<List *>(this)->front();
}

//===========================================================================
template <typename T, typename Tag>
T * List<T, Tag>::back() {
    return m_base.linked() ? static_cast<T *>(m_base.m_prevLink) : nullptr;
}

//===========================================================================
template <typename T, typename Tag>
const T * List<T, Tag>::back() const {
    return const_cast<List *>(this)->back();
}

//===========================================================================
template <typename T, typename Tag>
T * List<T, Tag>::next(T * node) {
    return node->m_nextLink == &m_base 
        ? nullptr 
        : static_cast<T *>(node->m_nextLink);
}

//===========================================================================
template <typename T, typename Tag>
const T * List<T, Tag>::next(const T * node) const {
    return const_cast<List *>(this)->next(node);
}

//===========================================================================
template <typename T, typename Tag>
T * List<T, Tag>::prev(T * node) {
    return node->m_prevLink == &m_base 
        ? nullptr 
        : static_cast<T *>(node->m_prevLink);
}

//===========================================================================
template <typename T, typename Tag>
const T * List<T, Tag>::prev(const T * node) const {
    return const_cast<List *>(this)->prev(node);
}

//===========================================================================
template <typename T, typename Tag>
void List<T, Tag>::linkBase(
    ListBaseLink<T, Tag> * next, 
    ListBaseLink<T, Tag> * first, 
    ListBaseLink<T, Tag> * last
) {
    auto lastIncl = last->m_prevLink;

    // detach [first, last)
    last->m_prevLink = first->m_prevLink;
    first->m_prevLink->m_nextLink = last;

    // update [first, lastIncl] pointers to list
    first->m_prevLink = next->m_prevLink;
    lastIncl->m_nextLink = next;

    // update list's pointers to [first, lastIncl] segment
    first->m_prevLink->m_nextLink = first;
    next->m_prevLink = lastIncl;
}

//===========================================================================
template <typename T, typename Tag>
void List<T, Tag>::link(T * pos, T * value) {
    auto first = static_cast<ListBaseLink<T, Tag> *>(value);
    linkBase(pos, first, first->m_nextLink);
}

//===========================================================================
template <typename T, typename Tag>
void List<T, Tag>::link(T * pos, T * first, T * last) {
    linkBase(pos, first, last);
}

//===========================================================================
template <typename T, typename Tag>
void List<T, Tag>::link(T * pos, List && other) {
    linkBase(pos, other.front(), other.back());
}

//===========================================================================
template <typename T, typename Tag>
void List<T, Tag>::linkBaseAfter(
    ListBaseLink<T, Tag> * prev, 
    ListBaseLink<T, Tag> * first, 
    ListBaseLink<T, Tag> * last
) {
    auto lastIncl = last->m_prevLink;

    // detach [first, last)
    last->m_prevLink = first->m_prevLink;
    first->m_prevLink->m_nextLink = last;

    // update [first, lastIncl] pointers to list
    first->m_prevLink = prev;
    lastIncl->m_nextLink = prev->m_nextLink;

    // update list's pointers to [first, lastIncl] segment
    prev->m_nextLink = first;
    lastIncl->m_nextLink->m_prevLink = lastIncl;
}

//===========================================================================
template <typename T, typename Tag>
void List<T, Tag>::linkAfter(T * pos, T * value) {
    auto first = static_cast<ListBaseLink<T, Tag> *>(value);
    linkBaseAfter(pos, first, first->m_nextLink);
}

//===========================================================================
template <typename T, typename Tag>
void List<T, Tag>::linkAfter(T * pos, T * first, T * last) {
    linkBaseAfter(pos, first, last);
}

//===========================================================================
template <typename T, typename Tag>
void List<T, Tag>::linkAfter(T * pos, List && other) {
    linkBaseAfter(pos, other.front(), other.back());
}

//===========================================================================
template <typename T, typename Tag>
void List<T, Tag>::unlinkAll() {
    while (auto ptr = front())
        unlink(ptr);
}

//===========================================================================
template <typename T, typename Tag>
void List<T, Tag>::unlink(T * value) {
    static_cast<ListBaseLink<T, Tag> *>(value)->unlink();
}

//===========================================================================
template <typename T, typename Tag>
void List<T, Tag>::pushBack(T * value) {
    link(static_cast<T *>(&m_base), value);
}

//===========================================================================
template <typename T, typename Tag>
void List<T, Tag>::pushBack(List && other) {
    link(static_cast<T *>(&m_base), other);
}

//===========================================================================
template <typename T, typename Tag>
T * List<T, Tag>::popBack() {
    auto link = back();
    if (link)
        link->unlink();
    return link;
}

//===========================================================================
template <typename T, typename Tag>
void List<T, Tag>::pushFront(T * value) {
    linkAfter(static_cast<T *>(&m_base), value);
}

//===========================================================================
template <typename T, typename Tag>
void List<T, Tag>::pushFront(List && other) {
    linkAfter(static_cast<T *>(&m_base), other);
}

//===========================================================================
template <typename T, typename Tag>
T * List<T, Tag>::popFront() {
    auto link = front();
    if (link)
        link->unlink();
    return link;
}

//===========================================================================
template <typename T, typename Tag>
void List<T, Tag>::swap(List & other) {
    swap(m_base.m_prevLink->m_nextLink, from.m_base.m_prevLink->m_nextLink);
    swap(m_base.m_nextLink->m_prevLink, from.m_base.m_nextLink->m_prevLink);
    swap(m_base, from.m_base);
}

} // namespace
