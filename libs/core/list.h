// Copyright Glen Knowles 2016 - 2018.
// Distributed under the Boost Software License, Version 1.0.
//
// list.h - dim core
//
// Intrusive linked list
//  - Member objects inherit from ListBaseLink one or more times (allowing
//    a single object to be in multiple lists).
//  - Members remove themselves from the list on destruction
//  - List unlinks (does NOT delete) members on destruction
//  - size() is O(N)
//
// Example usage:
// struct MyObj : ListBaseLink<> {};
// List<MyObj> list;
// list.link(new MyObj);
// list.clear(); // delete all the member
//
// Example of an object in multiple lists:
// struct Fruit; // tags only need to be a forward reference, the definition
// struct Color; //   is not required.
// struct MyObj : ListBaseLink<Fruit>, ListBaseLink<Color> {};
// List<MyObj, Fruit> fruits;
// List<MyObj, Color> colors;
// auto apple = new MyObj;
// fruits.link(apple);
// auto red = new MyObj;
// colors.link(red);
// auto orange = new MyObj;
// fruits.link(orange);
// colors.link(orange);
// fruits.clear(); // deletes apple & orange, unlinking orange from colors
// colors.clear(); // deletes red

#pragma once

#include "cppconf/cppconf.h"

#include "core/types.h"

#include <algorithm>

namespace Dim {


// forward declarations
struct DefaultTag;


/****************************************************************************
*
*   ListBaseLink
*
***/

template <typename Tag = DefaultTag>
class ListBaseLink : public NoCopy {
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
template<typename Tag>
ListBaseLink<Tag>::ListBaseLink () {
    construct();
}

//===========================================================================
template<typename Tag>
ListBaseLink<Tag>::~ListBaseLink () {
    detach();
}

//===========================================================================
template<typename Tag>
void ListBaseLink<Tag>::unlink() {
    assert(linked());
    detach();
    construct();
}

//===========================================================================
template<typename Tag>
bool ListBaseLink<Tag>::linked() const {
    return m_prevLink != this;
}

//===========================================================================
template<typename Tag>
void ListBaseLink<Tag>::construct() {
    m_prevLink = m_nextLink = this;
}

//===========================================================================
template<typename Tag>
void ListBaseLink<Tag>::detach() {
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
    bool operator==(const ListIterator & right);
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
bool ListIterator<List, T>::operator==(const ListIterator & right) {
    return m_current == right.m_current;
}

//===========================================================================
template <typename List, typename T>
bool ListIterator<List, T>::operator!=(const ListIterator & right) {
    return !(*this == right);
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

template <typename T, typename Tag = DefaultTag>
class List : public NoCopy {
public:
    using iterator = ListIterator<List, T>;
    using const_iterator = ListIterator<const List, const T>;
    using link_type = ListBaseLink<Tag>;

public:
    List();
    List(List && from);
    ~List();
    explicit operator bool() const { return !empty(); }

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
    T * next(const T * node);
    const T * next(const T * node) const;
    T * prev(const T * node);
    const T * prev(const T * node) const;
    void link(T * value);
    void link(List && other);
    void linkFront(T * value);
    void linkFront(List && other);
    void link(T * pos, T * value);
    void link(T * pos, T * first, T * last);
    void link(T * pos, List && other);
    void linkAfter(T * pos, T * value);
    void linkAfter(T * pos, T * first, T * last);
    void linkAfter(T * pos, List && other);
    void unlink(T * value);
    bool linked(const T * value) const;
    T * unlinkBack();
    T * unlinkFront();
    void unlinkAll();
    void swap(List & other);

private:
    link_type * cast(T * node) const;
    const link_type * cast(const T * node) const;
    T * cast(link_type * link) const;
    const T * cast(const link_type * link) const;

    void linkBase(link_type * pos, link_type * first, link_type * last);
    void linkBaseAfter(link_type * pos, link_type * first, link_type * last);

    link_type m_base;
};

//===========================================================================
template <typename T, typename Tag>
List<T, Tag>::List() {
    static_assert(
        std::is_base_of_v<link_type, T>,
        "List member type must be derived from ListBaseLink<Tag>"
    );
}

//===========================================================================
template <typename T, typename Tag>
List<T, Tag>::List(List && from) {
    static_assert(
        std::is_base_of_v<link_type, T>,
        "List member type must be derived from ListBaseLink<Tag>"
    );
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
    return m_base.linked() ? cast(m_base.m_nextLink) : nullptr;
}

//===========================================================================
template <typename T, typename Tag>
const T * List<T, Tag>::front() const {
    return const_cast<List *>(this)->front();
}

//===========================================================================
template <typename T, typename Tag>
T * List<T, Tag>::back() {
    return m_base.linked() ? cast(m_base.m_prevLink) : nullptr;
}

//===========================================================================
template <typename T, typename Tag>
const T * List<T, Tag>::back() const {
    return const_cast<List *>(this)->back();
}

//===========================================================================
template <typename T, typename Tag>
T * List<T, Tag>::next(const T * node) {
    auto link = cast(node)->m_nextLink;
    assert(link->linked());
    return link != &m_base ? cast(link) : nullptr;
}

//===========================================================================
template <typename T, typename Tag>
const T * List<T, Tag>::next(const T * node) const {
    return const_cast<List *>(this)->next(node);
}

//===========================================================================
template <typename T, typename Tag>
T * List<T, Tag>::prev(const T * node) {
    auto link = cast(node)->m_prevLink;
    assert(link->linked());
    return link != &m_base ? cast(link) : nullptr;
}

//===========================================================================
template <typename T, typename Tag>
const T * List<T, Tag>::prev(const T * node) const {
    return const_cast<List *>(this)->prev(node);
}

//===========================================================================
template <typename T, typename Tag>
void List<T, Tag>::link(T * pos, T * value) {
    auto first = cast(value);
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
void List<T, Tag>::linkAfter(T * pos, T * value) {
    auto first = cast(value);
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
void List<T, Tag>::link(T * value) {
    auto first = cast(value);
    linkBase(&m_base, first, first->m_nextLink);
}

//===========================================================================
template <typename T, typename Tag>
void List<T, Tag>::link(List && other) {
    linkBase(&m_base, other.front(), other.back());
}

//===========================================================================
template <typename T, typename Tag>
void List<T, Tag>::linkFront(T * value) {
    auto first = cast(value);
    linkBaseAfter(&m_base, first, first->m_nextLink);
}

//===========================================================================
template <typename T, typename Tag>
void List<T, Tag>::linkFront(List && other) {
    linkBaseAfter(&m_base, other.front(), other.back());
}

//===========================================================================
template <typename T, typename Tag>
void List<T, Tag>::unlink(T * value) {
    cast(value)->unlink();
}

//===========================================================================
template <typename T, typename Tag>
bool List<T, Tag>::linked(const T * value) const {
    return cast(value)->linked();
}

//===========================================================================
template <typename T, typename Tag>
T * List<T, Tag>::unlinkBack() {
    auto link = back();
    if (link)
        link->unlink();
    return link;
}

//===========================================================================
template <typename T, typename Tag>
T * List<T, Tag>::unlinkFront() {
    auto link = front();
    if (link)
        link->unlink();
    return link;
}

//===========================================================================
template <typename T, typename Tag>
void List<T, Tag>::unlinkAll() {
    auto ptr = m_base.m_nextLink;
    m_base.construct();
    while (ptr != &m_base) {
        auto next = ptr->m_nextLink;
        ptr->construct();
        ptr = next;
    }
}

//===========================================================================
template <typename T, typename Tag>
void List<T, Tag>::swap(List & other) {
    swap(m_base.m_prevLink->m_nextLink, other.m_base.m_prevLink->m_nextLink);
    swap(m_base.m_nextLink->m_prevLink, other.m_base.m_nextLink->m_prevLink);
    swap(m_base, other.m_base);
}

//===========================================================================
template <typename T, typename Tag>
auto List<T, Tag>::cast(T * node) const -> link_type * {
    return node;
}

//===========================================================================
template <typename T, typename Tag>
auto List<T, Tag>::cast(const T * node) const -> const link_type * {
    return node;
}

//===========================================================================
template <typename T, typename Tag>
T * List<T, Tag>::cast(link_type * link) const {
    return static_cast<T *>(link);
}

//===========================================================================
template <typename T, typename Tag>
const T * List<T, Tag>::cast(const link_type * link) const {
    return static_cast<const T *>(link);
}

//===========================================================================
template <typename T, typename Tag>
void List<T, Tag>::linkBase(
    link_type * next,
    link_type * first,
    link_type * last
) {
    auto inclusiveLast = last->m_prevLink;

    // detach [first, last)
    last->m_prevLink = first->m_prevLink;
    first->m_prevLink->m_nextLink = last;

    // update [first, inclusiveLast] pointers to list
    first->m_prevLink = next->m_prevLink;
    inclusiveLast->m_nextLink = next;

    // update list's pointers to [first, inclusiveLast] segment
    first->m_prevLink->m_nextLink = first;
    next->m_prevLink = inclusiveLast;
}

//===========================================================================
template <typename T, typename Tag>
void List<T, Tag>::linkBaseAfter(
    link_type * prev,
    link_type * first,
    link_type * last
) {
    linkBase(prev->m_nextLink, first, last);
}

} // namespace
