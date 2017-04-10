// Copyright Glen Knowles 2015 - 2017.
// Distributed under the Boost Software License, Version 1.0.
//
// winsockacc.cpp - dim windows platform
#include "pch.h"
#pragma hdrstop

using namespace std;
using namespace Dim;


/****************************************************************************
*
*   Tuning parameters
*
***/


/****************************************************************************
*
*   Private declarations
*
***/

namespace {

class ListenSocket;

class AcceptSocket : public SocketBase {
public:
    static bool accept(ListenSocket * notify);

public:
    using SocketBase::SocketBase;
    void onAccept(ListenSocket * listen, int xferError, int xferBytes);
};

class ListenSocket : public IWinEventWaitNotify {
public:
    bool m_stopRequested{false};
    bool m_stopDone{false};
    ISocketNotify::Mode m_mode{ISocketNotify::kInactive};

    SOCKET m_handle{INVALID_SOCKET};
    Endpoint m_localEnd;
    unique_ptr<AcceptSocket> m_socket;
    IFactory<ISocketNotify> * m_notify{nullptr};
    char m_addrBuf[2 * sizeof sockaddr_storage];

    bool m_inNotify{false};
    thread::id m_inThread;
    condition_variable m_inCv;

public:
    ListenSocket(IFactory<ISocketNotify> * notify, const Endpoint & end);

    void destroy_LK();

    void onTask() override;
};

} // namespace


/****************************************************************************
*
*   Variables
*
***/

static mutex s_mut;
static list<unique_ptr<ListenSocket>> s_listeners;


/****************************************************************************
*
*   ListenSocket
*
***/

//===========================================================================
ListenSocket::ListenSocket(
    IFactory<ISocketNotify> * notify, 
    const Endpoint & end
)
    : m_notify{notify}
    , m_localEnd{end} 
{}

//===========================================================================
void ListenSocket::destroy_LK() {
    for (auto it = s_listeners.begin(); it != s_listeners.end(); ++it) {
        if (this == it->get()) {
            s_listeners.erase(it);
            return;
        }
    }
}

//===========================================================================
void ListenSocket::onTask() {
    DWORD bytes;
    WinError err{0};
    if (!GetOverlappedResult(NULL, &m_overlapped, &bytes, false)) 
        err.set();
    m_socket->onAccept(this, err, bytes);

    if (!AcceptSocket::accept(this)) {
        assert(m_handle == INVALID_SOCKET);
        unique_lock<mutex> lk{s_mut};
        if (m_stopRequested) {
            if (m_stopDone) {
                destroy_LK();
            } else {
                m_mode = ISocketNotify::kClosed;
                lk.unlock();
                m_inCv.notify_all();
            }
        }
    }
}


/****************************************************************************
*
*   AcceptSocket
*
***/

//===========================================================================
static bool closeListen_LK(ListenSocket * listen) {
    listen->m_mode = ISocketNotify::kClosing;
    if (listen->m_handle != INVALID_SOCKET) {
        if (SOCKET_ERROR == closesocket(listen->m_handle))
            logMsgCrash() << "closesocket(listen): " << WinError{};
        listen->m_handle = INVALID_SOCKET;
    }
    return false;
}

//===========================================================================
static bool closeListen(ListenSocket * listen) {
    unique_lock<mutex> lk{s_mut};
    return closeListen_LK(listen);
}

//===========================================================================
// static
bool AcceptSocket::accept(ListenSocket * listen) {
    assert(!listen->m_socket);

    auto sock = make_unique<AcceptSocket>(nullptr);
    sock->m_handle = iSocketCreate();
    if (sock->m_handle == INVALID_SOCKET)
        return closeListen(listen);

    // get AcceptEx function
    GUID extId = WSAID_ACCEPTEX;
    LPFN_ACCEPTEX fAcceptEx;
    DWORD bytes;
    if (WSAIoctl(
        sock->m_handle,
        SIO_GET_EXTENSION_FUNCTION_POINTER,
        &extId,
        sizeof(extId),
        &fAcceptEx,
        sizeof(fAcceptEx),
        &bytes,
        nullptr, // overlapped
        nullptr  // completion routine
    )) {
        logMsgError() << "WSAIoctl(get AcceptEx): " << WinError{};
        return closeListen(listen);
    }

    sock->m_mode = Mode::kAccepting;
    listen->m_socket = move(sock);

    bool error = !fAcceptEx(
        listen->m_handle,
        listen->m_socket->m_handle,
        listen->m_addrBuf,
        0,                       // receive data length
        sizeof sockaddr_storage, // local endpoint length
        sizeof sockaddr_storage, // remote endpoint length
        nullptr,                 // bytes received
        &listen->m_overlapped
    );
    WinError err;
    if (!error || err != ERROR_IO_PENDING) {
        if (err == WSAENOTSOCK && listen->m_handle == INVALID_SOCKET) {
            // socket intentionally closed
        } else {
            logMsgError() << "AcceptEx(" << listen->m_localEnd << "): " << err;
        }
        listen->m_socket.reset();
        return closeListen(listen);
    }
    return true;
}

//===========================================================================
static bool getAcceptInfo(SocketInfo * out, SOCKET s, void * buffer) {
    GUID extId = WSAID_GETACCEPTEXSOCKADDRS;
    LPFN_GETACCEPTEXSOCKADDRS fGetAcceptExSockAddrs;
    DWORD bytes;
    if (WSAIoctl(
        s,
        SIO_GET_EXTENSION_FUNCTION_POINTER,
        &extId,
        sizeof(extId),
        &fGetAcceptExSockAddrs,
        sizeof(fGetAcceptExSockAddrs),
        &bytes,
        nullptr, // overlapped
        nullptr  // completion routine
    )) {
        logMsgError() << "WSAIoctl(get GetAcceptExSockAddrs): " << WinError{};
        return false;
    }

    sockaddr * lsa;
    int lsaLen;
    sockaddr * rsa;
    int rsaLen;
    fGetAcceptExSockAddrs(
        buffer,
        0,
        sizeof(sockaddr_storage),
        sizeof(sockaddr_storage),
        &lsa,
        &lsaLen,
        &rsa,
        &rsaLen
    );

    sockaddr_storage sas;
    memcpy(&sas, lsa, lsaLen);
    copy(&out->local, sas);
    memcpy(&sas, rsa, rsaLen);
    copy(&out->remote, sas);
    return true;
}

//===========================================================================
void AcceptSocket::onAccept(
    ListenSocket * listen,
    int xferError,
    int xferBytes
) {
    unique_ptr<AcceptSocket> hostage{move(listen->m_socket)};

    bool ok = listen->m_handle != INVALID_SOCKET;
    if (xferError) {
        if (xferError == ERROR_OPERATION_ABORTED && !ok) {
            // socket intentionally closed
        } else {
            logMsgError() << "onAccept: " << WinError(xferError);
        }
        return;
    }
    if (!ok)
        return;

    SocketInfo info;
    if (!getAcceptInfo(&info, m_handle, listen->m_addrBuf))
        return;

    if (SOCKET_ERROR == setsockopt(
        m_handle,
        SOL_SOCKET,
        SO_UPDATE_ACCEPT_CONTEXT,
        (char *)&listen->m_handle,
        sizeof listen->m_handle
    )) {
        logMsgError() << "setsockopt(SO_UPDATE_ACCEPT_CONTEXT): "
            << WinError{};
        return;
    }

    {
        lock_guard<mutex> lk{s_mut};
        m_notify = listen->m_notify->onFactoryCreate().release();
        listen->m_inThread = this_thread::get_id();
        listen->m_inNotify = true;
    }

    // create read/write queue
    if (createQueue() && m_notify->onSocketAccept(info))
        hostage.release();

    {
        lock_guard<mutex> lk{s_mut};
        listen->m_inThread = {};
        listen->m_inNotify = false;
    }
}


/****************************************************************************
*
*   ShutdownNotify
*
***/

namespace {
class ShutdownNotify : public IShutdownNotify {
    void onShutdownConsole(bool retry) override;
};
} // namespace
static ShutdownNotify s_cleanup;

//===========================================================================
void ShutdownNotify::onShutdownConsole(bool retry) {
    lock_guard<mutex> lk{s_mut};
    if (!s_listeners.empty())
        return shutdownIncomplete();
}


/****************************************************************************
*
*   Internal API
*
***/

//===========================================================================
void Dim::iSocketAcceptInitialize() {
    shutdownMonitor(&s_cleanup);
}


/****************************************************************************
*
*   Public API
*
***/

//===========================================================================
void Dim::socketListen(
    IFactory<ISocketNotify> * factory,
    const Endpoint & local
) {
    auto hostage = make_unique<ListenSocket>(factory, local);
    auto sock = hostage.get();
    sock->m_handle = iSocketCreate(local);
    if (sock->m_handle == INVALID_SOCKET)
        return;

    if (SOCKET_ERROR == listen(sock->m_handle, SOMAXCONN)) {
        logMsgError() << "listen(SOMAXCONN): " << WinError{};
        if (SOCKET_ERROR == closesocket(sock->m_handle))
            logMsgError() << "closesocket(listen): " << WinError{};
        return;
    }

    sock->m_mode = ISocketNotify::kActive;

    {
        lock_guard<mutex> lk{s_mut};
        s_listeners.push_back(move(hostage));
    }

    if (!AcceptSocket::accept(sock))
        sock->m_mode = ISocketNotify::kClosed;
}

//===========================================================================
void Dim::socketCloseWait(
    IFactory<ISocketNotify> * factory, 
    const Endpoint & local
) {
    unique_lock<mutex> lk{s_mut};
    auto it = s_listeners.begin(),
        e = s_listeners.end();
    ListenSocket * ptr;
    for (;; ++it) {
        if (it == e)
            return;
        ptr = it->get();
        if (ptr->m_notify == factory && ptr->m_localEnd == local) 
            break;
    }

    // found match listener
    closeListen_LK(ptr);
    ptr->m_stopRequested = true;

    // Wait until the callback handler returns, unless that would mean 
    // deadlocking by waiting for ourselves to return.
    while (ptr->m_inNotify && ptr->m_inThread != this_thread::get_id())
        ptr->m_inCv.wait(lk);

    if (ptr->m_mode == ISocketNotify::kClosed) {
        s_listeners.erase(it);
    } else {
        ptr->m_stopDone = true;
    }
}
