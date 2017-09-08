// Copyright Glen Knowles 2016 - 2017.
// Distributed under the Boost Software License, Version 1.0.
//
// util.h - dim core
#pragma once

#include "cppconf/cppconf.h"

#include <cassert>
#include <cstdint>
#include <memory>

namespace Dim {


/****************************************************************************
*
*   Crypt random
*
***/

void cryptRandomBytes(void * ptr, size_t count);


/****************************************************************************
*
*   Hashing
*
***/

size_t hashBytes(const void * ptr, size_t count);

size_t hashStr(const char src[]);

// calculates hash up to trailing null or maxlen, whichever comes first
size_t hashStr(const char src[], size_t maxlen);

//===========================================================================
constexpr void hashCombine(size_t & seed, size_t v) {
    seed ^= v + 0x9e3779b9 + (seed << 6) + (seed >> 2);
}


/****************************************************************************
*
*   Hex nibble conversions
*
***/

//===========================================================================
// hexToNibble - converts hex character (0-9, a-f, A-F) to unsigned (0-15)
constexpr unsigned hexToNibble(char ch) {
    return ((ch | 432) * 239'217'992 & 0xffff'ffff) >> 28;
}

//===========================================================================
constexpr char hexFromNibble(unsigned val) {
    assert(val < 16);
    const char s_chars[] = "0123456789abcdef";
    return s_chars[val];
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
*   void socketListen(IFactory<ISocketNotify> * fact, const Endpoint & e);
*
*   // Client implementation
*   class MySocket : public ISocketNotify {
*   };
*
*   socketListen(getFactory<ISocketNotify, MySocket>(), endpoint);
*
***/

template<typename T>
class IFactory {
public:
    virtual ~IFactory() {}
    virtual std::unique_ptr<T> onFactoryCreate() = 0;
};

//===========================================================================
template<typename Base, typename Derived> 
inline IFactory<Base> * getFactory() {
    static_assert(std::is_base_of_v<Base, Derived>);

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


/****************************************************************************
*
*   NoCopy base class
*
*   Prevents any class dervied from NoCopy from being copy constructed or
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

} // namespace
