// Copyright Glen Knowles 2015 - 2018.
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

// Set to 9 seconds to align with the default initial RTO of 3 seconds,
// see iSocketSetConnectTimeout() below for details.
constexpr auto kConnectTimeout {9s};


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
        Endpoint const & remote,
        Endpoint const & local,
        string_view initialData,
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
    ISocketNotify * m_notify{};
};

} // namespace


/****************************************************************************
*
*   Variables
*
***/

static List<ConnectTask> s_connectTasks;

static auto & s_perfConnected = uperf("sock.connects");
static auto & s_perfCurConnected = uperf("sock.connects (current)");
static auto & s_perfConnectFailed = uperf("sock.connect failed");


/****************************************************************************
*
*   ConnectTask
*
***/

//===========================================================================
ConnectTask::ConnectTask(unique_ptr<ConnSocket> && sock)
    : m_socket(move(sock))
{
    m_expiration = timeNow() + kConnectTimeout;
}

//===========================================================================
void ConnectTask::onTask() {
    auto [err, bytes] = getOverlappedResult();
    m_socket.release()->onConnect(err, bytes);
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
    taskPushEvent(ptr);
}

//===========================================================================
// static
void ConnSocket::connect(
    ISocketNotify * notify,
    Endpoint const & remote,
    Endpoint const & local,
    string_view data,
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
        (sockaddr *) &sas,
        sizeof(sas),
        (void *) data.data(), // send buffer
        (DWORD) data.size(),  // send buffer length
        &bytes, // bytes sent
        &task->overlapped()
    );
    WinError err;
    if (!error || err != ERROR_IO_PENDING) {
        logMsgError() << "ConnectEx(" << remote << "): " << err;
        return pushConnectFailed(notify);
    }

    s_connectTasks.link(task);
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

    // There doesn't seem to be a better way to get the local and remote
    // addresses than making two calls (getpeername & getsockname):
    //  - getsockopt(SO_BSP_STATE) returns address passed to connect as local
    //  - Use RIOReceiveEx and wait (forever?) for data

    sockaddr_storage sas = {};

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
    if (firstTry) {
        for (auto && task : s_connectTasks)
            task.m_socket->hardClose();
    }
    if (s_connectTasks)
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
    using namespace std::chrono;

    // Since there are up to two retransmissions and their back off is
    // exponential the total wait time is equal to seven times the initial
    // round trip time:
    //  - try, wait RTT, try, wait 2*RTT, try, wait 4*RTT
    //
    // Also note that Windows clamps the RTT to [300, 3000] (in milliseconds),
    // *except* that 0 and 65535 have special meaning!
    auto rtt = duration_cast<milliseconds>(wait).count() / 7 + 1;
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

    // Also set the connection timeout since:
    //  - SIO_TCP_INITIAL_RTO is only supported since Windows 8 and TCP_MAXRT
    //    is available since Windows Vista.
    //  - The RTT can be overridden at the system level, in a way we can't
    //    detect, potentially making the above call into something silly.
    DWORD val = (DWORD) duration_cast<seconds>(wait).count();
    if (SOCKET_ERROR == setsockopt(
        s,
        IPPROTO_TCP,
        TCP_MAXRT,
        (char *) &val,
        sizeof(val)
    )) {
        logMsgError() << "setsockopt(TCP_MAXRT): " << WinError{};
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
    Endpoint const & remote,
    Endpoint const & local,
    string_view data,
    Duration timeout
) {
    iSocketCheckThread();
    ConnSocket::connect(notify, remote, local, data, timeout);
}
