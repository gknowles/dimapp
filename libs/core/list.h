// Copyright Glen Knowles 2016 - 2022.
// Distributed under the Boost Software License, Version 1.0.
//
// list.h - dim core
//
// Intrusive linked list
//  - Member objects inherit from ListLink one or more times (allowing
//    a single object to be in multiple lists).
//  - Members remove themselves from the list on destruction
//  - List unlinks (does NOT delete) members on destruction
//  - size() is O(N)
//
// Example usage:
// struct MyObj : ListLink<> {};
// List<MyObj> list;
// list.link(new MyObj);
// list.clear(); // deletes all members
//
// Example of an object in multiple lists:
// struct Fruit; // Tags only need forward references, the definitions
// struct Color; //   aren't required.
// struct MyObj : ListLink<Fruit>, ListLink<Color> {};
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
*   ListLink
*
***/

template <typename Tag = DefaultTag>
class ListLink : public NoCopy {
public:
    ListLink();
    ~ListLink();

protected:
    void unlink();
    bool linked() const;

private:
    template <typename T, typename Tag> friend class List;

    void construct();
    void detach();

    ListLink * m_prevLink;
    ListLink * m_nextLink;
};

//===========================================================================
template<typename Tag>
ListLink<Tag>::ListLink () {
    construct();
}

//===========================================================================
template<typename Tag>
ListLink<Tag>::~ListLink () {
    detach();
}

//===========================================================================
template<typename Tag>
void ListLink<Tag>::unlink() {
    assert(linked());
    detach();
    construct();
}

//===========================================================================
template<typename Tag>
bool ListLink<Tag>::linked() const {
    return m_prevLink != this;
}

//===========================================================================
template<typename Tag>
void ListLink<Tag>::construct() {
    m_prevLink = m_nextLink = this;
}

//===========================================================================
template<typename Tag>
void ListLink<Tag>::detach() {
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
public:
    using iterator_category = std::bidirectional_iterator_tag;
    using value_type = List::value_type;
    using difference_type = List::difference_type;
    using pointer = const value_type*;
    using reference = value_type;

public:
    ListIterator() {}
    ListIterator(List * container, T * node);
    bool operator==(const ListIterator & right) const;
    ListIterator & operator++();
    T & operator*();
    T * operator->();

private:
    List * m_container{};
    T * m_current{};
};

//===========================================================================
template <typename List, typename T>
ListIterator<List, T>::ListIterator(List * container, T * node)
    : m_container{container}
    , m_current{node}
{}

//===========================================================================
template <typename List, typename T>
bool ListIterator<List, T>::operator==(const ListIterator & right) const {
    return m_current == right.m_current;
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
    using value_type = T;
    using difference_type = ptrdiff_t;
    using size_type = size_t;
    using iterator = ListIterator<List, T>;
    using const_iterator = ListIterator<const List, const T>;
    using link_type = ListLink<Tag>;

public:
    List();
    List(List && from) noexcept;
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
    const_iterator cbegin() const { return begin(); }
    const_iterator cend() const { return end(); }

    auto * front(this auto && self);
    auto * back(this auto && self);
    auto * next(this auto && self, const T * node);
    auto * prev(this auto && self, const T * node);
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
        std::derived_from<T, link_type>,
        "List member type must be derived from ListLink<Tag>"
    );
}

//===========================================================================
template <typename T, typename Tag>
List<T, Tag>::List(List && from) noexcept {
    static_assert(
        std::derived_from<T, link_type>,
        "List member type must be derived from ListLink<Tag>"
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
auto * List<T, Tag>::front(this auto && self) {
    return self.m_base.linked() ? self.cast(self.m_base.m_nextLink) : nullptr;
}

//===========================================================================
template <typename T, typename Tag>
auto * List<T, Tag>::back(this auto && self) {
    return self.m_base.linked() ? self.cast(self.m_base.m_prevLink) : nullptr;
}

//===========================================================================
template <typename T, typename Tag>
auto * List<T, Tag>::next(this auto && self, const T * node) {
    auto link = self.cast(node)->m_nextLink;
    assert(link->linked());
    return link != &self.m_base ? self.cast(link) : nullptr;
}

//===========================================================================
template <typename T, typename Tag>
auto * List<T, Tag>::prev(this auto && self, const T * node) {
    auto link = self.cast(node)->m_prevLink;
    assert(link->linked());
    return link != &self.m_base ? self.cast(link) : nullptr;
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
    if (other)
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
    if (other)
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
    if (other)
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
    if (other)
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
