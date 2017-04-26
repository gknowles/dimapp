// Copyright Glen Knowles 2015 - 2017.
// Distributed under the Boost Software License, Version 1.0.
//
// types.h - dim core
#pragma once

#include "cppconf/cppconf.h"

#include <cassert>
#include <cstdint>
#include <type_traits>

namespace Dim {


/****************************************************************************
*
*   Run modes
*
***/

enum RunMode {
    kRunStopped,
    kRunStarting,
    kRunRunning,
    kRunStopping,
};


/****************************************************************************
*
*   ForwardListIterator
*
***/

template <typename T> class ForwardListIterator {
protected:
    T * m_current{nullptr};

public:
    ForwardListIterator(T * node)
        : m_current(node) {}
    bool operator!=(const ForwardListIterator & right) {
        return m_current != right.m_current;
    }
    ForwardListIterator & operator++() {
        m_current = m_current->m_next;
        return *this;
    }
    T & operator*() {
        assert(m_current);
        return *m_current;
    }
    T * operator->() { return m_current; }
};


/****************************************************************************
*
*   Allow bitwise NOT, AND, XOR and OR operations on unsigned enum values
*
*   Set the underlying type of an enum to one of the unsigned types to allow 
*   bitwise operations between values of that enum.
*
*   enum EnumA : unsigned {
*       A1, A2, A3
*   };
*
*   Example:
*   enum EnumA : unsigned { A1, A2 };
*   enum EnumB : unsigned { B1, B2 };
*
*   void foo(EnumA flags);
*   
*   foo(A1 | A2); // or of two EnumA's is an EnumA
*   foo(A1);
*   foo(B1); // error - wrong enum type
*   foo(A1 | B1); // error - no conversion from int to EnumA
*
*   To override this behavior the type traits of individual enums can be 
*   specialized one way or the other:
*       template<> struct is_enum_flags<enum A : int> : std::true_type {};
*       template<> struct is_enum_flags<enum B : unsigned> : std::false_type {};
*
***/

template<typename T, bool = std::is_enum_v<T>> 
struct is_enum_flags : std::false_type {};

template<typename T> 
struct is_enum_flags<T, true> 
    : std::bool_constant< std::is_unsigned_v<std::underlying_type_t<T>> >
{};

template<typename T> constexpr bool is_enum_flags_v = is_enum_flags<T>::value;

//===========================================================================
template<typename T> constexpr 
std::enable_if_t<is_enum_flags_v<T>, T> operator~ (T val) {
    return T(~std::underlying_type_t<T>(val));
}

//===========================================================================
template<typename T> constexpr 
std::enable_if_t<is_enum_flags_v<T>, T> operator| (T left, T right) {
    return T(std::underlying_type_t<T>(left) | right);
}

//===========================================================================
template<typename T> constexpr 
std::enable_if_t<is_enum_flags_v<T>, T&> operator|= (T & left, T right) {
    return left = left | right;
}

//===========================================================================
template<typename T> constexpr 
std::enable_if_t<is_enum_flags_v<T>, T> operator& (T left, T right) {
    return T(std::underlying_type_t<T>(left) & right);
}

//===========================================================================
template<typename T> constexpr 
std::enable_if_t<is_enum_flags_v<T>, T&> operator&= (T & left, T right) {
    return left = left & right;
}

//===========================================================================
template<typename T> constexpr 
std::enable_if_t<is_enum_flags_v<T>, T> operator^ (T left, T right) {
    return T(std::underlying_type_t<T>(left) ^ right);
}

//===========================================================================
template<typename T> constexpr 
std::enable_if_t<is_enum_flags_v<T>, T&> operator^= (T & left, T right) {
    return left = left ^ right;
}


} // namespace
