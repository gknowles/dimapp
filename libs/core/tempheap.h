// Copyright Glen Knowles 2015 - 2024.
// Distributed under the Boost Software License, Version 1.0.
//
// tempheap.h - dim core
#pragma once

#include "cppconf/cppconf.h"

#include <memory_resource>
#include <span>
#include <string_view>

namespace Dim {


/****************************************************************************
*
*   Heap interface
*
***/

class IHeap : public std::pmr::memory_resource {
public:
    virtual ~IHeap() = default;

    template <typename T, typename... Args> T * emplace(Args &&... args);
    template <typename T> T * alloc(size_t num);
    template <typename T> std::span<T> allocSpan(size_t num);

    char * alloc(size_t bytes, size_t alignment);
    char * strDup(const char src[]);
    char * strDup(std::string_view src);
    char * strDup(const char src[], size_t len);
    std::string_view viewDup(const char src[]);
    std::string_view viewDup(std::string_view src);
    std::string_view viewDup(const char src[], size_t len);

private:
    // Inherited via std::pmr::memory_resource -- private members of base!
    void * do_allocate(size_t bytes, size_t alignment) override = 0;
    void do_deallocate(
        void * ptr,
        size_t bytes, size_t alignment
    ) override = 0;
    bool do_is_equal(
        const std::pmr::memory_resource & other
    ) const noexcept override;
};

//===========================================================================
template <typename T, typename... Args>
inline T * IHeap::emplace(Args &&... args) {
    char * tmp = alloc(sizeof(T), alignof(T));
    return new(tmp) T(std::forward<Args>(args)...);
}

//===========================================================================
template <typename T>
inline T * IHeap::alloc(size_t num) {
    T * tmp = (T *) alloc(num * sizeof(T), alignof(T));
    std::uninitialized_default_construct_n(tmp, num);
    return tmp;
}

//===========================================================================
template <typename T>
inline std::span<T> IHeap::allocSpan(size_t num) {
    return std::span(alloc<T>(num), num);
}

//===========================================================================
inline char * IHeap::alloc(size_t bytes, size_t alignment) {
    return (char *) allocate(bytes, alignment);
}

//===========================================================================
inline char * IHeap::strDup(const char src[]) {
    return strDup(std::string_view(src));
}

//===========================================================================
inline char * IHeap::strDup(std::string_view src) {
    return strDup(src.data(), src.size());
}

//===========================================================================
inline char * IHeap::strDup(const char src[], size_t len) {
    auto out = alloc<char>(len + 1);
    std::memcpy(out, src, len);
    out[len] = 0;
    return out;
}

//===========================================================================
inline std::string_view IHeap::viewDup(const char src[]) {
    return viewDup(std::string_view(src));
}

//===========================================================================
inline std::string_view IHeap::viewDup(std::string_view src) {
    return viewDup(src.data(), src.size());
}

//===========================================================================
inline std::string_view IHeap::viewDup(const char src[], size_t len) {
    auto out = alloc<char>(len);
    std::memcpy(out, src, len);
    return {out, len};
}

//===========================================================================
inline bool IHeap::do_is_equal(
    const std::pmr::memory_resource & other
) const noexcept {
    return this == &other;
}


/****************************************************************************
*
*   TempHeap
*
***/

class TempHeap : public IHeap {
public:
    TempHeap() = default;
    TempHeap(TempHeap && from) noexcept;
    ~TempHeap();
    TempHeap & operator=(const TempHeap & from) = delete;
    TempHeap & operator=(TempHeap && from) noexcept;

    void clear();
    void swap(TempHeap & from);

private:
    // Inherited via std::pmr::memory_resource -- private members of base!
    void * do_allocate(size_t bytes, size_t alignment) override;
    void do_deallocate(void * ptr, size_t bytes, size_t alignment) override;

    void * m_buffer{};
};


} // namespace
