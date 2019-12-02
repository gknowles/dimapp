// Copyright Glen Knowles 2015 - 2019.
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
*   NetAddr
*
***/

//===========================================================================
size_t std::hash<NetAddr>::operator()(const NetAddr & val) const {
    size_t out = 0;
    hashCombine(&out, std::hash<int32_t>{}(val.data[0]));
    hashCombine(&out, std::hash<int32_t>{}(val.data[1]));
    hashCombine(&out, std::hash<int32_t>{}(val.data[2]));
    hashCombine(&out, std::hash<int32_t>{}(val.data[3]));
    static_assert(sizeof val.data == 4 * sizeof int32_t);
    return out;
}

//===========================================================================
bool NetAddr::isIpv4() const {
    return data[2] == kIpv4MappedAddress;
}

//===========================================================================
uint32_t NetAddr::getIpv4() const {
    return data[3];
}

//===========================================================================
NetAddr::operator bool() const {
    return data[3]
        || data[0]
        || data[1]
        || data[2] && data[2] != kIpv4MappedAddress;
}

//===========================================================================
istream & Dim::operator>>(istream & in, NetAddr & out) {
    string tmp;
    in >> tmp;
    if (!parse(&out, tmp.c_str()))
        in.setstate(ios_base::failbit);
    return in;
}


/****************************************************************************
*
*   SockAddr
*
***/

//===========================================================================
size_t std::hash<SockAddr>::operator()(const SockAddr & val) const {
    size_t out = 0;
    hashCombine(&out, std::hash<NetAddr>{}(val.addr));
    hashCombine(&out, std::hash<unsigned>{}(val.port));
    hashCombine(&out, std::hash<unsigned>{}(val.scope));
    return out;
}

//===========================================================================
SockAddr::operator bool() const {
    return port || addr;
}

//===========================================================================
istream & Dim::operator>>(istream & in, SockAddr & out) {
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
    hashCombine(&out, std::hash<NetAddr>{}(val.addr));
    hashCombine(&out, std::hash<int>{}(val.mask));
    return out;
}
