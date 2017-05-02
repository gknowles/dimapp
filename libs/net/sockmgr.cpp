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
*   Tuning parameters
*
***/

const auto kDefaultPingInterval = 30s;
const auto kDefaultInactiveTimeout = 1min;


/****************************************************************************
*
*   Declarations
*
***/

namespace Dim::AppSocket {

enum ConfFlags : unsigned {
    fDisableInactiveTimeout = 0x01,
};

}

namespace {

class SocketManager;

class MgrSocket 
    : public ITimerListNotify<MgrSocket>
    , public IAppSocket
    , public IAppSocketNotify 
{
public:
    MgrSocket(SocketManager & mgr, unique_ptr<IAppSocketNotify> notify);
    ~MgrSocket();

    // Inherited via ITimerListNotify
    void onTimer(TimePoint now) override;

    // Inherited via IAppSocket
    void disconnect() override;
    void write(string_view data) override;
    void write(unique_ptr<SocketBuffer> buffer, size_t bytes) override;

    // Inherited via IAppSocketNotify
    bool onSocketAccept(const AppSocketInfo & info) override;
    void onSocketDisconnect() override;
    void onSocketDestroy() override;
    void onSocketRead(AppSocketData & data) override;

private:
    SocketManager & m_mgr;
};

class SocketManager 
    : public IFactory<IAppSocketNotify> 
    , public IConfigNotify
{
public:
    SocketManager(
        string_view name,
        IFactory<IAppSocketNotify> * fact,
        AppSocket::Family fam,
        AppSocket::MgrFlags flags
    );
    ~SocketManager();

    void setInactiveTimeout(Duration inactiveTimeout, Duration pingInterval);
    void touch(MgrSocket * sock);
    bool shutdown();

    // Inherited via IFactory<MgrSocket>
    unique_ptr<IAppSocketNotify> onFactoryCreate() override;

    // Inherited via IConfigNotify
    void onConfigChange(const XDocument & doc) override;

private:
    string m_name;
    IFactory<IAppSocketNotify> * m_cliSockFact;
    AppSocket::Family m_family;
    vector<Endpoint> m_endpoints;
    AppSocket::MgrFlags m_mgrFlags;
    AppSocket::ConfFlags m_confFlags;

    Duration m_pingInterval = kDefaultPingInterval;
    Duration m_inactiveTimeout = kDefaultInactiveTimeout;

    RunMode m_mode{kRunRunning};
    TimerList<MgrSocket> m_sockets;
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
*   MgrSocket
*
***/

//===========================================================================
MgrSocket::MgrSocket(
    SocketManager & mgr, 
    unique_ptr<IAppSocketNotify> notify
)
    : m_mgr(mgr)
    , IAppSocket(move(notify))
{}

//===========================================================================
MgrSocket::~MgrSocket() {
}

//===========================================================================
void MgrSocket::onTimer(TimePoint now) {
    disconnect();
}

//===========================================================================
void MgrSocket::disconnect() {
    socketDisconnect(this);
}

//===========================================================================
void MgrSocket::write(string_view data) {
    socketWrite(this, data);
}

//===========================================================================
void MgrSocket::write(unique_ptr<SocketBuffer> buffer, size_t bytes) {
    socketWrite(this, move(buffer), bytes);
}

//===========================================================================
bool MgrSocket::onSocketAccept (const AppSocketInfo & info) {
    return notifyAccept(info);
}

//===========================================================================
void MgrSocket::onSocketDisconnect () {
    notifyDisconnect();
}

//===========================================================================
void MgrSocket::onSocketDestroy () {
    notifyDestroy();
}

//===========================================================================
void MgrSocket::onSocketRead (AppSocketData & data) {
    m_mgr.touch(this);
    notifyRead(data);
}


/****************************************************************************
*
*   SocketManager
*
***/

//===========================================================================
SocketManager::SocketManager(
    string_view name,
    IFactory<IAppSocketNotify> * fact,
    AppSocket::Family fam,
    AppSocket::MgrFlags flags
) 
    : m_name{name}
    , m_cliSockFact{fact}
    , m_family{fam}
    , m_mgrFlags{flags}
    , m_sockets{m_inactiveTimeout}
{
    configMonitor("app.xml", this);
}

//===========================================================================
SocketManager::~SocketManager() {
    assert(m_sockets.values().empty());
}

//===========================================================================
void SocketManager::touch(MgrSocket * sock) {
    m_sockets.touch(sock);
}

//===========================================================================
void SocketManager::setInactiveTimeout(
    Duration inactiveTimeout, 
    Duration pingInterval
) {
    m_inactiveTimeout = inactiveTimeout;
    m_pingInterval = pingInterval;
    m_sockets.setTimeout(inactiveTimeout);
}

//===========================================================================
bool SocketManager::shutdown() {
    if (m_mode == kRunRunning) {
        m_mode = kRunStopping;
        for (auto && sock : m_sockets.values()) 
            sock.disconnect();
        for (auto && ep : m_endpoints) 
            socketCloseWait(this, ep, m_family);
    }
    if (m_sockets.values().empty())
        m_mode = kRunStopped;
    return m_mode == kRunStopped;
}

//===========================================================================
std::unique_ptr<IAppSocketNotify> SocketManager::onFactoryCreate() {
    auto ptr = make_unique<MgrSocket>(
        *this, 
        m_cliSockFact->onFactoryCreate()
    );
    m_sockets.touch(ptr.get());
    return ptr;
}

//===========================================================================
void SocketManager::onConfigChange(const XDocument & doc) {
    auto flags = AppSocket::ConfFlags{};
    if (configUnsigned(doc, "DisableInactiveTimeout"))
        flags |= AppSocket::fDisableInactiveTimeout;

    m_confFlags = flags;

    Endpoint ep;
    ep.port = configUnsigned(doc, "Port", nullptr, 41000);
    vector<Endpoint> endpts;
    endpts.push_back(ep);
    vector<Endpoint> diff;
    set_difference(
        endpts.begin(), 
        endpts.end(), 
        m_endpoints.begin(), 
        m_endpoints.end(), 
        back_inserter(diff)
    );

    bool console = (m_mgrFlags & AppSocket::fMgrConsole);
    for (auto && ep : diff) 
        socketListen(this, ep, m_family, console);

    diff.clear();
    set_difference(
        m_endpoints.begin(), 
        m_endpoints.end(), 
        endpts.begin(), 
        endpts.end(), 
        back_inserter(diff)
    );
    for (auto && ep : diff) {
        socketCloseWait(this, ep, m_family);
    }

    m_endpoints = move(endpts);
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
    auto mgr = new SocketManager(
        mgrName,
        factory,
        fam,
        flags
    );
    return s_mgrs.insert(mgr);
}

//===========================================================================
SockMgrHandle Dim::sockMgrConnect(
    string_view mgrName,
    IFactory<IAppSocketNotify> * factory,
    AppSocket::Family fam,
    AppSocket::MgrFlags flags
) {
    assert(0 && "Not implemented");
    return {};
}

//===========================================================================
void Dim::sockMgrSetInactiveTimeout(
    SockMgrHandle h,
    Duration pingInterval,
    Duration pingTimeout
) {
    auto mgr = s_mgrs.find(h);
    mgr->setInactiveTimeout(pingInterval, pingTimeout);
}

//===========================================================================
void Dim::sockMgrMonitorEndpoints(SockMgrHandle h, string_view host) {
    //auto mgr = s_mgrs.find(h);
    //return mgr->monitorEndpoints(host);
}

//===========================================================================
bool Dim::sockMgrShutdown(SockMgrHandle h) {
    auto mgr = s_mgrs.find(h);
    return mgr->shutdown();
}

//===========================================================================
void Dim::sockMgrDestroy(SockMgrHandle h) {
    auto mgr = s_mgrs.find(h);
    assert(!mgr || mgr->shutdown());
    s_mgrs.erase(h);
}
