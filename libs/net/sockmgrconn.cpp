// Copyright Glen Knowles 2017.
// Distributed under the Boost Software License, Version 1.0.
//
// sockmgrconn.cpp - dim net
#include "pch.h"
#pragma hdrstop

using namespace std;
using namespace Dim;


/****************************************************************************
*
*   Tuning parameters
*
***/

const auto kDefaultPingInterval = 30s;

const auto kReconnectInterval = 5s;


/****************************************************************************
*
*   Declarations
*
***/

namespace {

class ConnMgrSocket;

class ConnectManager final : public ISockMgrBase {
public:
    ConnectManager(
        string_view name,
        IFactory<IAppSocketNotify> * fact,
        AppSocket::MgrFlags flags
    );

    void connect(const Endpoint & addr);
    void connect(ConnMgrSocket & sock);
    void shutdown(const Endpoint & addr);
    void destroy(ConnMgrSocket & sock);

    // Inherited via ISockMgrBase
    bool listening() const override { return false; }
    void setEndpoints(const vector<Endpoint> & endpts) override;
    bool onShutdown(bool firstTry) override;

    // Inherited via IConfigNotify
    void onConfigChange(const XDocument & doc) override;

private:
    TimerList<ISockMgrSocket> m_unconnected;
    unordered_map<Endpoint, ConnMgrSocket> m_sockets;
};

class ConnMgrSocket final : public ISockMgrSocket {
public:
    ConnMgrSocket(
        ISockMgrBase & mgr,
        unique_ptr<IAppSocketNotify> notify,
        const Endpoint & addr
    );

    const Endpoint & targetAddress() const { return m_targetAddress; }

    // Inherited via ISockMgrSocket
    ConnectManager & mgr() override;
    void shutdown() override;

    // Inherited via ITimerListNotify
    void onTimer(TimePoint now) override;

    // Inherited via IAppSocketNotify
    void onSocketConnect(const AppSocketInfo & info) override;
    void onSocketConnectFailed() override;
    void onSocketDisconnect() override;
    void onSocketDestroy() override;

private:
    Endpoint m_targetAddress;
    bool m_connected{false};
};

} // namespace


/****************************************************************************
*
*   Variables
*
***/


/****************************************************************************
*
*   ConnMgrSocket
*
***/

//===========================================================================
ConnMgrSocket::ConnMgrSocket(
    ISockMgrBase & mgr,
    unique_ptr<IAppSocketNotify> notify,
    const Endpoint & addr
)
    : ISockMgrSocket{mgr, move(notify)}
    , m_targetAddress{addr}
{}

//===========================================================================
void ConnMgrSocket::shutdown() {
    m_mode = kRunStopping;
    disconnect();
}

//===========================================================================
void ConnMgrSocket::onTimer(TimePoint now) {
    if (m_mode != kRunStarting)
        return disconnect();

    if (m_connected) {
        m_mode = kRunRunning;
        mgr().touch(this);
    } else {
        mgr().connect(*this);
    }
}

//===========================================================================
void ConnMgrSocket::onSocketConnect (const AppSocketInfo & info) {
    assert(!m_connected);
    m_connected = true;
    if (m_mode != kRunStarting)
        mgr().touch(this);
    notifyConnect(info);
}

//===========================================================================
void ConnMgrSocket::onSocketConnectFailed () {
    notifyConnectFailed();
}

//===========================================================================
void ConnMgrSocket::onSocketDisconnect () {
    assert(m_connected);
    m_connected = false;
    ISockMgrSocket::onSocketDisconnect();
}

//===========================================================================
void ConnMgrSocket::onSocketDestroy () {
    mgr().destroy(*this);
}

//===========================================================================
ConnectManager & ConnMgrSocket::mgr() { 
    return static_cast<ConnectManager &>(ISockMgrSocket::mgr()); 
}


/****************************************************************************
*
*   ConnectManager
*
***/

//===========================================================================
ConnectManager::ConnectManager(
    string_view name,
    IFactory<IAppSocketNotify> * fact,
    AppSocket::MgrFlags flags
)
    : ISockMgrBase(
        name, 
        fact, 
        kDefaultPingInterval, 
        AppSocket::kInvalid, 
        flags
    )
    , m_unconnected{kReconnectInterval}
{
    init();
}

//===========================================================================
void ConnectManager::connect(const Endpoint & addr) {
    auto ib = m_sockets.try_emplace(
        addr, 
        *this, 
        m_cliSockFact->onFactoryCreate(),
        addr
    );
    connect(ib.first->second);
}

//===========================================================================
void ConnectManager::connect(ConnMgrSocket & sock) {
    m_mode = kRunStarting;
    m_unconnected.touch(&sock);
    socketConnect(&sock, sock.targetAddress(), {});
}

//===========================================================================
void ConnectManager::shutdown(const Endpoint & addr) {
    auto it = m_sockets.find(addr);
    if (it != m_sockets.end())
        it->second.shutdown();
}

//===========================================================================
void ConnectManager::destroy(ConnMgrSocket & sock) {
    switch (sock.mode()) {
    case kRunStarting:
        return;
    case kRunRunning:
        return connect(sock);
    }
    assert(sock.mode() == kRunStopping);
    sock.notifyDestroy(false);
    [[maybe_unused]] auto num = m_sockets.erase(sock.targetAddress());
    assert(num);
}

//===========================================================================
void ConnectManager::setEndpoints(const vector<Endpoint> & src) {
    vector<Endpoint> endpts{src.begin(), src.end()};
    sort(endpts.begin(), endpts.end());
    sort(m_endpoints.begin(), m_endpoints.end());

    vector<Endpoint> removed;
    for_each_diff(
        endpts.begin(), endpts.end(),
        m_endpoints.begin(), m_endpoints.end(),
        [&](const Endpoint & ep){ connect(ep); },
        [&](const Endpoint & ep){ shutdown(ep); }
    );

    m_endpoints = move(endpts);
}

//===========================================================================
bool ConnectManager::onShutdown(bool firstTry) {
    if (firstTry) {
        for (auto && sock : m_unconnected.values())
            sock.shutdown();
    }
    return m_unconnected.values().empty();
}

//===========================================================================
void ConnectManager::onConfigChange(const XDocument & doc) {
    auto flags = AppSocket::ConfFlags{};
    if (configUnsigned(doc, "DisableInactiveTimeout"))
        flags |= AppSocket::fDisableInactiveTimeout;
    m_confFlags = flags;
}


/****************************************************************************
*
*   Public API
*
***/

//===========================================================================
SockMgrHandle Dim::sockMgrConnect(
    string_view mgrName,
    IFactory<IAppSocketNotify> * factory,
    AppSocket::MgrFlags flags
) {
    auto mgr = new ConnectManager(mgrName, factory, flags);
    return iSockMgrAdd(mgr);
}
