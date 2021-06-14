// Copyright Glen Knowles 2015 - 2020.
// Distributed under the Boost Software License, Version 1.0.
//
// address.cpp - dim net
#include "pch.h"
#pragma hdrstop

using namespace std;
using namespace Dim;


/****************************************************************************
*
*   HostAddr
*
***/

//===========================================================================
size_t std::hash<HostAddr>::operator()(const HostAddr & val) const {
    size_t out = 0;
    hashCombine(&out, std::hash<int32_t>{}(val.data[0]));
    hashCombine(&out, std::hash<int32_t>{}(val.data[1]));
    hashCombine(&out, std::hash<int32_t>{}(val.data[2]));
    hashCombine(&out, std::hash<int32_t>{}(val.data[3]));
    static_assert(sizeof val.data == 4 * sizeof int32_t);
    return out;
}

//===========================================================================
bool HostAddr::isIpv4() const {
    return data[2] == kIpv4MappedAddress;
}

//===========================================================================
uint32_t HostAddr::getIpv4() const {
    return data[3];
}

//===========================================================================
HostAddr::operator bool() const {
    return data[3]
        || data[0]
        || data[1]
        || data[2] && data[2] != kIpv4MappedAddress;
}

//===========================================================================
namespace Dim {
istream & operator>>(istream & in, HostAddr & out) {
    string tmp;
    in >> tmp;
    if (!parse(&out, tmp.c_str()))
        in.setstate(ios_base::failbit);
    return in;
}
} // namespace

//===========================================================================
bool Dim::parse(HostAddr * out, string_view src) {
    SockAddr sa;
    if (!parse(&sa, src, 9)) {
        *out = {};
        return false;
    }
    *out = sa.addr;
    return true;
}

//===========================================================================
namespace Dim {
ostream & operator<<(ostream & os, const HostAddr & addr);
}
ostream & Dim::operator<<(ostream & os, const HostAddr & addr) {
    SockAddr sa;
    sa.addr = addr;
    return operator<<(os, sa);
}


/****************************************************************************
*
*   SockAddr
*
***/

//===========================================================================
size_t std::hash<SockAddr>::operator()(const SockAddr & val) const {
    size_t out = 0;
    hashCombine(&out, std::hash<HostAddr>{}(val.addr));
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
*   SubnetAddr
*
***/

//===========================================================================
size_t std::hash<SubnetAddr>::operator()(const SubnetAddr & val) const {
    size_t out = 0;
    hashCombine(&out, std::hash<HostAddr>{}(val.addr));
    hashCombine(&out, std::hash<int>{}(val.mask));
    return out;
}
