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
    void reset();
    bool linked() const;

private:
    template <typename T, typename Tag> friend class List;

    void construct();
    void detach();
    void linkBefore(ListBaseLink * next);
    void linkAfter(ListBaseLink * prev);

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
void ListBaseLink<T, Tag>::reset() {
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

//===========================================================================
template<typename T, typename Tag>
void ListBaseLink<T, Tag>::linkBefore(ListBaseLink * next) {
    detach();
    m_prevLink = next->m_prevLink;
    m_nextLink = next;

    m_prevLink->m_nextLink = this;
    next->m_prevLink = this;
}

//===========================================================================
template<typename T, typename Tag>
void ListBaseLink<T, Tag>::linkAfter(ListBaseLink * prev) {
    detach();
    m_prevLink = prev;
    m_nextLink = prev->m_nextLink;

    prev->m_nextLink = this;
    m_nextLink->m_prevLink = this;
}


/****************************************************************************
*
*   List
*
***/

template <typename T, typename Tag = LinkDefault>
class List {
public:
    class iterator;

public:
    List() {}
    List(List && from);
    ~List();

    List & operator=(List && from);
    bool operator==(const List & right) const;

    bool empty() const;
    size_t size() const;
    void clear();
    iterator begin();
    iterator end();
    T * front();
    const T * front() const;
    T * back();
    const T * back() const;
    T * next(T * node);
    const T * next(const T * node) const;
    T * prev(T * node);
    const T * prev(const T * node) const;
    void insert(const T * pos, T * value);
    void insert(const T * pos, T * first, T * last);
    void insert(const T * pos, List && other);
    void insertAfter(const T * pos, T * value);
    void insertAfter(const T * pos, T * first, T * last);
    void insertAfter(const T * pos, List && other);
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
    ListBaseLink<T, Tag> m_base;
};

template <typename T, typename Tag>
class List<T, Tag>::iterator {
    List * m_container{nullptr};
    T * m_current{nullptr};
public:
    iterator() {}
    iterator(List * container, T * node) 
        : m_container{container}
        , m_current{node} {}
    bool operator!=(const iterator & right) {
        return m_current != right.m_current;
    }
    iterator & operator++() {
        m_current = m_container->next(m_current);
        return *this;
    }
    T & operator*() {
        assert(m_current);
        return *m_current;
    }
    T * operator->() { return m_current; }
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
List<T, Tag> & List<T, Tag>::operator=(List && from) {
    swap(from);
    from.unlinkAll();
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
void List<T, Tag>::insert(const T * pos, T * value) {
    static_cast<ListBaseLink<T, Tag> *>(value)->linkBefore(pos);
}

//===========================================================================
template <typename T, typename Tag>
void List<T, Tag>::insert(const T * pos, T * first, T * last);

//===========================================================================
template <typename T, typename Tag>
void List<T, Tag>::insert(const T * pos, List && other);

//===========================================================================
template <typename T, typename Tag>
void List<T, Tag>::insertAfter(const T * pos, T * value) {
    static_cast<ListBaseLink<T, Tag> *>(value)->linkAfter(pos);
}

//===========================================================================
template <typename T, typename Tag>
void List<T, Tag>::insertAfter(const T * pos, T * first, T * last);

//===========================================================================
template <typename T, typename Tag>
void List<T, Tag>::insertAfter(const T * pos, List && other);

//===========================================================================
template <typename T, typename Tag>
void List<T, Tag>::unlinkAll() {
    while (auto ptr = front())
        unlink(ptr);
}

//===========================================================================
template <typename T, typename Tag>
void List<T, Tag>::unlink(T * value) {
    static_cast<ListBaseLink<T, Tag> *>(value)->reset();
}

//===========================================================================
template <typename T, typename Tag>
void List<T, Tag>::pushBack(T * value) {
    static_cast<ListBaseLink<T, Tag> *>(value)->linkBefore(&m_base);
}

//===========================================================================
template <typename T, typename Tag>
void List<T, Tag>::pushBack(List && other);

//===========================================================================
template <typename T, typename Tag>
T * List<T, Tag>::popBack() {
    auto link = back();
    if (link)
        link->reset();
    return link;
}

//===========================================================================
template <typename T, typename Tag>
void List<T, Tag>::pushFront(T * value) {
    static_cast<ListBaseLink<T, Tag> *>(value)->linkAfter(&m_base);
}

//===========================================================================
template <typename T, typename Tag>
void List<T, Tag>::pushFront(List && other);

//===========================================================================
template <typename T, typename Tag>
T * List<T, Tag>::popFront() {
    auto link = front();
    if (link)
        link->reset();
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
