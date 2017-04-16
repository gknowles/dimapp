// Copyright Glen Knowles 2015 - 2017.
// Distributed under the Boost Software License, Version 1.0.
//
// types.h - dim core
#pragma once

#include "cppconf/cppconf.h"

#include <cassert>
#include <chrono>
#include <cstdint>
#include <ctime> // time_t
#include <ratio>
#include <type_traits>

// using namespace std::literals;
namespace Dim {


/****************************************************************************
*
*   Clock
*
***/

struct Clock {
    typedef int64_t rep;
    typedef std::ratio_multiply<std::ratio<100, 1>, std::nano> period;
    typedef std::chrono::duration<rep, period> duration;
    typedef std::chrono::time_point<Clock> time_point;
    static const bool is_monotonic = false;
    static const bool is_steady = false;

    static time_point now() noexcept;

    // C conversions
    static time_t to_time_t(const time_point & time) noexcept;
    static time_point from_time_t(time_t tm) noexcept;
};

typedef Clock::duration Duration;
typedef Clock::time_point TimePoint;


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
*       template<> struct is_flags<enum A : int> : std::true_type {};
*       template<> struct is_flags<enum B : unsigned> : std::false_type {};
*
***/

template<typename T, bool = std::is_enum_v<T>> 
struct is_flags : std::false_type {};

template<typename T> 
struct is_flags<T, true> 
    : std::bool_constant< std::is_unsigned_v<std::underlying_type_t<T>> >
{};

template<typename T> constexpr bool is_flags_v = is_flags<T>::value;

//===========================================================================
template<typename T> constexpr 
std::enable_if_t<is_flags_v<T>, T> operator~ (T val) {
    return T(~std::underlying_type_t<T>(val));
}

//===========================================================================
template<typename T> constexpr 
std::enable_if_t<is_flags_v<T>, T> operator| (T left, T right) {
    return T(std::underlying_type_t<T>(left) | right);
}

//===========================================================================
template<typename T> constexpr 
std::enable_if_t<is_flags_v<T>, T> operator|= (T left, T right) {
    return left = left | right;
}

//===========================================================================
template<typename T> constexpr 
std::enable_if_t<is_flags_v<T>, T> operator& (T left, T right) {
    return T(std::underlying_type_t<T>(left) & right);
}

//===========================================================================
template<typename T> constexpr 
std::enable_if_t<is_flags_v<T>, T> operator&= (T left, T right) {
    return left = left & right;
}

//===========================================================================
template<typename T> constexpr 
std::enable_if_t<is_flags_v<T>, T> operator^ (T left, T right) {
    return T(std::underlying_type_t<T>(left) ^ right);
}

//===========================================================================
template<typename T> constexpr 
std::enable_if_t<is_flags_v<T>, T> operator^= (T left, T right) {
    return left = left ^ right;
}


} // namespace
