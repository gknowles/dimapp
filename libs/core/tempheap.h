// Copyright Glen Knowles 2015 - 2022.
// Distributed under the Boost Software License, Version 1.0.
//
// tempheap.h - dim core
#pragma once

#include "cppconf/cppconf.h"

#include <cstring>
#include <memory_resource>
#include <span>
#include <string_view>

namespace Dim {


/****************************************************************************
*
*   Temp heap interface
*
***/

class ITempHeap : public std::pmr::memory_resource {
public:
    virtual ~ITempHeap() = default;

    template <typename T, typename... Args>
        T * emplace(Args &&... args);
    template <typename T>
        T * alloc(size_t num);
    template <typename T>
        std::span<T> allocSpan(size_t num);

    char * strDup(const char src[]);
    char * strDup(std::string_view src);
    char * strDup(const char src[], size_t len);
    std::string_view viewDup(const char src[]);
    std::string_view viewDup(std::string_view src);
    std::string_view viewDup(const char src[], size_t len);

    virtual char * alloc(size_t bytes, size_t alignment) = 0;

private:
    // Inherited via std::pmr::memory_resource
    void * do_allocate(size_t bytes, size_t alignment) override;
    void do_deallocate(void * ptr, size_t bytes, size_t alignment) override;
    bool do_is_equal(
        const std::pmr::memory_resource & other
    ) const noexcept override;
};

//===========================================================================
template <typename T, typename... Args>
inline T * ITempHeap::emplace(Args &&... args) {
    char * tmp = alloc(sizeof(T), alignof(T));
    return new (tmp) T(std::forward<Args>(args)...);
}

//===========================================================================
template <typename T>
inline T * ITempHeap::alloc(size_t num) {
    T * tmp = (T *) alloc(num * sizeof(T), alignof(T));
    std::uninitialized_default_construct_n(tmp, num);
    return tmp;
}

//===========================================================================
template <typename T>
inline std::span<T> ITempHeap::allocSpan(size_t num) {
    return std::span(alloc<T>(num), num);
}

//===========================================================================
inline char * ITempHeap::strDup(const char src[]) {
    return strDup(std::string_view(src));
}

//===========================================================================
inline char * ITempHeap::strDup(std::string_view src) {
    return strDup(src.data(), src.size());
}

//===========================================================================
inline char * ITempHeap::strDup(const char src[], size_t len) {
    char * out = alloc(len + 1, alignof(char));
    std::memcpy(out, src, len);
    out[len] = 0;
    return out;
}

//===========================================================================
inline std::string_view ITempHeap::viewDup(const char src[]) {
    return viewDup(std::string_view(src));
}

//===========================================================================
inline std::string_view ITempHeap::viewDup(std::string_view src) {
    return viewDup(src.data(), src.size());
}

//===========================================================================
inline std::string_view ITempHeap::viewDup(const char src[], size_t len) {
    char * out = strDup(src, len);
    return {out, len};
}

//===========================================================================
inline void * ITempHeap::do_allocate(size_t bytes, size_t alignment) {
    return alloc(bytes, alignment);
}

//===========================================================================
inline void ITempHeap::do_deallocate(
    void * ptr,
    size_t bytes,
    size_t alignment
) {
    // Resources freed only on destruction of heap.
    return;
}

//===========================================================================
inline bool ITempHeap::do_is_equal(
    const std::pmr::memory_resource & other
) const noexcept {
    return this == &other;
}


/****************************************************************************
*
*   TempHeap
*
***/

class TempHeap : public ITempHeap {
public:
    TempHeap() = default;
    TempHeap(TempHeap && from) noexcept;
    ~TempHeap();
    TempHeap & operator=(const TempHeap & from) = delete;
    TempHeap & operator=(TempHeap && from) noexcept;

    using ITempHeap::alloc;
    void clear();
    void swap(TempHeap & from);

private:
    // ITempHeap
    char * alloc(size_t bytes, size_t alignment) override;

    void * m_buffer{};
};


} // namespace
