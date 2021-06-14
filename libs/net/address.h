// Copyright Glen Knowles 2015 - 2019.
// Distributed under the Boost Software License, Version 1.0.
//
// address.h - dim net
#pragma once

/****************************************************************************
*
*   Networking
*
*   HostAddr - machine location (IP)
*   SockAddr - machine location (IP) and service at location (port)
*   SubnetAddr - network location (IP) and size (net mask)
*
***/

#include "cppconf/cppconf.h"

#include "core/types.h"

#include <compare>
#include <iostream>
#include <string_view>
#include <vector>

// forward declarations
struct sockaddr_storage;

namespace Dim {


/****************************************************************************
*
*   Declarations
*
***/

// ::ffff:0:0/96 reserved for IPv4-mapped HostAddr [RFC4291]
constexpr uint32_t kIpv4MappedAddress = 0xffff0000;

// IP v4 or v6 address
struct HostAddr {
    uint32_t data[4] = { 0, 0, kIpv4MappedAddress, 0 };

    bool isIpv4() const;
    uint32_t getIpv4() const;

    std::strong_ordering operator<=>(const HostAddr & right) const = default;
    explicit operator bool() const;

private:
    friend std::ostream & operator<<(std::ostream & os, const HostAddr & addr);
    friend std::istream & operator>>(std::istream & in, HostAddr & out);
};
struct SockAddr {
    HostAddr addr;
    unsigned port = {};
    unsigned scope = {}; // aka zone

    std::strong_ordering operator<=>(const SockAddr & right) const = default;
    explicit operator bool() const;

private:
    friend std::ostream & operator<<(std::ostream & os, const SockAddr & end);
    friend std::istream & operator>>(std::istream & in, SockAddr & out);
};
struct SubnetAddr {
    HostAddr addr;
    int mask = {};
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

[[nodiscard]] bool parse(HostAddr * addr, std::string_view src);
[[nodiscard]] bool parse(
    SockAddr * end,
    std::string_view src,
    int defaultPort
);

//===========================================================================
// Native
//===========================================================================
void copy(sockaddr_storage * out, const SockAddr & end);
void copy(SockAddr * out, const sockaddr_storage & storage);
size_t bytesUsed(const sockaddr_storage & storage);


/****************************************************************************
*
*   Lookup
*
***/

void addressGetLocal(std::vector<HostAddr> * out);

class ISockAddrNotify {
public:
    virtual ~ISockAddrNotify() = default;
    // count of 0 means either no results or some kind of error occurred
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
