// Copyright Glen Knowles 2024.
// Distributed under the Boost Software License, Version 1.0.
//
// memory.h - dim core
#pragma once

#include "cppconf/cppconf.h"

#include <memory>
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

[[nodiscard]] void * operator new(
    size_t count,
    const std::source_location & loc
) noexcept;
[[nodiscard]] void * operator new[](
    size_t count,
    const std::source_location & loc
) noexcept;
void operator delete(void * ptr, const std::source_location & loc) noexcept;


namespace Dim {

/****************************************************************************
*
*   Allocators for unique pointer with source_location
*
***/

#define NEW_UNIQUE(T, ...) Dim::newUnique<T>( \
    std::source_location::current(), __VA_ARGS__)

//===========================================================================
template<typename T, typename... Args>
requires (!std::is_array_v<T>)
[[nodiscard]] std::unique_ptr<T> newUnique(
    const std::source_location & loc,
    Args&&... args
) {
    return std::unique_ptr<T>(new(loc) T(std::forward<Args>(args)...));
}

//===========================================================================
template<typename T>
requires (std::is_array_v<T> && std::extent_v<T> == 0)
[[nodiscard]] std::unique_ptr<T> newUnique(
    const std::source_location & loc,
    size_t count
) {
    using Elem = std::remove_extent_t<T>;
    return std::unique_ptr<T>(new(loc) Elem[count]);
}


/****************************************************************************
*
*   Allocators for shared pointer with source_location
*
***/

#define NEW_SHARED(T, ...) Dim::newShared<T>( \
    std::source_location::current(), __VA_ARGS__)

//===========================================================================
template<typename T, typename... Args>
requires (!std::is_array_v<T>)
[[nodiscard]] std::shared_ptr<T> newShared(
    const std::source_location & loc,
    Args&&... args
) {
    return std::shared_ptr<T>(new(loc) T(std::forward<Args>(args)...));
}

//===========================================================================
template<typename T>
requires (std::is_array_v<T> && std::extent_v<T> == 0)
[[nodiscard]] std::shared_ptr<T> newShared(
    const std::source_location & loc,
    size_t count
) {
    using Elem = std::remove_extent_t<T>;
    return std::shared_ptr<T>(new(loc) Elem[count]);
}


} // namespace
