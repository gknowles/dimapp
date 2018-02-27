// Copyright Glen Knowles 2016 - 2018.
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
// hexToNibble - converts hex character (0-9, a-f, A-F) to unsigned (0-15)
constexpr unsigned hexToNibbleUnsafe(char ch) {
    return ((ch | 432) * 239'217'992 & 0xffff'ffff) >> 28;
}

//===========================================================================
constexpr char hexFromNibble(unsigned val) {
    assert(val < 16);
    const char s_chars[] = "0123456789abcdef";
    return s_chars[val];
}

// On invalid input these return 16 and 256 respectively
unsigned hexToNibble(unsigned char val);
unsigned hexToByte(unsigned char high, unsigned char low);

bool hexToBytes(std::string & out, std::string_view src, bool append);
void hexFromBytes(std::string & out, std::string_view src, bool append);


/****************************************************************************
*
*   Endian conversions
*
***/

//===========================================================================
constexpr int ntoh8(const void * vptr) {
    auto ptr = static_cast<const char *>(vptr);
    return (uint32_t)(uint8_t)ptr[0];
}

//===========================================================================
constexpr int ntoh16(const void * vptr) {
    auto val = *static_cast<const uint16_t *>(vptr);
    return bswap16(val);
}

//===========================================================================
constexpr int ntoh24(const void * vptr) {
    auto ptr = static_cast<const char *>(vptr);
    return ((uint32_t)(uint8_t)ptr[0] << 16)
        + ((uint32_t)(uint8_t)ptr[1] << 8)
        + (uint8_t)ptr[2];
}

//===========================================================================
constexpr int ntoh32(const void * vptr) {
    auto val = *static_cast<const int32_t *>(vptr);
    return bswap32(val);
}

//===========================================================================
constexpr int64_t ntoh64(const void * vptr) {
    auto val = *static_cast<const uint64_t *>(vptr);
    return bswap64(val);
}

//===========================================================================
constexpr float ntohf32(const void * vptr) {
    static_assert(sizeof(float) == 4 && std::numeric_limits<float>::is_iec559);
    float val = 0;
    *((uint32_t*) &val) = ntoh32(vptr);
    return val;
}

//===========================================================================
constexpr double ntohf64(const void * vptr) {
    static_assert(sizeof(double) == 8 && std::numeric_limits<float>::is_iec559);
    double val = 0;
    *((uint64_t*) &val) = ntoh64(vptr);
    return val;
}

//===========================================================================
constexpr char * hton16(char * out, int val) {
    *(int16_t *)out = bswap16(val);
    return out;
}

//===========================================================================
constexpr char * hton24(char * out, int val) {
    *out++ = (val >> 16) & 0xff;
    *out++ = (val >> 8) & 0xff;
    *out++ = val & 0xff;
    return out;
}

//===========================================================================
constexpr char * hton32(char * out, int val) {
    *(int32_t *)out = bswap32(val);
    return out;
}

//===========================================================================
constexpr char * hton64(char * out, int64_t val) {
    *(int64_t *)out = bswap64(val);
    return out;
}

//===========================================================================
constexpr char * htonf32(char * out, float val) {
    return hton32(out, *(uint32_t *) &val);
}

//===========================================================================
constexpr char * htonf64(char * out, double val) {
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
