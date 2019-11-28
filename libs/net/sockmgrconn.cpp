// Copyright Glen Knowles 2017 - 2019.
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
struct RecentLink;

class ConnectManager final : public ISockMgrBase {
public:
    ConnectManager(
        string_view name,
        IFactory<IAppSocketNotify> * fact,
        AppSocket::MgrFlags flags
    );

    void connect(const SockAddr & addr);
    void connect(ConnMgrSocket & sock);
    void stable(ConnMgrSocket & sock);
    void shutdown(const SockAddr & addr);
    void destroy(ConnMgrSocket & sock);

    // Inherited via ISockMgrBase
    bool listening() const override { return false; }
    void setAddresses(const SockAddr * addrs, size_t count) override;
    bool onShutdown(bool firstTry) override;

    // Inherited via IConfigNotify
    void onConfigChange(const XDocument & doc) override;

private:
    TimerList<ConnMgrSocket, RecentLink> m_recent;
    unordered_map<SockAddr, ConnMgrSocket> m_sockets;
};

class ConnMgrSocket final
    : public ISockMgrSocket
    , public ITimerListNotify<RecentLink>
{
public:
    enum Mode {
        kUnconnected,
        kConnecting,
        kConnected,
        kClosing,
    };

public:
    ConnMgrSocket(
        ISockMgrBase & mgr,
        unique_ptr<IAppSocketNotify> notify,
        const SockAddr & addr
    );

    Mode mode() const { return m_mode; }
    bool stopping() const { return m_stopping; }
    const SockAddr & targetAddress() const { return m_targetAddress; }
    void connect();
    void shutdown();

    // Inherited via ISockMgrSocket
    ConnectManager & mgr() override;

    // Inherited via ITimerListNotify
    void onTimer(TimePoint now) override;
    void onTimer(TimePoint now, RecentLink*) override;

    // Inherited via IAppSocket
    void disconnect(AppSocket::Disconnect why) override;
    void write(string_view data) override;
    void write(unique_ptr<SocketBuffer> buffer, size_t bytes) override;

    // Inherited via IAppSocketNotify
    void onSocketConnect(const AppSocketInfo & info) override;
    void onSocketConnectFailed() override;
    void onSocketDisconnect() override;
    void onSocketDestroy() override;

private:
    SockAddr m_targetAddress;
    Mode m_mode{kUnconnected};
    bool m_stopping{false};
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
    const SockAddr & addr
)
    : ISockMgrSocket{mgr, move(notify)}
    , m_targetAddress{addr}
{}

//===========================================================================
void ConnMgrSocket::shutdown() {
    m_stopping = true;
    if (m_mode == kUnconnected) {
        onSocketDestroy();
    } else {
        disconnect(AppSocket::Disconnect::kAppRequest);
    }
}

//===========================================================================
void ConnMgrSocket::connect() {
    assert(m_mode == kUnconnected);
    m_mode = kConnecting;
    mgr().connect(*this);
}

//===========================================================================
// notify by inactivity timer list
void ConnMgrSocket::onTimer(TimePoint now) {
    notifyPingRequired();
}

//===========================================================================
// notify by recent timer list
void ConnMgrSocket::onTimer(TimePoint now, RecentLink*) {
    if (m_mode == kUnconnected) {
        connect();
    } else {
        mgr().stable(*this);
    }
}

//===========================================================================
void ConnMgrSocket::disconnect(AppSocket::Disconnect why) {
    if (m_mode == kConnecting || m_mode == kConnected)
        socketDisconnect(this, why);
}

//===========================================================================
void ConnMgrSocket::write(string_view data) {
    mgr().touch(this);
    ISockMgrSocket::write(data);
}

//===========================================================================
void ConnMgrSocket::write(unique_ptr<SocketBuffer> buffer, size_t bytes) {
    mgr().touch(this);
    ISockMgrSocket::write(move(buffer), bytes);
}

//===========================================================================
void ConnMgrSocket::onSocketConnect (const AppSocketInfo & info) {
    assert(m_mode == kConnecting);
    mgr().touch(this);
    m_mode = kConnected;
    notifyConnect(info);
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
    m_mode = kClosing;
    ISockMgrSocket::onSocketDisconnect();
}

//===========================================================================
void ConnMgrSocket::onSocketDestroy () {
    assert(m_mode == kClosing || m_mode == kUnconnected);
    m_mode = kUnconnected;
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
    , m_recent{kReconnectInterval, 0s}
{
    init();
}

//===========================================================================
void ConnectManager::connect(const SockAddr & addr) {
    auto ib = m_sockets.try_emplace(
        addr,
        *this,
        m_cliSockFact->onFactoryCreate(),
        addr
    );
    ib.first->second.connect();
}

//===========================================================================
void ConnectManager::connect(ConnMgrSocket & sock) {
    assert(sock.mode() == ConnMgrSocket::kConnecting);
    m_recent.touch(&sock);
    socketConnect(&sock, sock.targetAddress(), {});
}

//===========================================================================
void ConnectManager::stable(ConnMgrSocket & sock) {
    m_recent.unlink(&sock);
}

//===========================================================================
void ConnectManager::shutdown(const SockAddr & addr) {
    auto it = m_sockets.find(addr);
    if (it != m_sockets.end())
        it->second.shutdown();
}

//===========================================================================
void ConnectManager::destroy(ConnMgrSocket & sock) {
    assert(sock.mode() == ConnMgrSocket::kUnconnected);
    if (!sock.stopping()) {
        if (!m_recent.values().linked(&sock))
            sock.connect();
    } else {
        sock.notifyDestroy(false);
        if (auto num = m_sockets.erase(sock.targetAddress()); !num)
            logMsgFatal() << "ConnectManager::destroy(): socket not found";
    }
}

//===========================================================================
void ConnectManager::setAddresses(const SockAddr * addrs, size_t count) {
    vector<SockAddr> endpts{addrs, addrs + count};
    sort(endpts.begin(), endpts.end());
    sort(m_addrs.begin(), m_addrs.end());

    vector<SockAddr> removed;
    for_each_diff(
        endpts.begin(), endpts.end(),
        m_addrs.begin(), m_addrs.end(),
        [&](const SockAddr & ep){ connect(ep); },
        [&](const SockAddr & ep){ shutdown(ep); }
    );

    m_addrs = move(endpts);
}

//===========================================================================
bool ConnectManager::onShutdown(bool firstTry) {
    if (firstTry) {
        auto pos = m_sockets.begin(),
            epos = m_sockets.end();
        while (pos != epos) {
            auto next = std::next(pos);
            pos->second.shutdown();
            pos = next;
        }
    }
    return m_sockets.empty();
}

//===========================================================================
void ConnectManager::onConfigChange(const XDocument & doc) {
    auto flags = AppSocket::ConfFlags{};
    if (configNumber(doc, "DisableInactiveTimeout"))
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
