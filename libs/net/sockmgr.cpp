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
*   Variables
*
***/

static HandleMap<SockMgrHandle, ISockMgrBase> s_mgrs;


/****************************************************************************
*
*   ISockMgrSocket
*
***/

//===========================================================================
ISockMgrSocket::ISockMgrSocket(
    ISockMgrBase & mgr, 
    unique_ptr<IAppSocketNotify> notify
)
    : IAppSocket(notify.release())
    , m_mgr(mgr)
{}

//===========================================================================
void ISockMgrSocket::write(string_view data) {
    socketWrite(this, data);
}

//===========================================================================
void ISockMgrSocket::write(unique_ptr<SocketBuffer> buffer, size_t bytes) {
    socketWrite(this, move(buffer), bytes);
}

//===========================================================================
void ISockMgrSocket::onSocketDisconnect () {
    notifyDisconnect();
}

//===========================================================================
void ISockMgrSocket::onSocketRead (AppSocketData & data) {
    notifyRead(data);
}

//===========================================================================
void ISockMgrSocket::onSocketBufferChanged(const AppSocketBufferInfo & info) {
    notifyBufferChanged(info);
}


/****************************************************************************
*
*   ISockMgrBase
*
***/

//===========================================================================
ISockMgrBase::ISockMgrBase(
    string_view name,
    IFactory<IAppSocketNotify> * fact,
    Duration inactiveTimeout,
    AppSocket::Family fam,
    AppSocket::MgrFlags flags
) 
    : m_name{name}
    , m_cliSockFact{fact}
    , m_family{fam}
    , m_mgrFlags{flags}
    , m_inactivity{inactiveTimeout}
{}

//===========================================================================
void ISockMgrBase::init() {
    configMonitor("app.xml", this);
}

//===========================================================================
ISockMgrBase::~ISockMgrBase() {
    assert(m_inactivity.values().empty());
}

//===========================================================================
void ISockMgrBase::touch(ISockMgrSocket * sock) {
    m_inactivity.touch(sock);
}

//===========================================================================
void ISockMgrBase::unlink(ISockMgrSocket * sock) {
    m_inactivity.unlink(sock);
}

//===========================================================================
void ISockMgrBase::setInactiveTimeout(Duration timeout) {
    m_inactivity.setTimeout(timeout);
}

//===========================================================================
bool ISockMgrBase::shutdown() {
    bool firstTry = (m_mode == kRunRunning);

    m_mode = kRunStopping;
    if (onShutdown(firstTry)) {
        m_mode = kRunStopped;
        return true;
    }
    return false;
}


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
    for (auto mgr : s_mgrs) {
        if (mgr.second->shutdown()) 
            s_mgrs.erase(mgr.first);
    }
    if (!s_mgrs.empty())
        return shutdownIncomplete();
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

//===========================================================================
SockMgrHandle Dim::iSockMgrAdd(ISockMgrBase * mgr) {
    return s_mgrs.insert(mgr);
}


/****************************************************************************
*
*   Public API
*
***/

//===========================================================================
void Dim::sockMgrSetInactiveTimeout(SockMgrHandle h, Duration timeout) {
    auto mgr = s_mgrs.find(h);
    mgr->setInactiveTimeout(timeout);
}

//===========================================================================
void Dim::sockMgrSetEndpoints(
    SockMgrHandle h, 
    const vector<Endpoint> & addrs
) {
    auto mgr = s_mgrs.find(h);
    mgr->setEndpoints(addrs);
}

//===========================================================================
//void Dim::sockMgrMonitorEndpoints(SockMgrHandle h, string_view host) {
//    auto mgr = s_mgrs.find(h);
//    return mgr->monitorEndpoints(host);
//}

//===========================================================================
bool Dim::sockMgrShutdown(SockMgrHandle h) {
    auto mgr = s_mgrs.find(h);
    return mgr->shutdown();
}

//===========================================================================
void Dim::sockMgrDestroy(SockMgrHandle h) {
    if (auto mgr = s_mgrs.find(h)) {
        assert(mgr->shutdown());
        s_mgrs.erase(h);
    }
}
