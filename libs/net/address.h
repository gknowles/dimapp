// Copyright Glen Knowles 2015 - 2018.
// Distributed under the Boost Software License, Version 1.0.
//
// address.h - dim net
#pragma once

/****************************************************************************
*
*   Networking
*
*   Address - machine location (IP)
*   Endpoint - machine location (IP) and service at location (port)
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

// ::ffff:0:0/96 reserved for IPv4-mapped Address [RFC4291]
constexpr uint32_t kIpv4MappedAddress = 0xffff0000;

// IP v4 or v6 address
struct Address {
    uint32_t data[4] = { 0, 0, kIpv4MappedAddress, 0 };

    bool isIpv4() const;
    uint32_t getIpv4() const;

    bool operator==(Address const & right) const;
    bool operator<(Address const & right) const;
    explicit operator bool() const;
};
struct Endpoint {
    Address addr;
    unsigned port{0};

    bool operator==(Endpoint const & right) const;
    bool operator<(Endpoint const & right) const;
    explicit operator bool() const;
};
struct Network {
    Address addr;
    int mask{0};
};
} // namespace

namespace std {
template <> struct hash<Dim::Address> {
    size_t operator()(Dim::Address const & val) const;
};
template <> struct hash<Dim::Endpoint> {
    size_t operator()(Dim::Endpoint const & val) const;
};
template <> struct hash<Dim::Network> {
    size_t operator()(Dim::Network const & val) const;
};
} // namespace std

namespace Dim {

[[nodiscard]] bool parse(Address * addr, std::string_view src);
[[nodiscard]] bool parse(Endpoint * end, std::string_view src, int defaultPort);

std::ostream & operator<<(std::ostream & os, Address const & addr);
std::ostream & operator<<(std::ostream & os, Endpoint const & end);
std::istream & operator>>(std::istream & in, Address & out);
std::istream & operator>>(std::istream & in, Endpoint & out);


//===========================================================================
// Native
//===========================================================================
void copy(sockaddr_storage * out, Endpoint const & end);
void copy(Endpoint * out, sockaddr_storage const & storage);


/****************************************************************************
*
*   Lookup
*
***/

void addressGetLocal(std::vector<Address> * out);

class IEndpointNotify {
public:
    virtual ~IEndpointNotify() = default;
    // count of 0 means either no results or some kind of error occurred
    virtual void onEndpointFound(Endpoint const * ptr, int count) = 0;
};

void endpointQuery(
    int * cancelId,
    IEndpointNotify * notify,
    std::string_view name,
    int defaultPort
);
void endpointCancelQuery(int cancelId);

} // namespace dim
