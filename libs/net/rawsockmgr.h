// Copyright Glen Knowles 2017.
// Distributed under the Boost Software License, Version 1.0.
//
// rawsockmgr.h - dim net
#pragma once

#include "cppconf/cppconf.h"

#include "net/address.h"
#include "net/appsocket.h"

#include <string_view>

namespace Dim {

namespace AppSocket {
enum MgrFlags : unsigned {
};
} // namespace

struct RawSocketMgrHandle : HandleBase {};

RawSocketMgrHandle rawSockMgrListen(
    std::string_view mgrName,
    IFactory<IAppSocketNotify> * factory,
    /* security requirements, */
    AppSocket::Family fam,
    AppSocket::MgrFlags flags = {}
);

RawSocketMgrHandle rawSockMgrConnect(
    std::string_view mgrName,
    IFactory<IAppSocketNotify> * factory,
    AppSocket::Family fam,
    AppSocket::MgrFlags flags = {}
);

void endpointMonitor(RawSocketMgrHandle mgr, std::string_view nodeName);

} // namespace
