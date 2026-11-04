// Copyright Glen Knowles 2015 - 2025.
// Distributed under the Boost Software License, Version 1.0.
//
// handle.h - dim basic
#pragma once

#include "cppconf/cppconf.h"

#include <shared_mutex>
#include <span>
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
*   HandleMapBase - internal only
*
***/

struct HandleMapNode {
    HandleContent * value;
    int next;
};

template <typename H, typename T, typename S>
class HandleMapBase : public S {
public:
    class Iterator {
        HandleMapNode * node{};
        HandleMapNode * base{};
        HandleMapNode * end{};

    public:
        Iterator() {}
        Iterator(HandleMapNode * base, HandleMapNode * end);

        bool operator==(const Iterator & right) const;
        std::pair<H, T *> operator*();
        Iterator & operator++();
    };

public:
    explicit operator bool() const { return !empty(); }
    bool empty() const;
    T * find(H handle);

    void clear();
    H insert(T * value);
    void erase(H handle);

    // Releasing an empty handle is ignored and returns nullptr
    T * release(H handle);

    Iterator begin();
    Iterator end();

protected:
    HandleMapBase();
};

//===========================================================================
template<typename H, typename T, typename S>
HandleMapBase<H, T, S>::HandleMapBase() {
    static_assert(std::derived_from<H, HandleBase>);
    static_assert(std::derived_from<T, HandleContent>);
    static_assert(sizeof (HandleBase) == sizeof (H));
}

//===========================================================================
template<typename H, typename T, typename S>
bool HandleMapBase<H, T, S>::empty() const {
    return S::doEmpty();
}

//===========================================================================
template<typename H, typename T, typename S>
T * HandleMapBase<H, T, S>::find(H handle) {
    return static_cast<T *>(S::doFind(handle));
}

//===========================================================================
template<typename H, typename T, typename S>
void HandleMapBase<H, T, S>::clear() {
    for (auto && ht : *this)
        erase(ht);
}

//===========================================================================
template<typename H, typename T, typename S>
H HandleMapBase<H, T, S>::insert(T * value) {
    return S::doInsert(value).as<H>();
}

//===========================================================================
template<typename H, typename T, typename S>
void HandleMapBase<H, T, S>::erase(H handle) {
    delete release(handle);
}

//===========================================================================
template<typename H, typename T, typename S>
T * HandleMapBase<H, T, S>::release(H handle) {
    return static_cast<T *>(S::doRelease(handle));
}

//===========================================================================
template<typename H, typename T, typename S>
inline auto HandleMapBase<H, T, S>::begin() -> Iterator {
    std::span<HandleMapNode> s = S::doSpan();
    auto data = s.data();
    return Iterator(data, data + s.size());
}

//===========================================================================
template<typename H, typename T, typename S>
inline auto HandleMapBase<H, T, S>::end() -> Iterator {
    return Iterator{};
}


/****************************************************************************
*
*   HandleMapBase::Iterator
*
***/

//===========================================================================
template<typename H, typename T, typename S>
HandleMapBase<H, T, S>::Iterator::Iterator(
    HandleMapNode * base,
    HandleMapNode * end
)
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
template<typename H, typename T, typename S>
bool HandleMapBase<H, T, S>::Iterator::operator==(const Iterator & right) const {
    return node == right.node;
}

//===========================================================================
template<typename H, typename T, typename S>
std::pair<H, T *> HandleMapBase<H, T, S>::Iterator::operator*() {
    H handle;
    handle.pos = int(node - base);
    return std::make_pair(handle, static_cast<T *>(node->value));
}

//===========================================================================
template<typename H, typename T, typename S>
auto HandleMapBase<H, T, S>::Iterator::operator++() -> Iterator& {
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
*   HandleStore
*
***/

class HandleStore {
public:
    HandleStore();
    ~HandleStore();

    bool doEmpty() const;
    std::span<HandleMapNode> doSpan();
    HandleContent * doFind(HandleBase handle);
    HandleBase doInsert(HandleContent * value);
    HandleContent * doRelease(HandleBase handle);

private:
    std::vector<HandleMapNode> m_values;
    int m_numUsed{0};
    int m_firstFree{0};
};


/****************************************************************************
*
*   HandleStoreAsync
*
***/

class HandleStoreAsync {
protected:
    bool doEmpty() const;
    std::span<HandleMapNode> doSpan();
    HandleContent * doFind(HandleBase handle);
    HandleBase doInsert(HandleContent * value);
    HandleContent * doRelease(HandleBase handle);

private:
    HandleStore s;
    mutable std::shared_mutex m_mut;
};


/****************************************************************************
*
*   HandleMap
*
*   Expected usage:
*   struct WidgetHandle : HandleBase {};
*   class WidgetClass : public HandleContent { ... }
*   HandleMap<WidgetHandle, WidgetClass> widgets;
*
***/

template <typename H, typename T>
class HandleMap : public HandleMapBase<H, T, HandleStore>
{};

} // namespace
