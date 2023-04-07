// Copyright Glen Knowles 2015 - 2023.
// Distributed under the Boost Software License, Version 1.0.
//
// address.h - dim net
#pragma once


/****************************************************************************
*
*   Networking
*
*   HostAddr - machine location (IP)
*       "a.b.c.d", such as 127.0.0.1, [::1], 192.0.2.1, [2001:db8::1]
*   SockAddr - HostAddr and service at location (port)
*       "a.b.c.d:<port>"
*   SubnetAddr - HostAddr and number of bits that are the network prefix
*       "a.b.c.d/<prefix>"
*
***/

#include "cppconf/cppconf.h"

#include "core/types.h"

#include <compare>
#include <iostream>
#include <string_view>
#include <vector>

// Forward declarations
struct sockaddr_storage;

namespace Dim {


/****************************************************************************
*
*   Declarations
*
***/

// IP v4 or v6 address
struct HostAddr {
    uint8_t data[16] = {};  // network byte order (i.e. big endian)
    uint32_t scope = {}; // aka IPv6 zone

public:
    // Address type
    //
    // NOTE: Listed in reverse priority order.
    enum Type {
        kUnspecified,   // no address specified
            // 0.0.0.0/32 [RFC 1122]
            // ::/128 [RFC 4291]
        kLoopback,
            // 127.0.0.0/8 [RFC 1122]
            // ::1/128 [RFC 4291]
        kMulticast,
            // 224.0.0.0/4 [RFC 1112, *not* 1122]
            // ff00::/8 [RFC 4291]
        kLinkLocal,
            // 169.254.0.0/16 [RFC 3927]
            // fe80::/10 [RFC 4291]
        kPrivate,
            // 10.0.0.0/8, 172.16.0.0/12, 192.168.0.0/16 [RFC 1918]
            // fc00::/7 [RFC 4193]
        kPublic,
    };

    // Addresses are stored in Ipv6 format.
    enum Family {
        kUnspec,    // no address specified
        kIpv4,      // mapped from Ipv4 as "::FFFF.a.b.c.d" [RFC 4291]
        kIpv6,      // anything else
    };

    Family family() const;

    // Address type, such as loopback, private, or public.
    Type type() const;

    // Asserts if not "::" or "::FFFF.a.b.c.d"
    uint32_t ipv4() const;

    // Sets to "::FFFF.a.b.c.d" format
    uint32_t ipv4(uint32_t addr);

    std::strong_ordering operator<=>(const HostAddr & right) const = default;
    explicit operator bool() const;

private:
    friend std::ostream & operator<<(std::ostream & os, const HostAddr & addr);
    friend std::istream & operator>>(std::istream & in, HostAddr & out);
};

struct SockAddr {
    HostAddr addr;
    unsigned port = {};

    std::strong_ordering operator<=>(const SockAddr & right) const = default;
    explicit operator bool() const;

private:
    friend std::ostream & operator<<(std::ostream & os, const SockAddr & addr);
    friend std::istream & operator>>(std::istream & in, SockAddr & out);
};

struct SubnetAddr {
    HostAddr addr;
    unsigned prefixLen = {};

private:
    friend std::ostream & operator<<(
        std::ostream & os,
        const SubnetAddr & addr
    );
    friend std::istream & operator>>(
        std::istream & in,
        SubnetAddr & out
    );
};
} // namespace

namespace std {
template <> struct hash<Dim::HostAddr> {
    size_t operator()(const Dim::HostAddr & val) const;
};
template <> struct hash<Dim::SockAddr> {
    size_t operator()(const Dim::SockAddr & val) const;
};
template <> struct hash<Dim::SubnetAddr> {
    size_t operator()(const Dim::SubnetAddr & val) const;
};
} // namespace std

namespace Dim {

[[nodiscard]] bool parse(HostAddr * out, std::string_view src);
[[nodiscard]] bool parse(
    SockAddr * out,
    std::string_view src,
    int defaultPort
);
[[nodiscard]] bool parse(SubnetAddr * out, std::string_view src);

std::string toString(const HostAddr & addr);
std::string toString(const SockAddr & addr);
std::string toString(const SubnetAddr & addr);

//===========================================================================
// Native
//===========================================================================
size_t bytesUsed(const sockaddr_storage & storage);
void copy(sockaddr_storage * out, const HostAddr & addr);
void copy(HostAddr * out, const sockaddr_storage & storage);
void copy(sockaddr_storage * out, const SockAddr & addr);
void copy(SockAddr * out, const sockaddr_storage & storage);


/****************************************************************************
*
*   Lookup
*
***/

void addressGetLocal(std::vector<HostAddr> * out);

class ISockAddrNotify {
public:
    virtual ~ISockAddrNotify() = default;
    // Count of 0 means either no results or some kind of error occurred.
    virtual void onSockAddrFound(const SockAddr * ptr, int count) = 0;
};

void addressQuery(
    int * cancelId,
    ISockAddrNotify * notify,
    std::string_view name,
    int defaultPort
);
void addressCancelQuery(int cancelId);

} // namespace dim
