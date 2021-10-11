// Copyright Glen Knowles 2019 - 2021.
// Distributed under the Boost Software License, Version 1.0.
//
// pageheap.h - dim core
#pragma once

#include "cppconf/cppconf.h"

#include "uset.h"

#include <cassert>
#include <cstdint>
#include <memory>
#include <vector>

namespace Dim {


/****************************************************************************
*
*   IPageHeap
*
***/

class IPageHeap {
public:
    virtual ~IPageHeap() = default;

    virtual size_t create() = 0;
    virtual void destroy(size_t pgno) = 0;
    virtual void setRoot(size_t pgno) = 0;

    virtual size_t root() const = 0;
    virtual size_t pageSize() const = 0;

    virtual bool empty() const = 0;

    virtual uint8_t * ptr(size_t pgno) = 0;
    virtual const uint8_t * ptr(size_t pgno) const = 0;
};


/****************************************************************************
*
*   PageHeap
*
***/

template <int N>
class PageHeap : public IPageHeap {
public:
    void clear();

    size_t pageCount() const;

    size_t create() override;
    void destroy(size_t pgno) override;
    void setRoot(size_t pgno) override;

    size_t root() const override;
    size_t pageSize() const override;
    bool empty() const override;

    uint8_t * ptr(size_t pgno) override;
    const uint8_t * ptr(size_t pgno) const override;

private:
    size_t m_root = 0;
    UnsignedSet m_freePages;
    std::vector<std::unique_ptr<uint8_t[]>> m_pages;
};

//===========================================================================
template <int N>
inline void PageHeap<N>::clear() {
    m_pages.clear();
}

//===========================================================================
template <int N>
inline size_t PageHeap<N>::pageCount() const {
    return m_pages.size();
}

//===========================================================================
template <int N>
inline size_t PageHeap<N>::create() {
    if (m_freePages) {
        return m_freePages.pop_front();
    } else {
        m_pages.push_back(std::make_unique<uint8_t[]>(pageSize()));
        return m_pages.size() - 1;
    }
}

//===========================================================================
template <int N>
inline void PageHeap<N>::destroy(size_t pgno) {
    assert(pgno < m_pages.size());
    if (!m_freePages.insert((unsigned) pgno))
        assert(!"page already free");
}

//===========================================================================
template <int N>
inline void PageHeap<N>::setRoot(size_t pgno) {
    assert(pgno < pageCount());
    m_root = pgno;
}

//===========================================================================
template <int N>
inline size_t PageHeap<N>::root() const {
    return m_root;
}

//===========================================================================
template <int N>
inline size_t PageHeap<N>::pageSize() const {
    return N;
}

//===========================================================================
template <int N>
inline bool PageHeap<N>::empty() const {
    return m_pages.empty();
}

//===========================================================================
template <int N>
inline uint8_t * PageHeap<N>::ptr(size_t pgno) {
    return pgno >= m_pages.size() ? nullptr : m_pages[pgno].get();
}

//===========================================================================
template <int N>
inline const uint8_t * PageHeap<N>::ptr(size_t pgno) const {
    return const_cast<PageHeap *>(this)->ptr(pgno);
}

} // namespace
