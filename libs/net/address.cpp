// Copyright Glen Knowles 2015 - 2021.
// Distributed under the Boost Software License, Version 1.0.
//
// address.cpp - dim net
#include "pch.h"
#pragma hdrstop

using namespace std;
using namespace Dim;


/****************************************************************************
*
*   Declarations
*
***/

const uint8_t kIpv4Prefix[] = {
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0xff, 0xff
};
const HostAddr kLoopback6 = {
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 1,
};

namespace Dim {
istream & operator>>(istream & in, HostAddr & out);
ostream & operator<<(ostream & os, const HostAddr & addr);

istream & operator>>(istream & in, SockAddr & out);
ostream & operator<<(ostream & os, const SockAddr & src);

istream & operator>>(istream & in, SubnetAddr & out);
ostream & operator<<(ostream & os, const SubnetAddr & src);
} // namespace


/****************************************************************************
*
*   HostAddr
*
***/

//===========================================================================
size_t std::hash<HostAddr>::operator()(const HostAddr & val) const {
    size_t out = 0;
    hashCombine(&out, hash<string_view>()({
        (const char *) val.data,
        size(val.data)
    }));
    hashCombine(&out, std::hash<uint32_t>{}(val.scope));
    return out;
}

//===========================================================================
HostAddr::Family HostAddr::family() const {
    if (*(uint64_t *) data == 0) {
        auto addr = ntoh64(data + 8);
        if (!addr) {
            return kUnspec;
        } else if ((addr & 0xffff'ffff'0000'0000) == 0x0000'ffff'0000'0000) {
            return kIpv4;
        }
    }
    return kIpv6;
}

//===========================================================================
HostAddr::Type HostAddr::type() const {
    if (!*this) // 0.0.0.0
        return kUnspecified;
    if (family() == kIpv4) {
        auto addr = ipv4();
        if ((addr & 0xff00'0000) == 0x7f00'0000) {        // 127.0.0.0/8
            return kLoopback;
        } else if ((addr & 0xf000'0000) == 0xe000'0000) { // 224.0.0.0/4
            return kMulticast;
        } else if ((addr & 0xffff'0000) == 0xa9fe'0000) { // 169.254.0.0/16
            return kLinkLocal;
        } else if ((addr & 0xff00'0000) == 0x0a00'0000    // 10.0.0.0/8
            || (addr & 0xfff0'0000) == 0xac10'0000        // 172.16.0.0/12
            || (addr & 0xffff'0000) == 0xc0a8'0000        // 192.168.0.0/16
            ) {
            return kPrivate;
        }
    } else {
        if (*this == kLoopback6) {      // ::1/128
            return kLoopback;
        } else if (data[0] == 0xff) {   // ff00::/8
            return kMulticast;
        } else if (data[0] == 0xfe && (data[1] & 0x80) == 0x80) { // fe80::/10
            return kLinkLocal;
        } else if ((data[0] & 0xfe) == 0xfc) {    // fc00::/7
            return kPrivate;
        }
    }
    return kPublic;
}

//===========================================================================
uint32_t HostAddr::ipv4() const {
    assert(family() != kIpv6);
    return ntoh32(this->data + 12);
}

//===========================================================================
uint32_t HostAddr::ipv4(uint32_t addr) {
    *(uint64_t *) this->data = 0;
    uint64_t val = addr;
    if (val)
        val |= 0x0000'ffff'0000'0000;
    hton64(this->data + 8, val);
    this->scope = 0;
    return addr;
}

//===========================================================================
HostAddr::operator bool() const {
    return *this != HostAddr{};
}

//===========================================================================
istream & Dim::operator>>(istream & in, HostAddr & out) {
    string tmp;
    in >> tmp;
    if (!parse(&out, tmp.c_str()))
        in.setstate(ios_base::failbit);
    return in;
}

//===========================================================================
// Writes word in lowercase hex with no leading zeros.
static ostream & writeHex(ostream & os, uint16_t word) {
    switch (countl_zero(word) / 4) {
    case 0:
        os << hexFromNibble(word >> 12);
        [[fallthrough]];
    case 1:
        os << hexFromNibble((word >> 8) & 15);
        [[fallthrough]];
    case 2:
        os << hexFromNibble((word >> 4) & 15);
        [[fallthrough]];
    case 3:
        os << hexFromNibble(word % 15);
        break;
    case 4:
        os << '0';
        break;
    }
    return os;
}

//===========================================================================
ostream & Dim::operator<<(ostream & os, const HostAddr & addr) {
    if (addr.family() == HostAddr::kIpv4) {
        auto s = addr.data;
        os << (int) s[12] << '.' << (int) s[13] << '.'
            << (int) s[14] << '.' << (int) s[15];
        return os;
    }

    auto word = [&addr](size_t pos) {
        return ntoh16(addr.data + 2 * pos);
    };

    // Find largest range of words that are 0's.
    int cmpStart = 8;
    int cmpLen = 1;
    auto first = 0;
    while (first < 8) {
        if (word(first)) {
            first += 1;
            continue;
        }
        auto last = first + 1;
        for (; last < 8; ++last) {
            if (word(last))
                break;
        }
        if (last - first > cmpLen) {
            cmpStart = first;
            cmpLen = last - first;
        }
        first = last;
    }

    auto pos = 0;
    if (cmpStart > 0) {
        writeHex(os, word(pos));
        for (pos += 1; pos < cmpStart; ++pos) {
            os << ':';
            writeHex(os, word(pos));
        }
    }
    if (cmpStart < 8) {
        os << ':';
        if (cmpLen == 8) {
            os << ':';
        } else {
            for (pos += cmpLen; pos < 8; ++pos) {
                os << ':';
                writeHex(os, word(pos));
            }
        }
    }
    if (addr.scope)
        os << '%' << addr.scope;
    return os;
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

//===========================================================================
ostream & Dim::operator<<(ostream & os, const SockAddr & src) {
    if (src.addr.family() == HostAddr::kIpv4) {
        os << src.addr;
    } else {
        os << '[' << src.addr << ']';
    }
    if (src.port)
        os << ':' << src.port;
    return os;
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
    hashCombine(&out, std::hash<int>{}(val.prefixLen));
    return out;
}
