// Copyright Glen Knowles 2015 - 2021.
// Distributed under the Boost Software License, Version 1.0.
//
// types.h - dim core
#pragma once

#include "cppconf/cppconf.h"

#include <cassert>
#include <compare>
#include <cstdint>
#include <functional>
#include <string>
#include <string_view>
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

const char * toString(RunMode mode, const char def[] = "");
RunMode fromString(std::string_view src, RunMode def);


/****************************************************************************
*
*   VersionInfo
*
***/

struct VersionInfo {
    unsigned major;
    unsigned minor;
    unsigned patch;
    unsigned build;

    bool operator==(const VersionInfo &) const = default;
    std::strong_ordering operator<=>(const VersionInfo &) const = default;
};

std::string toString(const VersionInfo & ver);


/****************************************************************************
*
*   NoCopy base class
*
*   Prevents any class derived from NoCopy from being copy constructed or
*   copy assigned.
*
***/

class NoCopy {
protected:
    NoCopy() = default;
    ~NoCopy() = default;

    NoCopy(const NoCopy &) = delete;
    NoCopy & operator=(const NoCopy &) = delete;
};


/****************************************************************************
*
*   Finally
*
***/

struct Finally : public std::function<void()> {
    using function::function;
    ~Finally();
};

//===========================================================================
inline Finally::~Finally() {
    if (*this)
        (*this)();
}


/****************************************************************************
*
*   ForwardListIterator
*
*   The type being iterated over by the iterator must:
*       - have a m_next member of type T* that points to the next object
*       - have the m_next member of the last object set to nullptr
*
***/

template <typename T>
class ForwardListIterator {
public:
    ForwardListIterator(T * node);
    bool operator==(const ForwardListIterator & right) const = default;
    ForwardListIterator & operator++();
    T & operator*();
    T * operator->();

protected:
    T * m_current{};
};

//===========================================================================
template <typename T>
ForwardListIterator<T>::ForwardListIterator(T * node)
    : m_current(node)
{}

//===========================================================================
template <typename T>
ForwardListIterator<T> & ForwardListIterator<T>::operator++() {
    m_current = m_current->m_next;
    return *this;
}

//===========================================================================
template <typename T>
T & ForwardListIterator<T>::operator*() {
    assert(m_current);
    return *m_current;
}

//===========================================================================
template <typename T>
T * ForwardListIterator<T>::operator->() {
    return m_current;
}


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
*   template<> struct is_enum_flags<enum A : int> : std::true_type {};
*   template<> struct is_enum_flags<enum B : unsigned> : std::false_type {};
*
***/

// Filter out non-enums because underlying_type (used below) with them is
// a compile error.
template<typename T, bool = std::is_enum_v<T>>
struct is_enum_flags : std::false_type {};

// Default unsigned unscoped enums to be flags
template<typename T>
struct is_enum_flags<T, true>
    : std::bool_constant<
        std::is_unsigned_v<std::underlying_type_t<T>>
        && std::is_convertible_v<T, std::underlying_type_t<T>>
    >
{};

template<typename T> constexpr bool is_enum_flags_v = is_enum_flags<T>::value;

//===========================================================================
template<typename T>
requires is_enum_flags_v<T>
constexpr T operator~ (T val) {
    return T(~std::underlying_type_t<T>(val));
}

//===========================================================================
template<typename T>
requires is_enum_flags_v<T>
constexpr T operator| (T left, T right) {
    return T(std::underlying_type_t<T>(left) | right);
}

//===========================================================================
template<typename T>
requires is_enum_flags_v<T>
constexpr T& operator|= (T & left, T right) {
    return left = left | right;
}

//===========================================================================
template<typename T>
requires is_enum_flags_v<T>
constexpr T operator& (T left, T right) {
    return T(std::underlying_type_t<T>(left) & right);
}

//===========================================================================
template<typename T>
requires is_enum_flags_v<T>
constexpr T& operator&= (T & left, T right) {
    return left = left & right;
}

//===========================================================================
template<typename T>
requires is_enum_flags_v<T>
constexpr T operator^ (T left, T right) {
    return T(std::underlying_type_t<T>(left) ^ right);
}

//===========================================================================
template<typename T>
requires is_enum_flags_v<T>
constexpr T& operator^= (T & left, T right) {
    return left = left ^ right;
}


/****************************************************************************
*
*   IFactory
*
*   If a service defines Base* and accepts IFactory<Base>* in its interface,
*   clients that define their own class (Derived) derived from Base can use
*   getFactory<Base, Derived>() to get a pointer to a suitable static factory.
*
*   Example usage:
*   // Service interface
*   void socketListen(IFactory<ISocketNotify> * fact, const SockAddr & e);
*
*   // Client implementation
*   class MySocket : public ISocketNotify {
*   };
*
*   socketListen(getFactory<ISocketNotify, MySocket>(), sockAddr);
*
***/

template<typename T>
class IFactory {
public:
    virtual ~IFactory() = default;
    virtual std::unique_ptr<T> onFactoryCreate() = 0;
};

//===========================================================================
template<typename Base, typename Derived>
requires std::is_base_of_v<Base, Derived>
inline IFactory<Base> * getFactory() {
    class Factory : public IFactory<Base> {
        std::unique_ptr<Base> onFactoryCreate() override {
            return std::make_unique<Derived>();
        }
    };
    // As per http://en.cppreference.com/w/cpp/language/inline
    // "In an inline function, function-local static objects in all function
    // definitions are shared across all translation units (they all refer to
    // the same object defined in one translation unit)"
    //
    // Note that this is a difference between C and C++
    static Factory s_factory;
    return &s_factory;
}

} // namespace
