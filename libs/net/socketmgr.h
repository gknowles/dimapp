// Copyright Glen Knowles 2017.
// Distributed under the Boost Software License, Version 1.0.
//
// socketmgr.h - dim net
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

struct SocketMgrHandle : HandleBase {};

SocketMgrHandle sockMgrListen(
    std::string_view mgrName,
    IFactory<IAppSocketNotify> * factory,
    /* security requirements, */
    AppSocket::Family fam,
    AppSocket::MgrFlags flags = (AppSocket::MgrFlags) 0
);

SocketMgrHandle sockMgrConnect(
    std::string_view mgrName,
    IFactory<IAppSocketNotify> * factory,
    AppSocket::Family fam,
    AppSocket::MgrFlags flags = (AppSocket::MgrFlags) 0
);

void endpointMonitor(SocketMgrHandle mgr, std::string_view nodeName);

} // namespace
