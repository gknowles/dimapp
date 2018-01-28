// Copyright Glen Knowles 2015 - 2018.
// Distributed under the Boost Software License, Version 1.0.
//
// address.cpp - dim net
#include "pch.h"
#pragma hdrstop

using namespace std;
using namespace Dim;


/****************************************************************************
*
*   Helpers
*
***/


/****************************************************************************
*
*   Address
*
***/

//===========================================================================
size_t std::hash<Address>::operator()(const Address & val) const {
    size_t out = 0;
    hashCombine(&out, std::hash<int32_t>{}(val.data[0]));
    hashCombine(&out, std::hash<int32_t>{}(val.data[1]));
    hashCombine(&out, std::hash<int32_t>{}(val.data[2]));
    hashCombine(&out, std::hash<int32_t>{}(val.data[3]));
    static_assert(sizeof(val.data) == 4 * sizeof(int32_t));
    return out;
}

//===========================================================================
bool Address::operator==(const Address & right) const {
    return memcmp(this, &right, sizeof *this) == 0;
}

//===========================================================================
bool Address::operator<(const Address & right) const {
    return memcmp(this, &right, sizeof *this) < 0;
}

//===========================================================================
Address::operator bool() const {
    return data[3]
        || data[0]
        || data[1]
        || data[2] && data[2] != kIPv4MappedAddress;
}

//===========================================================================
istream & Dim::operator>>(istream & in, Address & out) {
    string tmp;
    in >> tmp;
    if (!parse(&out, tmp.c_str()))
        in.setstate(ios_base::failbit);
    return in;
}


/****************************************************************************
*
*   Endpoint
*
***/

//===========================================================================
size_t std::hash<Endpoint>::operator()(const Endpoint & val) const {
    size_t out = 0;
    hashCombine(&out, std::hash<Address>{}(val.addr));
    hashCombine(&out, std::hash<unsigned>{}(val.port));
    return out;
}

//===========================================================================
bool Endpoint::operator==(const Endpoint & right) const {
    return port == right.port && addr == right.addr;
}

//===========================================================================
bool Endpoint::operator<(const Endpoint & right) const {
    return tie(addr, port) < tie(right.addr, right.port);
}

//===========================================================================
Endpoint::operator bool() const {
    return port || addr;
}

//===========================================================================
istream & Dim::operator>>(istream & in, Endpoint & out) {
    string tmp;
    in >> tmp;
    if (!parse(&out, tmp.c_str(), 0))
        in.setstate(ios_base::failbit);
    return in;
}


/****************************************************************************
*
*   Network
*
***/

//===========================================================================
size_t std::hash<Network>::operator()(const Network & val) const {
    size_t out = 0;
    hashCombine(&out, std::hash<Address>{}(val.addr));
    hashCombine(&out, std::hash<int>{}(val.mask));
    return out;
}
