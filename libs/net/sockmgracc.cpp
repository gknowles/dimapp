// Copyright Glen Knowles 2017 - 2019.
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
    bool onSocketAccept(const AppSocketInfo & info) override;
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
bool AccMgrSocket::onSocketAccept (const AppSocketInfo & info) {
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
    vector<SockAddr> endpts{addrs, addrs + count};
    sort(endpts.begin(), endpts.end());
    sort(m_addrs.begin(), m_addrs.end());

    bool console = (m_mgrFlags & AppSocket::fMgrConsole);
    for_each_diff(
        endpts.begin(), endpts.end(),
        m_addrs.begin(), m_addrs.end(),
        [&](const SockAddr & ep){ socketListen(this, ep, m_family, console); },
        [&](const SockAddr & ep){ socketCloseWait(this, ep, m_family); }
    );

    m_addrs = move(endpts);
}

//===========================================================================
bool AcceptManager::onShutdown(bool firstTry) {
    if (firstTry) {
        for (auto && ep : m_addrs)
            socketCloseWait(this, ep, m_family);
        for (auto && sock : m_inactivity.values())
            sock.disconnect(AppSocket::Disconnect::kAppRequest);
    }
    return !m_inactivity.values();
}

//===========================================================================
void AcceptManager::onConfigChange(const XDocument & doc) {
    auto flags = AppSocket::ConfFlags{};
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
    auto mgr = new AcceptManager(mgrName, factory, fam, flags);
    return iSockMgrAdd(mgr);
}
