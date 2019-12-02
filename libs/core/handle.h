// Copyright Glen Knowles 2015 - 2019.
// Distributed under the Boost Software License, Version 1.0.
//
// handle.h - dim core
#pragma once

#include "cppconf/cppconf.h"

#include <utility> // std::pair
#include <vector>

namespace Dim {


/****************************************************************************
*
*   Base classes
*   Clients inherit from these classes to make different kinds of handle
*   and content pairs.
*
*   Expected usage:
*   struct WidgetHandle : HandleBase {};
*   class Widget : public HandleContent { ... };
*
***/

struct HandleBase {
    // handles to data >0, 0 means no data, <0 is not allowed
    int pos{0};

    explicit operator bool() const { return pos; }
    bool operator==(HandleBase right) const { return pos == right.pos; }
    template <typename H> H as() const {
        H handle;
        static_cast<HandleBase &>(handle) = *this;
        return handle;
    }
};

struct HandleContent {
    virtual ~HandleContent() = default;
};


/****************************************************************************
*
*   Handle map base type - internal only
*
***/

class HandleMapBase {
public:
    template <typename H, typename T> class Iterator;
    struct Node {
        HandleContent * value;
        int next;
    };

public:
    explicit operator bool() const { return !empty(); }
    bool empty() const;

protected:
    HandleMapBase();
    ~HandleMapBase();
    HandleContent * find(HandleBase handle);

    HandleBase insert(HandleContent * value);

    // Releasing an empty handle is ignored and returns nullptr
    HandleContent * release(HandleBase handle);

    template <typename H, typename T> Iterator<H, T> begin();
    template <typename H, typename T> Iterator<H, T> end();

private:
    std::vector<Node> m_values;
    int m_numUsed{0};
    int m_firstFree{0};
};

//===========================================================================
template <typename H, typename T>
inline auto HandleMapBase::begin() -> Iterator<H, T> {
    auto data = m_values.data();
    return Iterator<H, T>(data, data + m_values.size());
}

//===========================================================================
template <typename H, typename T>
inline auto HandleMapBase::end() -> Iterator<H, T> {
    return Iterator<H, T>{};
}

//===========================================================================
// handle map base iterator - internal only
//===========================================================================
template <typename H, typename T> class HandleMapBase::Iterator {
    HandleMapBase::Node * node{};
    HandleMapBase::Node * base{};
    HandleMapBase::Node * end{};

public:
    Iterator() {}
    Iterator(Node * base, Node * end);

    bool operator==(const Iterator & right) const;
    std::pair<H, T *> operator*();
    Iterator & operator++();
};

//===========================================================================
template<typename H, typename T>
HandleMapBase::Iterator<H, T>::Iterator(Node * base, Node * end)
    : node{base}
    , base{base}
    , end{end}
{
    for (; node != end; ++node) {
        if (node->value)
            return;
    }
    node = nullptr;
}

//===========================================================================
template<typename H, typename T>
bool HandleMapBase::Iterator<H, T>::operator==(const Iterator & right) const {
    return node == right.node;
}

//===========================================================================
template<typename H, typename T>
std::pair<H, T *> HandleMapBase::Iterator<H, T>::operator*() {
    H handle;
    handle.pos = int(node - base);
    return std::make_pair(handle, static_cast<T *>(node->value));
}

//===========================================================================
template<typename H, typename T>
auto HandleMapBase::Iterator<H, T>::operator++() -> Iterator<H, T>& {
    node += 1;
    for (; node != end; ++node) {
        if (node->value)
            return *this;
    }
    node = nullptr;
    return *this;
}


/****************************************************************************
*
*   Handle map
*   Container of handles
*
*   Expected usage:
*   struct WidgetHandle : HandleBase {};
*   class WidgetClass : public HandleContent { ... }
*   HandleMap<WidgetHandle, WidgetClass> widgets;
*
***/

template <typename H, typename T>
class HandleMap : public HandleMapBase {
public:
    HandleMap();

    T * find(H handle);

    void clear();
    H insert(T * value);
    void erase(H handle);
    T * release(H handle);

    Iterator<H, T> begin();
    Iterator<H, T> end();
};

//===========================================================================
template<typename H, typename T>
HandleMap<H, T>::HandleMap() {
    static_assert(std::is_base_of_v<HandleBase, H>);
    static_assert(std::is_base_of_v<HandleContent, T>);
    static_assert(sizeof HandleBase == sizeof H);
}

//===========================================================================
template<typename H, typename T>
T * HandleMap<H, T>::find(H handle) {
    return static_cast<T *>(HandleMapBase::find(handle));
}

//===========================================================================
template<typename H, typename T>
void HandleMap<H, T>::clear() {
    for (auto && ht : *this)
        erase(ht);
}

//===========================================================================
template<typename H, typename T>
H HandleMap<H, T>::insert(T * value) {
    return HandleMapBase::insert(value).as<H>();
}

//===========================================================================
template<typename H, typename T>
void HandleMap<H, T>::erase(H handle) {
    delete release(handle);
}

//===========================================================================
template<typename H, typename T>
T * HandleMap<H, T>::release(H handle) {
    return static_cast<T *>(HandleMapBase::release(handle));
}

//===========================================================================
template<typename H, typename T>
auto HandleMap<H, T>::begin() -> Iterator<H, T> {
    return HandleMapBase::begin<H, T>();
}

//===========================================================================
template<typename H, typename T>
auto HandleMap<H, T>::end() -> Iterator<H, T> {
    return HandleMapBase::end<H, T>();
}

} // namespace
