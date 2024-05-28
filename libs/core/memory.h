// Copyright Glen Knowles 2024.
// Distributed under the Boost Software License, Version 1.0.
//
// memory.h - dim core
#pragma once

#include "cppconf/cppconf.h"

#include <source_location>
#include <type_traits>


/****************************************************************************
*
*   Standard allocators
*
***/

void * operator new(size_t count);
void * operator new[](size_t count);


/****************************************************************************
*
*   Allocators with source_location
*
***/

#define NEW(T) new(std::source_location::current()) T

void * operator new(size_t count, const std::source_location & loc) noexcept;
void * operator new[](size_t count, const std::source_location & loc) noexcept;
void operator delete(void * ptr, const std::source_location & loc) noexcept;


#if 0
//===========================================================================
template<
    typename T,
    typename ...Args
>
requires (!std::is_array_v<T>)
T * newObj(
    Args &&... args,
    std::source_location loc = std::source_location::current()
) {
    return new(_NORMAL_BLOCK, loc.file_name(), loc.line())
        T(std::forward<Args>(args)...);
}

//===========================================================================
template<typename T>
requires (std::is_array_v<T> && std::extent_v<T> == 0)
auto newObj(
    size_t count,
    std::source_location loc = std::source_location::current()
) {
    using Elem = std::remove_extent_t<T>;
    return new(_NORMAL_BLOCK, loc.file_name(), loc.line())
        Elem[count];
}
#endif
