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
    void onConnect(int error, int bytes);
};

class ConnectTask : public IWinEventWaitNotify {
public:
    TimePoint m_expiration;
    unique_ptr<ConnSocket> m_socket;
    list<ConnectTask>::iterator m_iter;

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

class ConnectTimer : public ITimerNotify {
    Duration onTimer(TimePoint now) override;
};

} // namespace


/****************************************************************************
*
*   Variables
*
***/

static mutex s_mut;
static list<ConnectTask> s_connecting;
static list<ConnectTask> s_closing;
static ConnectTimer s_connectTimer;


/****************************************************************************
*
*   ConnectTimer
*
***/

//===========================================================================
Duration ConnectTimer::onTimer(TimePoint now) {
    lock_guard<mutex> lk{s_mut};
    while (!s_connecting.empty()) {
        auto it = s_connecting.begin();
        if (now < it->m_expiration) {
            return it->m_expiration - now;
        }
        it->m_socket->hardClose_LK();
        it->m_expiration = TimePoint::max();
        s_closing.splice(s_closing.end(), s_connecting, it);
    }
    return kTimerInfinite;
}


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
    DWORD bytes;
    WinError err{0};
    if (!GetOverlappedResult(NULL, &m_overlapped, &bytes, false))
        err = WinError{};
    m_socket.release()->onConnect(err, bytes);

    lock_guard<mutex> lk{s_mut};
    if (m_expiration == TimePoint::max()) {
        s_closing.erase(m_iter);
    } else {
        s_connecting.erase(m_iter);
    }
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

    sock->m_mode = Mode::kConnecting;
    list<ConnectTask>::iterator it;
    timerUpdate(&s_connectTimer, timeout, true);

    {
        lock_guard<mutex> lk{s_mut};
        TimePoint expiration = Clock::now() + timeout;

        // TODO: check if this really puts them in expiration order!
        auto rhint = find_if(
            s_connecting.rbegin(), 
            s_connecting.rend(), 
            [&](auto && task) { return task.m_expiration <= expiration; }
        );
        it = s_connecting.emplace(rhint.base(), move(sock));

        it->m_iter = it;
        it->m_expiration = expiration;
    }

    sockaddr_storage sas;
    copy(&sas, remote);
    DWORD bytes; // ignored, only here for analyzer
    bool error = !s_ConnectEx(
        it->m_socket->m_handle,
        (sockaddr *)&sas,
        sizeof(sas),
        NULL,   // send buffer
        0,      // send buffer length
        &bytes, // bytes sent (ignored)
        &it->m_overlapped
    );
    WinError err;
    if (!error || err != ERROR_IO_PENDING) {
        logMsgError() << "ConnectEx(" << remote << "): " << err;
        lock_guard<mutex> lk{s_mut};
        s_connecting.erase(it);
        return pushConnectFailed(notify);
    }
}

//===========================================================================
void ConnSocket::onConnect(int error, int bytes) {
    unique_ptr<ConnSocket> hostage(this);

    if (m_mode == ISocketNotify::kClosing)
        return m_notify->onSocketConnectFailed();

    assert(m_mode == ISocketNotify::kConnecting);

    if (error)
        return m_notify->onSocketConnectFailed();

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
        return m_notify->onSocketConnectFailed();
    }

    //-----------------------------------------------------------------------
    // get local and remote addresses
    sockaddr_storage sas = {};

    // TODO: use getsockopt(SO_BSP_STATE) instead of getpeername & getsockname

    // address of remote node
    int sasLen = sizeof(sas);
    if (SOCKET_ERROR == getpeername(m_handle, (sockaddr *)&sas, &sasLen)) {
        logMsgError() << "getpeername: " << WinError{};
        return m_notify->onSocketConnectFailed();
    }
    copy(&m_connInfo.remote, sas);

    // locally bound address
    if (SOCKET_ERROR == getsockname(m_handle, (sockaddr *)&sas, &sasLen)) {
        logMsgError() << "getsockname: " << WinError{};
        return m_notify->onSocketConnectFailed();
    }
    copy(&m_connInfo.local, sas);

    //-----------------------------------------------------------------------
    // create read/write queue
    if (!createQueue())
        return m_notify->onSocketConnectFailed();

    // notify socket connect event
    hostage.release();
    m_notify->onSocketConnect(m_connInfo);
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
    lock_guard<mutex> lk{s_mut};
    if (firstTry) {
        for (auto && task : s_connecting)
            task.m_socket->hardClose_LK();
    }
    if (!s_connecting.empty() || !s_closing.empty())
        shutdownIncomplete();
}


/****************************************************************************
*
*   Internal API
*
***/

//===========================================================================
void Dim::iSocketConnectInitialize() {
    // Don't register cleanup until all dependents (aka sockbuf) have
    // registered their cleanups (aka been initialized)
    shutdownMonitor(&s_cleanup);
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
