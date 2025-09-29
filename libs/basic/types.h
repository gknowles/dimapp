// Copyright Glen Knowles 2015 - 2024.
// Distributed under the Boost Software License, Version 1.0.
//
// types.h - dim basic
#pragma once

#include "cppconf/cppconf.h"

#include <bit>
#include <cassert>
#include <compare>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <system_error>
#include <type_traits>

namespace Dim {


/****************************************************************************
*
*   Concepts
*
***/

template <typename T>
concept CharType = std::is_same_v<std::remove_cv_t<T>, char>
    || std::is_same_v<std::remove_cv_t<T>, signed char>
    || std::is_same_v<std::remove_cv_t<T>, unsigned char>
    || std::is_same_v<std::remove_cv_t<T>, char8_t>
    || std::is_same_v<std::remove_cv_t<T>, char16_t>
    || std::is_same_v<std::remove_cv_t<T>, char32_t>
    || std::is_same_v<std::remove_cv_t<T>, wchar_t>;


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
    explicit operator bool() const { return *this != VersionInfo{}; }
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
    using function::operator=;
    ~Finally();
    void release();
};

//===========================================================================
inline Finally::~Finally() {
    if (*this)
        (*this)();
}

//===========================================================================
inline void Finally::release() {
    *this = {};
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
*   EnumFlags
*
*   Allow bitwise NOT, AND, XOR and OR operations on enum values
*
*   The result of bitwise operations on enums is an instance of the EnumFlags<T>
*   class templated on the enum. There is no conversion back from a EnumFlags
*   object to an enum.
*
*   Function arguments that expect a bit field of options or'd together should
*   do it by taking a EnumFlags<T> by value.
*
*   Testing of flags is done with the any(), all(), and none() member
*   functions.
*
*   enum EnumA : unsigned { A1 = 1, A2 = 2, A3 = 4 };
*   EnumFlags<EnumA> flags = A1 | A2;
*   flags |= A3;
*
*   bool testFor1and3(EnumFlags<EnumA> flags) {
*       return flags.all(A1 | A3);
*   }
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

template<typename T>
inline constexpr bool is_enum_flags_v = is_enum_flags<T>::value;

template<typename T>
concept EnumFlagsType = is_enum_flags_v<T>;

template<EnumFlagsType T>
class EnumFlags {
public:
    using enum_type = T;

public:
    constexpr EnumFlags() = default;
    constexpr EnumFlags(const EnumFlags & val) = default;
    constexpr EnumFlags(T val);

    constexpr EnumFlags & set (EnumFlags other);
    constexpr EnumFlags & reset ();
    constexpr EnumFlags & reset (EnumFlags other);
    constexpr EnumFlags & flip (EnumFlags other);

    constexpr auto underlying() const;
    constexpr T value() const;

    constexpr bool operator==(const EnumFlags & val) const = default;
    constexpr bool any() const;
    constexpr bool any(EnumFlags mask) const;

    // Prefer any() for testing a single bit, use of all() should imply that
    // the mask will, some of the time, have multiple bits.
    constexpr bool all(EnumFlags mask) const;

    constexpr size_t count() const;
    constexpr size_t count(EnumFlags mask) const;

    constexpr EnumFlags & operator= (const EnumFlags & other) = default;
    constexpr EnumFlags & operator|= (EnumFlags other);
    constexpr EnumFlags & operator&= (EnumFlags other);
    constexpr EnumFlags & operator^= (EnumFlags other);

    // PRIVATE
    // Access is public to make this a structural class type; so that it can
    // be used as a non-type template parameter.
    std::underlying_type_t<T> m_value;
};

//===========================================================================
template<EnumFlagsType T>
constexpr EnumFlags<T>::EnumFlags(T val)
    : m_value(std::to_underlying(val))
{}

//===========================================================================
template<EnumFlagsType T>
constexpr auto EnumFlags<T>::underlying() const {
    return m_value;
}

//===========================================================================
template<EnumFlagsType T>
constexpr T EnumFlags<T>::value() const {
    return static_cast<T>(m_value);
}

//===========================================================================
template<EnumFlagsType T>
constexpr EnumFlags<T> & EnumFlags<T>::set(EnumFlags other) {
    m_value |= other.underlying();
    return *this;
}

//===========================================================================
template<EnumFlagsType T>
constexpr EnumFlags<T> & EnumFlags<T>::reset() {
    m_value = {};
    return *this;
}

//===========================================================================
template<EnumFlagsType T>
constexpr EnumFlags<T> & EnumFlags<T>::reset(EnumFlags other) {
    m_value &= ~other.underlying();
    return *this;
}

//===========================================================================
template<EnumFlagsType T>
constexpr EnumFlags<T> & EnumFlags<T>::flip(EnumFlags other) {
    m_value ^= other.underlying();
    return *this;
}

//===========================================================================
template<EnumFlagsType T>
constexpr bool EnumFlags<T>::any() const {
    return m_value;
}

//===========================================================================
template<EnumFlagsType T>
constexpr bool EnumFlags<T>::any(EnumFlags other) const {
    return (m_value & other.underlying()) != 0;
}

//===========================================================================
template<EnumFlagsType T>
constexpr bool EnumFlags<T>::all(EnumFlags other) const {
    return (m_value & other.underlying()) == other.underlying();
}

//===========================================================================
template<EnumFlagsType T>
constexpr size_t EnumFlags<T>::count() const {
    return std::popcount(m_value);
}

//===========================================================================
template<EnumFlagsType T>
constexpr size_t EnumFlags<T>::count(EnumFlags other) const {
    return std::popcount(m_value & other.underlying());
}

//===========================================================================
template<EnumFlagsType T>
constexpr EnumFlags<T> & EnumFlags<T>::operator|=(EnumFlags other) {
    m_value |= other.underlying();
    return *this;
}

//===========================================================================
template<EnumFlagsType T>
constexpr EnumFlags<T> & EnumFlags<T>::operator&=(EnumFlags other) {
    m_value &= other.underlying();
    return *this;
}

//===========================================================================
template<EnumFlagsType T>
constexpr EnumFlags<T> & EnumFlags<T>::operator^=(EnumFlags other) {
    m_value ^= other.underlying();
    return *this;
}

//===========================================================================
template<EnumFlagsType T>
constexpr EnumFlags<T> operator~(EnumFlags<T> a) {
    return ~a.underlying();
}

//===========================================================================
template<EnumFlagsType T>
constexpr auto operator|(EnumFlags<T> a, EnumFlags<T> b) {
    return EnumFlags<T>(a) |= b;
}
template<EnumFlagsType T>
constexpr auto operator|(EnumFlags<T> a, T b) {
    return EnumFlags<T>(a) |= b;
}
template<EnumFlagsType T>
constexpr auto operator|(T a, EnumFlags<T> b) {
    return EnumFlags<T>(a) |= b;
}
template<EnumFlagsType T>
constexpr auto operator|(T a, T b) {
    return EnumFlags<T>(a) |= b;
}

//===========================================================================
template<EnumFlagsType T>
constexpr auto operator&(EnumFlags<T> a, EnumFlags<T> b) {
    return EnumFlags<T>(a) &= b;
}
template<EnumFlagsType T>
constexpr auto operator&(EnumFlags<T> a, T b) {
    return EnumFlags<T>(a) &= b;
}
template<EnumFlagsType T>
constexpr auto operator&(T a, EnumFlags<T> b) {
    return EnumFlags<T>(a) &= b;
}
template<EnumFlagsType T>
constexpr auto operator&(T a, T b) {
    return EnumFlags<T>(a) &= b;
}

//===========================================================================
template<EnumFlagsType T>
constexpr auto operator^(EnumFlags<T> a, EnumFlags<T> b) {
    return EnumFlags<T>(a) ^= b;
}
template<EnumFlagsType T>
constexpr auto operator^(EnumFlags<T> a, T b) {
    return EnumFlags<T>(a) ^= b;
}
template<EnumFlagsType T>
constexpr auto operator^(T a, EnumFlags<T> b) {
    return EnumFlags<T>(a) ^= b;
}
template<EnumFlagsType T>
constexpr auto operator^(T a, T b) {
    return EnumFlags<T>(a) ^= b;
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
requires std::derived_from<Derived, Base>
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
