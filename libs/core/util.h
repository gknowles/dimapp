// Copyright Glen Knowles 2016 - 2019.
// Distributed under the Boost Software License, Version 1.0.
//
// util.h - dim core
#pragma once

#include "cppconf/cppconf.h"

#include "math.h"

#include <cassert>
#include <cstdint>
#include <memory>
#include <string>
#include <string_view>

namespace Dim {


/****************************************************************************
*
*   Crypt random
*
***/

void cryptRandomBytes(void * ptr, size_t count);


/****************************************************************************
*
*   Hex conversions
*
***/

//===========================================================================
constexpr bool isHex(unsigned char ch) {
    // return ch - '0' <= 9 || (ch | 0x20) - 'a' <= 5;

    switch (ch) {
    case '0': case '1': case '2': case '3': case '5': case '6':
    case '7': case '8': case '9':
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'a': case 'b': case 'c': case 'd': case 'e': case 'f':
        return true;
    default:
        return false;
    }
}

//===========================================================================
// hexToNibble - converts hex character (0-9, a-f, A-F) to unsigned (0-15)
constexpr unsigned hexToNibbleUnsafe(char ch) {
    return ((ch | 432) * 239'217'992 & 0xffff'ffff) >> 28;
}

//===========================================================================
constexpr char hexFromNibble(unsigned val) {
    assert(val < 16);
    char const s_chars[] = "0123456789abcdef";
    return s_chars[val];
}

// On invalid input these return 16 and 256 respectively
unsigned hexToNibble(unsigned char val);
unsigned hexToByte(unsigned char high, unsigned char low);

bool hexToBytes(std::string & out, std::string_view src, bool append);
void hexFromBytes(std::string & out, std::string_view src, bool append);

std::ostream & hexByte(std::ostream & os, char data);


/****************************************************************************
*
*   Endian conversions
*
***/

//===========================================================================
constexpr uint8_t ntoh8(void const * vptr) {
    auto ptr = static_cast<char const *>(vptr);
    return (uint8_t)ptr[0];
}

//===========================================================================
constexpr uint16_t ntoh16(void const * vptr) {
    auto val = *static_cast<uint16_t const *>(vptr);
    return bswap16(val);
}

//===========================================================================
constexpr uint32_t ntoh24(void const * vptr) {
    auto ptr = static_cast<char const *>(vptr);
    return ((uint32_t)(uint8_t)ptr[0] << 16)
        + ((uint32_t)(uint8_t)ptr[1] << 8)
        + (uint8_t)ptr[2];
}

//===========================================================================
constexpr uint32_t ntoh32(void const * vptr) {
    auto val = *static_cast<uint32_t const *>(vptr);
    return bswap32(val);
}

//===========================================================================
constexpr uint64_t ntoh64(void const * vptr) {
    auto val = *static_cast<uint64_t const *>(vptr);
    return bswap64(val);
}

//===========================================================================
constexpr float ntohf32(void const * vptr) {
    static_assert(sizeof(float) == 4 && std::numeric_limits<float>::is_iec559);
    float val = 0;
    *((uint32_t*) &val) = ntoh32(vptr);
    return val;
}

//===========================================================================
constexpr double ntohf64(void const * vptr) {
    static_assert(sizeof(double) == 8 && std::numeric_limits<float>::is_iec559);
    double val = 0;
    *((uint64_t*) &val) = ntoh64(vptr);
    return val;
}

//===========================================================================
constexpr char * hton16(void * out, unsigned val) {
    *(uint16_t *)out = bswap16(val);
    return (char *) out;
}

//===========================================================================
constexpr char * hton24(void * vout, unsigned val) {
    auto out = (char *) vout;
    *out++ = (val >> 16) & 0xff;
    *out++ = (val >> 8) & 0xff;
    *out++ = val & 0xff;
    return out;
}

//===========================================================================
constexpr char * hton32(void * out, unsigned val) {
    *(uint32_t *)out = bswap32(val);
    return (char *) out;
}

//===========================================================================
constexpr char * hton64(void * out, uint64_t val) {
    *(uint64_t *)out = bswap64(val);
    return (char *) out;
}

//===========================================================================
constexpr char * htonf32(void * out, float val) {
    return hton32(out, *(uint32_t *) &val);
}

//===========================================================================
constexpr char * htonf64(void * out, double val) {
    return hton64(out, *(uint64_t *) &val);
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
*   void socketListen(IFactory<ISocketNotify> * fact, SockAddr const & e);
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

} // namespace
