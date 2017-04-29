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

class SocketManager;

class MgrSocket : public IAppSocket, public IAppSocketNotify {
public:
    ListMemberLink<MgrSocket> m_link;

public:
    MgrSocket(SocketManager & mgr, unique_ptr<IAppSocketNotify> notify);
    ~MgrSocket();

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
{
public:
    SocketManager(IFactory<IAppSocketNotify> * fact);
    ~SocketManager();

    // IFactory<MgrSocket>
    unique_ptr<IAppSocketNotify> onFactoryCreate() override;

private:
    IFactory<IAppSocketNotify> * m_cliSockFact;
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
    notifyRead(data);
}


/****************************************************************************
*
*   SocketManager
*
***/

//===========================================================================
SocketManager::SocketManager(IFactory<IAppSocketNotify> * fact) 
    : m_cliSockFact{fact}
{
}

//===========================================================================
SocketManager::~SocketManager() {
}

//===========================================================================
std::unique_ptr<IAppSocketNotify> SocketManager::onFactoryCreate() {
    return make_unique<MgrSocket>(*this, m_cliSockFact->onFactoryCreate());
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
    std::string_view mgrName,
    IFactory<IAppSocketNotify> * factory,
    /* security requirements, */
    AppSocket::Family fam,
    AppSocket::MgrFlags flags
) {
    auto mgr = new SocketManager(factory);
    return s_mgrs.insert(mgr);
}

//===========================================================================
SockMgrHandle Dim::sockMgrConnect(
    std::string_view mgrName,
    IFactory<IAppSocketNotify> * factory,
    AppSocket::Family fam,
    AppSocket::MgrFlags flags
) {
    assert(0 && "Not implemented");
    return {};
}

//===========================================================================
void Dim::sockMgrMonitorEndpoints(SockMgrHandle h, std::string_view host) {
    //auto mgr = s_mgrs.find(h);
    //return mgr->monitorEndpoints(host);
}

//===========================================================================
bool Dim::sockMgrShutdown(SockMgrHandle h) {
    return true;
}

//===========================================================================
void Dim::sockMgrCloseWait(SockMgrHandle h) {
    s_mgrs.erase(h);
}
