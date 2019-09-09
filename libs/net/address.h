// Copyright Glen Knowles 2015 - 2019.
// Distributed under the Boost Software License, Version 1.0.
//
// address.h - dim net
#pragma once

/****************************************************************************
*
*   Networking
*
*   NetAddr - machine location (IP)
*   SockAddr - machine location (IP) and service at location (port)
*   Network - network location (IP) and size (net mask)
*
***/

#include "cppconf/cppconf.h"

#include "core/types.h"

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

// ::ffff:0:0/96 reserved for IPv4-mapped NetAddr [RFC4291]
constexpr uint32_t kIpv4MappedAddress = 0xffff0000;

// IP v4 or v6 address
struct NetAddr {
    uint32_t data[4] = { 0, 0, kIpv4MappedAddress, 0 };

    bool isIpv4() const;
    uint32_t getIpv4() const;

    bool operator==(NetAddr const & right) const;
    bool operator<(NetAddr const & right) const;
    explicit operator bool() const;
};
struct SockAddr {
    NetAddr addr;
    unsigned port{0};

    bool operator==(SockAddr const & right) const;
    bool operator<(SockAddr const & right) const;
    explicit operator bool() const;
};
struct Network {
    NetAddr addr;
    int mask{0};
};
} // namespace

namespace std {
template <> struct hash<Dim::NetAddr> {
    size_t operator()(Dim::NetAddr const & val) const;
};
template <> struct hash<Dim::SockAddr> {
    size_t operator()(Dim::SockAddr const & val) const;
};
template <> struct hash<Dim::Network> {
    size_t operator()(Dim::Network const & val) const;
};
} // namespace std

namespace Dim {

[[nodiscard]] bool parse(NetAddr * addr, std::string_view src);
[[nodiscard]] bool parse(SockAddr * end, std::string_view src, int defaultPort);

std::ostream & operator<<(std::ostream & os, NetAddr const & addr);
std::ostream & operator<<(std::ostream & os, SockAddr const & end);
std::istream & operator>>(std::istream & in, NetAddr & out);
std::istream & operator>>(std::istream & in, SockAddr & out);


//===========================================================================
// Native
//===========================================================================
void copy(sockaddr_storage * out, SockAddr const & end);
void copy(SockAddr * out, sockaddr_storage const & storage);


/****************************************************************************
*
*   Lookup
*
***/

void addressGetLocal(std::vector<NetAddr> * out);

class ISockAddrNotify {
public:
    virtual ~ISockAddrNotify() = default;
    // count of 0 means either no results or some kind of error occurred
    virtual void onSockAddrFound(SockAddr const * ptr, int count) = 0;
};

void addressQuery(
    int * cancelId,
    ISockAddrNotify * notify,
    std::string_view name,
    int defaultPort
);
void addressCancelQuery(int cancelId);

} // namespace dim
