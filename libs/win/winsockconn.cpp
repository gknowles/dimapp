// Copyright Glen Knowles 2015 - 2017.
// Distributed under the Boost Software License, Version 1.0.
//
// winsockconn.cpp - dim windows platform
#include "pch.h"
#pragma hdrstop

using namespace std;
using namespace Dim;


/****************************************************************************
*
*   Tuning parameters
*
***/

const Duration kConnectTimeout{10s};


/****************************************************************************
*
*   Private declarations
*
***/

namespace {

class ConnSocket : public SocketBase {
public:
    static void connect(
        ISocketNotify * notify,
        const Endpoint & remote,
        const Endpoint & local,
        Duration timeout
    );

public:
    using SocketBase::SocketBase;
    ~ConnSocket();
    void onConnect(WinError error, int bytes);

private:
    void connectFailed();
};

class ConnectTask 
    : public IWinOverlappedNotify
    , public ListBaseLink<> 
{
public:
    TimePoint m_expiration;
    unique_ptr<ConnSocket> m_socket;

public:
    explicit ConnectTask(unique_ptr<ConnSocket> && sock);
    void onTask() override;
};

class ConnectFailedTask : public ITaskNotify {
public:
    explicit ConnectFailedTask(ISocketNotify * notify);
    void onTask() override;

private:
    ISocketNotify * m_notify{nullptr};
};

} // namespace


/****************************************************************************
*
*   Variables
*
***/

static mutex s_mut;
static List<ConnectTask> s_connecting;

static auto & s_perfConnected = uperf("sock connections");
static auto & s_perfCurConnected = uperf("sock connections (current)");
static auto & s_perfConnectFailed = uperf("sock connect failed");


/****************************************************************************
*
*   ConnectTask
*
***/

//===========================================================================
ConnectTask::ConnectTask(unique_ptr<ConnSocket> && sock)
    : m_socket(move(sock)) 
{
    m_expiration = Clock::now() + kConnectTimeout;
}

//===========================================================================
void ConnectTask::onTask() {
    auto [err, bytes] = getOverlappedResult();
    m_socket.release()->onConnect(err, bytes);

    {
        scoped_lock<mutex> lk{s_mut};
        unlink();
    }
    delete this;
}


/****************************************************************************
*
*   ConnectFailedTask
*
***/

//===========================================================================
ConnectFailedTask::ConnectFailedTask(ISocketNotify * notify)
    : m_notify(notify) {}

//===========================================================================
void ConnectFailedTask::onTask() {
    m_notify->onSocketConnectFailed();
    delete this;
}


/****************************************************************************
*
*   ConnSocket
*
***/

//===========================================================================
static void pushConnectFailed(ISocketNotify * notify) {
    auto ptr = new ConnectFailedTask(notify);
    taskPushEvent(*ptr);
}

//===========================================================================
// static
void ConnSocket::connect(
    ISocketNotify * notify,
    const Endpoint & remote,
    const Endpoint & local,
    Duration timeout
) {
    assert(getMode(notify) == Mode::kInactive);

    if (timeout == 0ms)
        timeout = kConnectTimeout;

    auto sock = make_unique<ConnSocket>(notify);
    sock->m_handle = iSocketCreate(local);
    if (sock->m_handle == INVALID_SOCKET)
        return pushConnectFailed(notify);
    if (!winIocpBindHandle((HANDLE) sock->m_handle))
        return pushConnectFailed(notify);

    sock->m_mode = Mode::kConnecting;
    auto hostage = make_unique<ConnectTask>(move(sock));
    auto task = hostage.get();
    iSocketSetConnectTimeout(task->m_socket->m_handle, timeout);

    sockaddr_storage sas;
    copy(&sas, remote);
    DWORD bytes; // ignored, only here for analyzer
    bool error = !s_ConnectEx(
        task->m_socket->m_handle,
        (sockaddr *)&sas,
        sizeof(sas),
        NULL,   // send buffer
        0,      // send buffer length
        &bytes, // bytes sent (ignored)
        &task->overlapped()
    );
    WinError err;
    if (!error || err != ERROR_IO_PENDING) {
        logMsgError() << "ConnectEx(" << remote << "): " << err;
        return pushConnectFailed(notify);
    }

    scoped_lock<mutex> lk{s_mut};
    s_connecting.link(task);

    hostage.release();
}

//===========================================================================
ConnSocket::~ConnSocket() {
    if (m_mode == Mode::kClosed)
        s_perfCurConnected -= 1;
}

//===========================================================================
void ConnSocket::connectFailed() {
    s_perfConnectFailed += 1;
    m_notify->onSocketConnectFailed();
}

//===========================================================================
void ConnSocket::onConnect(WinError error, int bytes) {
    unique_ptr<ConnSocket> hostage(this);

    if (m_mode == ISocketNotify::kClosing)
        return connectFailed();

    assert(m_mode == ISocketNotify::kConnecting);

    if (error)
        return connectFailed();

    //-----------------------------------------------------------------------
    // update socket and start receiving
    if (SOCKET_ERROR == setsockopt(
        m_handle, 
        SOL_SOCKET, 
        SO_UPDATE_CONNECT_CONTEXT, 
        nullptr, 
        0
    )) {
        logMsgError() << "setsockopt(SO_UPDATE_CONNECT_CONTEXT): "
            << WinError{};
        return connectFailed();
    }

    //-----------------------------------------------------------------------
    // get local and remote addresses
    sockaddr_storage sas = {};

    // TODO: use getsockopt(SO_BSP_STATE) instead of getpeername & getsockname

    // address of remote node
    int sasLen = sizeof(sas);
    if (SOCKET_ERROR == getpeername(m_handle, (sockaddr *)&sas, &sasLen)) {
        logMsgError() << "getpeername: " << WinError{};
        return connectFailed();
    }
    copy(&m_connInfo.remote, sas);

    // locally bound address
    if (SOCKET_ERROR == getsockname(m_handle, (sockaddr *)&sas, &sasLen)) {
        logMsgError() << "getsockname: " << WinError{};
        return connectFailed();
    }
    copy(&m_connInfo.local, sas);

    //-----------------------------------------------------------------------
    // create read/write queue
    if (!createQueue()) 
        return connectFailed();

    // notify socket connect event
    s_perfConnected += 1;
    s_perfCurConnected += 1;
    hostage.release();
    m_notify->onSocketConnect(m_connInfo);
    enableEvents();
}


/****************************************************************************
*
*   ShutdownNotify
*
***/

namespace {

class ShutdownNotify : public IShutdownNotify {
    void onShutdownConsole(bool firstTry) override;
};

} // namespace

static ShutdownNotify s_cleanup;

//===========================================================================
void ShutdownNotify::onShutdownConsole(bool firstTry) {
    scoped_lock<mutex> lk{s_mut};
    if (firstTry) {
        for (auto && task : s_connecting)
            task.m_socket->hardClose();
    }
    if (!s_connecting.empty())
        shutdownIncomplete();
}


/****************************************************************************
*
*   Internal API
*
***/

//===========================================================================
void Dim::iSocketConnectInitialize() {
    shutdownMonitor(&s_cleanup);
}

//===========================================================================
void Dim::iSocketSetConnectTimeout(SOCKET s, Duration wait) {
    // Since there are up to two retransmissions and their back off is 
    // exponential the total wait time is equal to seven times the initial 
    // round trip time:
    //  - try, wait rtt, try, wait 2*rtt, try, wait 4*rtt
    // 
    // Also note that Windows clamps the rtt to [300, 3000] (in milliseconds),
    // *except* that 0 and 65535 have special meaning!
    auto rtt = chrono::duration_cast<chrono::milliseconds>(wait).count() / 7;
    rtt = clamp((size_t) rtt, (size_t) 1, (size_t) 65534);

    TCP_INITIAL_RTO_PARAMETERS params[2] = {};
    params[0].Rtt = (unsigned short) rtt;
    params[0].MaxSynRetransmissions = 2;
    DWORD bytes;
    if (SOCKET_ERROR == WSAIoctl(
        s,
        SIO_TCP_INITIAL_RTO,
        &params[0], sizeof(params),
        nullptr, 0, // output buffer, buffer size
        &bytes,     // bytes returned
        nullptr,    // overlapped
        nullptr     // completion routine
    )) {
        logMsgError() << "WSAIoctl(SIO_TCP_INITIAL_RTO): " << WinError{};
    }
}


/****************************************************************************
*
*   Public API
*
***/

//===========================================================================
void Dim::socketConnect(
    ISocketNotify * notify,
    const Endpoint & remote,
    const Endpoint & local,
    Duration timeout
) {
    ConnSocket::connect(notify, remote, local, timeout);
}
