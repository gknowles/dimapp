// Copyright Glen Knowles 2017.
// Distributed under the Boost Software License, Version 1.0.
//
// sockmgrint.h - dim net
#pragma once

namespace Dim {


/****************************************************************************
*
*   Declarations
*
***/

namespace AppSocket {

enum ConfFlags : unsigned {
    fDisableInactiveTimeout = 0x01,
};

}

class ISockMgrBase;

class ISockMgrSocket 
    : public ITimerListNotify<>
    , public IAppSocket
    , public IAppSocketNotify 
{
public:
    ISockMgrSocket(
        ISockMgrBase & mgr, 
        std::unique_ptr<IAppSocketNotify> notify
    );

    virtual ISockMgrBase & mgr() { return m_mgr; }

    // Inherited via IAppSocket
    void disconnect() override = 0;
    void write(std::string_view data) override;
    void write(std::unique_ptr<SocketBuffer> buffer, size_t bytes) override;

    // Inherited via IAppSocketNotify
    void onSocketDisconnect() override;
    void onSocketDestroy() override = 0;
    void onSocketRead(AppSocketData & data) override;
    void onSocketBufferChanged(const AppSocketBufferInfo & info) override;

private:
    ISockMgrBase & m_mgr;
};

class ISockMgrBase : public IConfigNotify, public HandleContent {
public:
    ISockMgrBase(
        std::string_view name,
        IFactory<IAppSocketNotify> * fact,
        Duration inactiveTimeout,
        AppSocket::Family fam,
        AppSocket::MgrFlags flags
    );
    virtual ~ISockMgrBase();

    void setInactiveTimeout(Duration timeout);
    void touch(ISockMgrSocket * sock);
    void unlink(ISockMgrSocket * sock);
    bool shutdown();

    virtual bool listening() const = 0;
    virtual void setEndpoints(const std::vector<Endpoint> & endpts) = 0;
    virtual bool onShutdown(bool firstTry) = 0;

    // Inherited via IConfigNotify
    virtual void onConfigChange(const XDocument & doc) override = 0;

protected:
    void init();

    std::string m_name;
    IFactory<IAppSocketNotify> * m_cliSockFact;
    AppSocket::Family m_family;
    std::vector<Endpoint> m_endpoints;
    AppSocket::MgrFlags m_mgrFlags;
    AppSocket::ConfFlags m_confFlags;

    RunMode m_mode{kRunRunning};
    TimerList<ISockMgrSocket> m_inactivity;
};

// Adds manager to handle set.
SockMgrHandle iSockMgrAdd(ISockMgrBase * mgr);

} // namespace
