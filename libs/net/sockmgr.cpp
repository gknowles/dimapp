// Copyright Glen Knowles 2017 - 2022.
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
SocketInfo ISockMgrSocket::getInfo() const {
    return socketGetInfo(this);
}

//===========================================================================
void ISockMgrSocket::write(string_view data) {
    socketWrite(this, data);
}

//===========================================================================
void ISockMgrSocket::write(unique_ptr<SocketBuffer> buffer, size_t bytes) {
    socketWrite(this, move(buffer), bytes);
}

//===========================================================================
void ISockMgrSocket::read() {
    socketRead(this);
}

//===========================================================================
void ISockMgrSocket::onSocketDisconnect () {
    notifyDisconnect();
}

//===========================================================================
bool ISockMgrSocket::onSocketRead (AppSocketData & data) {
    return notifyRead(data);
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
    EnumFlags<AppSocket::MgrFlags> flags
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
    assert(!m_inactivity.values());
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

//===========================================================================
SockMgrInfo ISockMgrBase::info() const {
    SockMgrInfo out;
    out.name = m_name;
    out.family = m_family;
    out.inbound = listening();
    out.addrs = m_addrs;
    out.mgrFlags = m_mgrFlags;
    out.confFlags = m_confFlags;
    out.inactiveTimeout = m_inactivity.timeout();
    out.inactiveMinWait = m_inactivity.minWait();
    return out;
}

//===========================================================================
size_t Dim::ISockMgrBase::getSockInfos(
    vector<SocketInfo> * out,
    size_t limit
) const {
    size_t found = 0;
    for (auto&& sock : m_inactivity.values()) {
        if (limit) {
            limit -= 1;
            auto & info = out->emplace_back(sock.getInfo());
            info.dir = listening()
                ? SocketDir::kInbound
                : SocketDir::kOutbound;
        }
        found += 1;
    }
    return found;
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
    if (s_mgrs)
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
*   Conversions
*
***/

TokenTable s_mgrFlags({
    { AppSocket::MgrFlags::fMgrConsole, "console" },
});
TokenTable s_confFlags({
    { AppSocket::ConfFlags::fDisableInactiveTimeout, "disableTimeout" },
});

//===========================================================================
vector<string_view> Dim::toStrings(
    EnumFlags<AppSocket::MgrFlags> flags
) {
    return s_mgrFlags.findNames(flags);
}

//===========================================================================
vector<string_view> Dim::toStrings(
    EnumFlags<AppSocket::ConfFlags> flags
) {
    return s_confFlags.findNames(flags);
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
void Dim::sockMgrSetAddresses(
    SockMgrHandle h,
    const SockAddr * addrs,
    size_t count
) {
    auto mgr = s_mgrs.find(h);
    mgr->setAddresses(addrs, count);
}

//===========================================================================
//void Dim::sockMgrMonitorAddresses(SockMgrHandle h, string_view host) {
//    auto mgr = s_mgrs.find(h);
//    return mgr->monitorAddresses(host);
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

//===========================================================================
vector<SockMgrInfo> Dim::sockMgrGetInfos() {
    vector<SockMgrInfo> out;
    for (auto&& mgr : s_mgrs) {
        out.push_back(mgr.second->info());
        out.back().handle = mgr.first;
    }
    return out;
}

//===========================================================================
size_t Dim::sockMgrWriteInfo(
    IJBuilder * out,
    const SockMgrInfo & info,
    bool show,
    size_t limit
) {
    auto mgr = s_mgrs.find(info.handle);
    if (!mgr)
        return 0;
    out->object();
    out->member("name", info.name);
    out->member("family", info.family);
    out->member("inbound", info.inbound);
    out->member("addrs").array(info.addrs.begin(), info.addrs.end());
    out->member("mgrFlags").array(toStrings(info.mgrFlags));
    out->member("confFlags").array(toStrings(info.confFlags));
    out->member("inactiveTimeout", info.inactiveTimeout);
    out->member("inactiveMinWait", info.inactiveMinWait);
    out->member("show", show);
    vector<SocketInfo> socks;
    auto found = mgr->getSockInfos(&socks, limit);
    out->member("socketCount", found);
    out->member("sockets").array();
    for (auto&& sock: socks) {
        out->object();
        if (sock.dir == SocketDir::kInbound) {
            out->member("inbound", true);
        } else if (sock.dir == SocketDir::kOutbound) {
            out->member("inbound", false);
        }
        out->member("mode", toString(sock.mode))
            .member("lastModeTime", sock.lastModeTime)
            .member("localAddr", sock.local)
            .member("remoteAddr", sock.remote);
        if (!empty(sock.lastReadTime))
            out->member("lastReadTime", sock.lastReadTime);
        if (!empty(sock.lastWriteTime))
            out->member("lastWriteTime", sock.lastWriteTime);
        out->end();
    }
    out->end();
    out->end();
    return found;
}
