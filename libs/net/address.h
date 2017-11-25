// Copyright Glen Knowles 2015 - 2017.
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
constexpr uint32_t kIPv4MappedAddress = 0xffff0000;

// IP v4 or v6 address
struct Address {
    uint32_t data[4] = { 0, 0, kIPv4MappedAddress, 0 };

    bool operator==(const Address & right) const;
    bool operator<(const Address & right) const;
    explicit operator bool() const;
};
struct Endpoint {
    Address addr;
    unsigned port{0};

    bool operator==(const Endpoint & right) const;
    bool operator<(const Endpoint & right) const;
    explicit operator bool() const;
};
struct Network {
    Address addr;
    int mask{0};
};
} // namespace

namespace std {
template <> struct hash<Dim::Address> {
    size_t operator()(const Dim::Address & val) const;
};
template <> struct hash<Dim::Endpoint> {
    size_t operator()(const Dim::Endpoint & val) const;
};
template <> struct hash<Dim::Network> {
    size_t operator()(const Dim::Network & val) const;
};
} // namespace std

namespace Dim {

bool parse(Address * addr, std::string_view src);
bool parse(Endpoint * end, std::string_view src, int defaultPort);

std::ostream & operator<<(std::ostream & os, const Address & addr);
std::ostream & operator<<(std::ostream & os, const Endpoint & end);
std::istream & operator>>(std::istream & in, Address & out);
std::istream & operator>>(std::istream & in, Endpoint & out);


//===========================================================================
// Native
//===========================================================================
void copy(sockaddr_storage * out, const Endpoint & end);
void copy(Endpoint * out, const sockaddr_storage & storage);


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
    virtual void onEndpointFound(const Endpoint * ptr, int count) = 0;
};

void endpointQuery(
    int * cancelId,
    IEndpointNotify * notify,
    std::string_view name,
    int defaultPort
);
void endpointCancelQuery(int cancelId);

} // namespace dim
