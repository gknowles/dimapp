// Copyright Glen Knowles 2016 - 2024.
// Distributed under the Boost Software License, Version 1.0.
//
// endian.h - dim core
#pragma once

#include "cppconf/cppconf.h"

#include <bit>
#include <cassert>
#include <cstdint>
#include <memory>

namespace Dim {


/****************************************************************************
*
*   Endian conversions
*
***/

//===========================================================================
// Network to host endian
//===========================================================================
constexpr uint16_t ntoh16(uint16_t val) {
    if constexpr (std::endian::native == std::endian::big) {
        return val;
    } else {
        return std::byteswap(val);
    }
}

//===========================================================================
constexpr uint32_t ntoh32(uint32_t val) {
    if constexpr (std::endian::native == std::endian::big) {
        return val;
    } else {
        return std::byteswap(val);
    }
}

//===========================================================================
constexpr uint64_t ntoh64(uint64_t val) {
    if constexpr (std::endian::native == std::endian::big) {
        return val;
    } else {
        return std::byteswap(val);
    }
}

//===========================================================================
// Network to host endian from buffer
//===========================================================================
constexpr uint8_t ntoh8(const void * src) {
    return *static_cast<const uint8_t *>(src);
}

//===========================================================================
constexpr uint8_t ntoh8(const std::byte ** src) {
    auto ptr = *src;
    *src += sizeof(uint8_t);
    return ntoh8(ptr);
}

//===========================================================================
constexpr uint16_t ntoh16(const void * src) {
    auto ptr = static_cast<const uint16_t *>(src);
    return ntoh16(*ptr);
}

//===========================================================================
constexpr uint16_t ntoh16(const std::byte ** src) {
    auto ptr = *src;
    *src += sizeof(uint16_t);
    return ntoh16(ptr);
}

//===========================================================================
constexpr uint32_t ntoh24(const void * src) {
    auto ptr = static_cast<const uint8_t *>(src);
    return ((uint32_t)ptr[0] << 16)
        + ((uint32_t)ptr[1] << 8)
        + ptr[2];
}

//===========================================================================
constexpr uint32_t ntoh24(const std::byte ** src) {
    auto ptr = *src;
    *src += 3;
    return ntoh24(ptr);
}

//===========================================================================
constexpr uint32_t ntoh32(const void * src) {
    auto ptr = static_cast<const uint32_t *>(src);
    return ntoh32(*ptr);
}

//===========================================================================
constexpr uint32_t ntoh32(const std::byte ** src) {
    auto ptr = *src;
    *src += sizeof(uint32_t);
    return ntoh32(ptr);
}

//===========================================================================
constexpr float ntohf32(const void * src) {
    static_assert(sizeof uint32_t == 4);
    static_assert(std::numeric_limits<float>::is_iec559);
    return std::bit_cast<float>(ntoh32(src));
}

//===========================================================================
constexpr float ntohf32(const std::byte ** src) {
    static_assert(sizeof uint32_t == 4);
    static_assert(std::numeric_limits<float>::is_iec559);
    return std::bit_cast<float>(ntoh32(src));
}

//===========================================================================
constexpr uint64_t ntoh64(const void * src) {
    auto ptr = static_cast<const uint64_t *>(src);
    return ntoh64(*ptr);
}

//===========================================================================
constexpr uint64_t ntoh64(const std::byte ** src) {
    auto ptr = *src;
    *src += sizeof(uint64_t);
    return ntoh64(ptr);
}

//===========================================================================
constexpr double ntohf64(const void * src) {
    static_assert(sizeof uint64_t == 8);
    static_assert(std::numeric_limits<double>::is_iec559);
    return std::bit_cast<double>(ntoh64(src));
}

//===========================================================================
constexpr double ntohf64(const std::byte ** src) {
    static_assert(sizeof uint64_t == 8);
    static_assert(std::numeric_limits<double>::is_iec559);
    return std::bit_cast<double>(ntoh64(src));
}

//===========================================================================
// Host to network endian
//===========================================================================
constexpr uint16_t hton16(uint16_t val) {
    if constexpr (std::endian::native == std::endian::big) {
        return val;
    } else {
        return std::byteswap(val);
    }
}

//===========================================================================
constexpr uint32_t hton32(uint32_t val) {
    if constexpr (std::endian::native == std::endian::big) {
        return val;
    } else {
        return std::byteswap(val);
    }
}

//===========================================================================
constexpr uint64_t hton64(uint64_t val) {
    if constexpr (std::endian::native == std::endian::big) {
        return val;
    } else {
        return std::byteswap(val);
    }
}

//===========================================================================
// Host to network endian in buffer
//===========================================================================
constexpr std::byte * hton16(void * out, uint16_t val) {
    auto ptr = static_cast<uint16_t *>(out);
    *ptr = hton16(val);
    return static_cast<std::byte *>(out);
}

//===========================================================================
constexpr std::byte * hton16(std::byte ** out, uint16_t val) {
    auto ptr = *out;
    *out += sizeof(uint16_t);
    return hton16(ptr, val);
}

//===========================================================================
constexpr std::byte * hton24(void * out, uint32_t val) {
    auto ptr = static_cast<uint8_t *>(out);
    ptr[0] = (val >> 16) & 0xff;
    ptr[1] = (val >> 8) & 0xff;
    ptr[2] = val & 0xff;
    return static_cast<std::byte *>(out);
}

//===========================================================================
constexpr std::byte * hton24(std::byte ** out, uint32_t val) {
    auto ptr = *out;
    *out += 3;
    return hton24(ptr, val);
}

//===========================================================================
constexpr std::byte * hton32(void * out, uint32_t val) {
    auto ptr = static_cast<uint32_t *>(out);
    *ptr = hton32(val);
    return static_cast<std::byte *>(out);
}

//===========================================================================
constexpr std::byte * hton32(std::byte ** out, uint32_t val) {
    auto ptr = *out;
    *out += sizeof(uint32_t);
    return hton32(ptr, val);
}

//===========================================================================
constexpr std::byte * htonf32(void * out, float val) {
    return hton32(out, std::bit_cast<uint32_t>(val));
}

//===========================================================================
constexpr std::byte * htonf32(std::byte ** out, float val) {
    return hton32(out, std::bit_cast<uint32_t>(val));
}

//===========================================================================
constexpr std::byte * hton64(void * out, uint64_t val) {
    auto ptr = static_cast<uint64_t *>(out);
    *ptr = hton64(val);
    return static_cast<std::byte *>(out);
}

//===========================================================================
constexpr std::byte * hton64(std::byte ** out, uint64_t val) {
    auto ptr = *out;
    *out += sizeof(uint64_t);
    return hton64(ptr, val);
}

//===========================================================================
constexpr std::byte * htonf64(void * out, double val) {
    return hton64(out, std::bit_cast<uint64_t>(val));
}

//===========================================================================
constexpr std::byte * htonf64(std::byte ** out, double val) {
    return hton64(out, std::bit_cast<uint64_t>(val));
}

} // namespace
