// Copyright Glen Knowles 2017 - 2023.
// Distributed under the Boost Software License, Version 1.0.
//
// sockmgracc.cpp - dim net
#include "pch.h"
#pragma hdrstop

using namespace std;
using namespace Dim;


/****************************************************************************
*
*   Tuning parameters
*
***/

constexpr Duration kDefaultInactiveTimeout = 1min;


/****************************************************************************
*
*   Declarations
*
***/

namespace {

class AcceptManager final
    : public ISockMgrBase
    , public IFactory<IAppSocketNotify>
{
public:
    AcceptManager(
        string_view name,
        IFactory<IAppSocketNotify> * fact,
        AppSocket::Family fam,
        AppSocket::MgrFlags flags
    );

    // Inherited via ISockMgrBase
    bool listening() const override { return true; }

    void setAddresses(const SockAddr * addrs, size_t count) override;
    bool onShutdown(bool firstTry) override;

    // Inherited via IConfigNotify via ISockMgrBase
    void onConfigChange(const XDocument & doc) override;

    // Inherited via IFactory<ISockMgrSocket>
    unique_ptr<IAppSocketNotify> onFactoryCreate() override;
};

class AccMgrSocket final : public ISockMgrSocket {
public:
    AccMgrSocket(
        ISockMgrBase & mgr,
        unique_ptr<IAppSocketNotify> notify
    );

    // Inherited via ISockMgrSocket
    AcceptManager & mgr() override;
    void disconnect(AppSocket::Disconnect why) override;

    // Inherited via ITimerListNotify
    void onTimer(TimePoint now) override;

    // Inherited via IAppSocketNotify
    bool onSocketAccept(const AppSocketConnectInfo & info) override;
    void onSocketDestroy() override;
    bool onSocketRead(AppSocketData & data) override;
};

} // namespace


/****************************************************************************
*
*   AccMgrSocket
*
***/

//===========================================================================
AccMgrSocket::AccMgrSocket(
    ISockMgrBase & mgr,
    unique_ptr<IAppSocketNotify> notify
)
    : ISockMgrSocket{mgr, move(notify)}
{}

//===========================================================================
AcceptManager & AccMgrSocket::mgr() {
    return static_cast<AcceptManager &>(ISockMgrSocket::mgr());
}

//===========================================================================
void AccMgrSocket::disconnect(AppSocket::Disconnect why) {
    socketDisconnect(this, why);
}

//===========================================================================
void AccMgrSocket::onTimer(TimePoint now) {
    disconnect(AppSocket::Disconnect::kInactivity);
}

//===========================================================================
bool AccMgrSocket::onSocketAccept (const AppSocketConnectInfo & info) {
    if (notifyAccept(info)) {
        mgr().touch(this);
        return true;
    }
    return false;
}

//===========================================================================
void AccMgrSocket::onSocketDestroy () {
    notifyDestroy();
}

//===========================================================================
bool AccMgrSocket::onSocketRead (AppSocketData & data) {
    mgr().touch(this);
    return ISockMgrSocket::onSocketRead(data);
}


/****************************************************************************
*
*   AcceptManager
*
***/

//===========================================================================
AcceptManager::AcceptManager(
    string_view name,
    IFactory<IAppSocketNotify> * fact,
    AppSocket::Family fam,
    AppSocket::MgrFlags flags
)
    : ISockMgrBase(name, fact, kDefaultInactiveTimeout, fam, flags)
{
    init();
}

//===========================================================================
void AcceptManager::setAddresses(const SockAddr * addrs, size_t count) {
    vector<SockAddr> saddrs{addrs, addrs + count};
    ranges::sort(saddrs);
    ranges::sort(m_addrs);

    auto fam = m_family;
    bool console = m_mgrFlags.any(AppSocket::fMgrConsole);
    for_each_diff(
        saddrs.begin(), saddrs.end(),
        m_addrs.begin(), m_addrs.end(),
        [&](auto & addr){ socketListen(this, addr, fam, console); },
        [&](auto & addr){ socketCloseWait(this, addr, fam); }
    );

    m_addrs = move(saddrs);
}

//===========================================================================
bool AcceptManager::onShutdown(bool firstTry) {
    if (firstTry) {
        for (auto&& addr : m_addrs)
            socketCloseWait(this, addr, m_family);
        for (auto&& sock : m_inactivity.values())
            sock.disconnect(AppSocket::Disconnect::kAppRequest);
    }
    return !m_inactivity.values();
}

//===========================================================================
void AcceptManager::onConfigChange(const XDocument & doc) {
    EnumFlags flags = AppSocket::ConfFlags{};
    if (configNumber(doc, "DisableInactiveTimeout"))
        flags |= AppSocket::fDisableInactiveTimeout;

    m_confFlags = flags;

    SockAddr addr;
    addr.port = (unsigned) configNumber(doc, "Port", 41000);
    setAddresses(&addr, 1);
}

//===========================================================================
unique_ptr<IAppSocketNotify> AcceptManager::onFactoryCreate() {
    auto ptr = make_unique<AccMgrSocket>(
        *this,
        m_cliSockFact->onFactoryCreate()
    );
    return ptr;
}


/****************************************************************************
*
*   Public API
*
***/

//===========================================================================
SockMgrHandle Dim::sockMgrListen(
    string_view mgrName,
    IFactory<IAppSocketNotify> * factory,
    /* security requirements, */
    AppSocket::Family fam,
    AppSocket::MgrFlags flags
) {
    auto mgr = NEW(AcceptManager)(mgrName, factory, fam, flags);
    return iSockMgrAdd(mgr);
}
