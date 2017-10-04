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
    fMgrConsole = 0x01, // console connections (for server monitoring)
};
} // namespace

struct SockMgrHandle : HandleBase {};


/****************************************************************************
*
*   Create socket manager
*
***/

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
    AppSocket::MgrFlags flags = {}
);

//===========================================================================
// Helpers to implicitly create factories for connect and listen. Templated
// on socket class derived from IAppSocketNotify.
//===========================================================================
template <typename S>
SockMgrHandle sockMgrListen(
    std::string_view mgrName,
    /* security requirements, */
    AppSocket::Family fam,
    AppSocket::MgrFlags flags = {}
) {
    static_assert(std::is_base_of_v<IAppSocketNotify, S>);
    auto factory = getFactory<IAppSocketNotify, S>();
    return sockMgrListen(mgrName, factory, fam, flags);
}

//===========================================================================
template <typename S>
SockMgrHandle sockMgrConnect(
    std::string_view mgrName,
    AppSocket::MgrFlags flags = {}
) {
    static_assert(std::is_base_of_v<IAppSocketNotify, S>);
    auto factory = getFactory<IAppSocketNotify, S>();
    return sockMgrConnect(mgrName, factory, flags);
}


/****************************************************************************
*
*   Access socket manager
*
***/

// Only the connecting side sends pings, and only the accepting side
// disconnects due to inactivity. Both can be set to kTimerInfinite, in 
// which case no pings are sent and inactivity causes no disconnects.
void sockMgrSetInactiveTimeout(
    SockMgrHandle mgr,
    Duration pingInterval,  // duration of inactivity that triggers a ping
    Duration pingTimeout    // inactivity that triggers a disconnect
);

// Not implemented
void sockMgrSetEndpoints(
    SockMgrHandle mgr, 
    const std::vector<Endpoint> & addrs
);
void sockMgrMonitorEndpoints(SockMgrHandle mgr, std::string_view host);

// Starts closing, no new connections will be allowed. Returns true if all 
// sockets are closed. May be called multiple times. After shutdown has 
// completed you must still call sockMgrDestroy().
bool sockMgrShutdown(SockMgrHandle mgr);

// Destroys the manager, all sockets must already be closed.
void sockMgrDestroy(SockMgrHandle mgr);

} // namespace
