// Copyright Glen Knowles 2017.
// Distributed under the Boost Software License, Version 1.0.
//
// sockmgr.cpp - dim net
#include "pch.h"
#pragma hdrstop

using namespace std;
using namespace Dim;


/****************************************************************************
*
*   Declarations
*
***/

namespace {

class MgrSocket : public IAppSocketNotify {
};

class SocketManager {
public:
private:
};

} // namespace


/****************************************************************************
*
*   Variables
*
***/

static HandleMap<SockMgrHandle, SocketManager> s_mgrs;


/****************************************************************************
*
*   TlsSocket
*
***/


/****************************************************************************
*
*   Shutdown monitor
*
***/

namespace {
class ShutdownNotify : public IShutdownNotify {
    void onShutdownClient(bool firstTry) override;
};
} // namespace
static ShutdownNotify s_cleanup;

//===========================================================================
void ShutdownNotify::onShutdownClient(bool firstTry) {
}


/****************************************************************************
*
*   Internal API
*
***/

//===========================================================================
void Dim::iSockMgrInitialize() {
    shutdownMonitor(&s_cleanup);
}


/****************************************************************************
*
*   Public API
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
