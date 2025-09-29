// Copyright Glen Knowles 2017 - 2023.
// Distributed under the Boost Software License, Version 1.0.
//
// sockmgr.h - dim net
#pragma once

#include "cppconf/cppconf.h"

#include "basic/handle.h"
#include "net/address.h"
#include "net/appsocket.h"

#include <string_view>

namespace Dim {


/****************************************************************************
*
*   Declarations
*
***/

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
// on a socket class derived from IAppSocketNotify.
//===========================================================================
template <typename S>
requires std::derived_from<S, IAppSocketNotify>
SockMgrHandle sockMgrListen(
    std::string_view mgrName,
    /* security requirements, */
    AppSocket::Family fam,
    AppSocket::MgrFlags flags = {}
) {
    auto factory = getFactory<IAppSocketNotify, S>();
    return sockMgrListen(mgrName, factory, fam, flags);
}

//===========================================================================
template <typename S>
requires std::derived_from<S, IAppSocketNotify>
SockMgrHandle sockMgrConnect(
    std::string_view mgrName,
    AppSocket::MgrFlags flags = {}
) {
    auto factory = getFactory<IAppSocketNotify, S>();
    return sockMgrConnect(mgrName, factory, flags);
}


/****************************************************************************
*
*   Access socket manager
*
***/

// Inactivity causes the connecting side to send pings and the listening side
// to disconnect. It can be set to kTimerInfinite, in which case no pings are
// sent or disconnects initiated.
//
// Defaults for connectors and listeners are 30s and 1min respectively. The
// listeners timeout must be longer than the connectors so that pings can
// reliably stave off disconnection.
void sockMgrSetInactiveTimeout(SockMgrHandle mgr, Duration timeout);

void sockMgrSetAddresses(
    SockMgrHandle mgr,
    const SockAddr * addrs,
    size_t numAddrs
);
// Not implemented
void sockMgrMonitorAddresses(SockMgrHandle mgr, std::string_view host);

// Starts closing, no new connections will be allowed. Returns true if all
// sockets are closed. May be called multiple times. After shutdown has
// completed you must still call sockMgrDestroy().
bool sockMgrShutdown(SockMgrHandle mgr);

// Destroys the manager, all sockets must already be closed.
void sockMgrDestroy(SockMgrHandle mgr);


/****************************************************************************
*
*   Utilities
*
***/

namespace AppSocket {
    enum ConfFlags : unsigned;
}

std::vector<std::string_view> toStrings(EnumFlags<AppSocket::MgrFlags> flags);
std::vector<std::string_view> toStrings(EnumFlags<AppSocket::ConfFlags> flags);

struct SockMgrInfo {
    SockMgrHandle handle;
    std::string name;
    AppSocket::Family family;
    bool inbound;
    std::vector<SockAddr> addrs;
    EnumFlags<AppSocket::MgrFlags> mgrFlags;
    EnumFlags<AppSocket::ConfFlags> confFlags;
    Duration inactiveTimeout;
    Duration inactiveMinWait;
};
std::vector<SockMgrInfo> sockMgrGetInfos();

// Returns the number of sockets included.
size_t sockMgrWriteInfo(
    IJBuilder * out,
    const SockMgrInfo & info,
    bool active = false,
    size_t limit = 0        // Maximum number of connected sockets to include.
);

} // namespace
