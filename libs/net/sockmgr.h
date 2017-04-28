// Copyright Glen Knowles 2017.
// Distributed under the Boost Software License, Version 1.0.
//
// sockmgr.h - dim net
#pragma once

#include "cppconf/cppconf.h"

#include "core/handle.h"
#include "net/address.h"
#include "net/appsocket.h"

#include <string_view>

namespace Dim {

namespace AppSocket {
enum MgrFlags : unsigned {
};
} // namespace

struct SockMgrHandle : HandleBase {};

SockMgrHandle sockMgrListen(
    std::string_view mgrName,
    IFactory<IAppSocketNotify> * factory,
    /* security requirements, */
    AppSocket::Family fam,
    AppSocket::MgrFlags flags = {}
);

SockMgrHandle sockMgrConnect(
    std::string_view mgrName,
    IFactory<IAppSocketNotify> * factory,
    AppSocket::Family fam,
    AppSocket::MgrFlags flags = {}
);

void sockMgrMonitorEndpoints(SockMgrHandle mgr, std::string_view host);

// Starts closing, no new connections will be allowed. Returns true if all 
// sockets are closed. May be called multiple times. After shutdown has 
// completed you must still call sockMgrCloseWait().
bool sockMgrShutdown(SockMgrHandle mgr);

// Closes all sockets and destroys the manager.
void sockMgrCloseWait(SockMgrHandle mgr);

} // namespace
