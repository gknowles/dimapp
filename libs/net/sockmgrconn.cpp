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
    TimerList<ISockMgrSocket> m_recent;
    unordered_map<Endpoint, ConnMgrSocket> m_sockets;
};

class ConnMgrSocket final : public ISockMgrSocket {
public:
    enum Mode { 
        kUnconnected = 0x02,
        kConnecting = 0x04,
        kConnected = 0x06,
        kStopping = 0x08,
    };

public:
    ConnMgrSocket(
        ISockMgrBase & mgr,
        unique_ptr<IAppSocketNotify> notify,
        const Endpoint & addr
    );

    bool recentConnect() const { return m_recentConnect; }
    Mode mode() const { return m_mode; }
    const Endpoint & targetAddress() const { return m_targetAddress; }
    void connect();

    // Inherited via ISockMgrSocket
    ConnectManager & mgr() override;
    void shutdown() override;

    // Inherited via ITimerListNotify
    void onTimer(TimePoint now) override;

    // Inherited via IAppSocket
    void write(string_view data) override;
    void write(unique_ptr<SocketBuffer> buffer, size_t bytes) override;

    // Inherited via IAppSocketNotify
    void onSocketConnect(const AppSocketInfo & info) override;
    void onSocketConnectFailed() override;
    void onSocketDisconnect() override;
    void onSocketDestroy() override;

private:
    Endpoint m_targetAddress;
    Mode m_mode{kUnconnected};
    bool m_recentConnect{false};
};

} // namespace


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
    disconnect();
}

//===========================================================================
void ConnMgrSocket::connect() {
    assert(m_mode == kUnconnected);
    m_mode = kConnecting;
    mgr().connect(*this);
    m_recentConnect = true;
}

//===========================================================================
void ConnMgrSocket::onTimer(TimePoint now) {
    switch (m_mode) {
    case kUnconnected:
        m_recentConnect = false;
        connect();
        break;
    case kConnecting:
        m_recentConnect = false;
        mgr().touch(this);
        break;
    case kConnected:
        if (m_recentConnect) {
            m_recentConnect = false;
            mgr().touch(this);
        }
        notifyPingRequired();
        break;
    }
}

//===========================================================================
void ConnMgrSocket::write(string_view data) {
    if (!m_recentConnect)
        mgr().touch(this);
    ISockMgrSocket::write(data);
}

//===========================================================================
void ConnMgrSocket::write(unique_ptr<SocketBuffer> buffer, size_t bytes) {
    if (!m_recentConnect)
        mgr().touch(this);
    ISockMgrSocket::write(move(buffer), bytes);
}

//===========================================================================
void ConnMgrSocket::onSocketConnect (const AppSocketInfo & info) {
    assert(m_mode == kConnecting);
    if (!m_recentConnect)
        mgr().touch(this);
    notifyConnect(info);
    m_mode = kConnected;
}

//===========================================================================
void ConnMgrSocket::onSocketConnectFailed () {
    assert(m_mode == kConnecting);
    m_mode = kUnconnected;
    notifyConnectFailed();
}

//===========================================================================
void ConnMgrSocket::onSocketDisconnect () {
    assert(m_mode == kConnected);
    m_mode = kUnconnected;
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
    , m_recent{kReconnectInterval}
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
    assert(sock.mode() == ConnMgrSocket::kConnecting);
    m_recent.touch(&sock);
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
    case ConnMgrSocket::kUnconnected:
        return sock.connect();
    case ConnMgrSocket::kStopping:
        break;
    default:
        return;
    }
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
        for (auto && sock : m_recent.values())
            sock.shutdown();
    }
    return m_recent.values().empty();
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
