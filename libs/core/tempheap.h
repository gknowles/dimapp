// Copyright Glen Knowles 2015 - 2020.
// Distributed under the Boost Software License, Version 1.0.
//
// tempheap.h - dim core
#pragma once

#include "cppconf/cppconf.h"

#include <cstring>
#include <string_view>

namespace Dim {


/****************************************************************************
*
*   Temp heap interface
*
***/

class ITempHeap {
public:
    virtual ~ITempHeap() = default;

    template <typename T, typename... Args> T * emplace(Args &&... args);
    template <typename T> T * alloc(size_t num);

    char * strDup(const char src[]);
    char * strDup(std::string_view src);
    char * strDup(const char src[], size_t len);

    char * alloc(size_t bytes);
    virtual char * alloc(size_t bytes, size_t alignment) = 0;
};

//===========================================================================
template <typename T, typename... Args>
inline T * ITempHeap::emplace(Args &&... args) {
    char * tmp = alloc(sizeof T, alignof(T));
    return new (tmp) T(args...);
}

//===========================================================================
template <typename T> inline T * ITempHeap::alloc(size_t num) {
    char * tmp = alloc(num * sizeof T, alignof(T));
    return new (tmp) T[num];
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
inline char * ITempHeap::alloc(size_t bytes) {
    return alloc(bytes, alignof(char));
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

    void clear();
    void swap(TempHeap & from);

    // ITempHeap
    char * alloc(size_t bytes, size_t alignment) override;

private:
    void * m_buffer{};
};

} // namespace
