// Copyright Glen Knowles 2015 - 2021.
// Distributed under the Boost Software License, Version 1.0.
//
// winsockacc.cpp - dim windows platform
#include "pch.h"
#pragma hdrstop

using namespace std;
using namespace Dim;


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
    ~AcceptSocket();

    void onAccept(ListenSocket * listen, WinError xferError, int xferBytes);
};

class ListenSocket
    : public IWinOverlappedNotify
    , public ListLink<>
{
public:
    bool m_stopRequested{false};
    bool m_stopDone{false};
    ISocketNotify::Mode m_mode{ISocketNotify::kInactive};

    SOCKET m_handle{INVALID_SOCKET};
    unique_ptr<AcceptSocket> m_socket;
    IFactory<ISocketNotify> * m_notify{};
    SockAddr m_localAddr;
    char m_addrBuf[2 * sizeof sockaddr_storage];

public:
    ListenSocket(IFactory<ISocketNotify> * notify, const SockAddr & end);

    void onTask() override;
};

} // namespace


/****************************************************************************
*
*   Variables
*
***/

static List<ListenSocket> s_listeners;

static auto & s_perfAccepts = uperf("sock.accepts");
static auto & s_perfCurAccepts = uperf("sock.accepts (current)");
static auto & s_perfNotAccepted = uperf("sock.disconnect (not accepted)");


/****************************************************************************
*
*   ListenSocket
*
***/

//===========================================================================
ListenSocket::ListenSocket(
    IFactory<ISocketNotify> * notify,
    const SockAddr & addr
)
    : m_notify{notify}
    , m_localAddr{addr}
{}

//===========================================================================
void ListenSocket::onTask() {
    auto [err, bytes] = decodeOverlappedResult();
    m_socket->onAccept(this, err, bytes);

    if (!AcceptSocket::accept(this)) {
        assert(m_handle == INVALID_SOCKET);
        if (!m_stopRequested)
            return;
        if (m_stopDone) {
            delete this;
        } else {
            m_mode = ISocketNotify::kClosed;
        }
    }
}


/****************************************************************************
*
*   AcceptSocket
*
***/

//===========================================================================
static bool closeListenHandle(ListenSocket * listen) {
    listen->m_mode = ISocketNotify::kClosing;
    if (listen->m_handle != INVALID_SOCKET) {
        if (SOCKET_ERROR == closesocket(listen->m_handle))
            logMsgFatal() << "closesocket(listen): " << wsaError();
        listen->m_handle = INVALID_SOCKET;
    }
    return false;
}

//===========================================================================
static void closeListen(ListenSocket * listen) {
    closeListenHandle(listen);
    listen->m_stopRequested = true;

    if (listen->m_mode == ISocketNotify::kClosed) {
        delete listen;
    } else {
        listen->m_stopDone = true;
    }
}

//===========================================================================
// static
bool AcceptSocket::accept(ListenSocket * listen) {
    assert(!listen->m_socket);

    auto sock = make_unique<AcceptSocket>(nullptr);
    sock->m_handle = iSocketCreate();
    if (sock->m_handle == INVALID_SOCKET)
        return closeListenHandle(listen);

    sock->m_mode = Mode::kAccepting;
    listen->m_socket = move(sock);

    if (listen->m_handle != INVALID_SOCKET) {
        DWORD bytes; // ignored, only here for analyzer
        WinError err = 0;
        if (!s_AcceptEx(
            listen->m_handle,
            listen->m_socket->m_handle,
            listen->m_addrBuf,
            0,                       // receive data length
            sizeof sockaddr_storage, // local address length
            sizeof sockaddr_storage, // remote address length
            &bytes,                  // bytes received (ignored)
            &listen->overlapped()
        )) {
            err.set();
        }
        if (!err || err == ERROR_IO_PENDING)
            return true;

        if (err != WSAECONNRESET) {
            logMsgError() << "AcceptEx(" << listen->m_localAddr << "): "
                << err;
        }
    }

    listen->m_socket.reset();
    return closeListenHandle(listen);
}

//===========================================================================
AcceptSocket::~AcceptSocket() {
    if (m_mode == Mode::kClosed)
        s_perfCurAccepts -= 1;
}

//===========================================================================
static bool getAcceptInfo(SocketInfo * out, SOCKET s, void * buffer) {
    sockaddr * lsa;
    int lsaLen;
    sockaddr * rsa;
    int rsaLen;
    SocketBase::s_GetAcceptExSockaddrs(
        buffer,
        0,
        sizeof sockaddr_storage,
        sizeof sockaddr_storage,
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
    WinError xferError,
    int xferBytes
) {
    unique_ptr<AcceptSocket> hostage{move(listen->m_socket)};

    bool ok = listen->m_handle != INVALID_SOCKET;
    if (xferError) {
        if (!ok
            && (xferError == ERROR_OPERATION_ABORTED
                || xferError == ERROR_NETNAME_DELETED
                )
        ) {
            // socket intentionally closed
        } else {
            logMsgError() << "onAccept: " << xferError;
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
            << wsaError();
        return;
    }

    m_notify = listen->m_notify->onFactoryCreate().release();

    // create read/write queue
    if (createQueue()) {
        s_perfAccepts += 1;
        s_perfCurAccepts += 1;
        hostage.release();
        if (!m_notify->onSocketAccept(info)) {
            s_perfNotAccepted += 1;
            hardClose();
        }
        enableEvents();
    }
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
    if (s_listeners)
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
    const SockAddr & local
) {
    iSocketCheckThread();

    auto hostage = make_unique<ListenSocket>(factory, local);
    auto sock = hostage.get();
    sock->m_handle = iSocketCreate(local);
    if (sock->m_handle == INVALID_SOCKET)
        return;
    if (!winIocpBindHandle((HANDLE) sock->m_handle))
        return;

    if (SOCKET_ERROR == listen(sock->m_handle, SOMAXCONN)) {
        logMsgError() << "listen(SOMAXCONN): " << wsaError();
        if (SOCKET_ERROR == closesocket(sock->m_handle))
            logMsgError() << "closesocket(listen): " << wsaError();
        return;
    }

    sock->m_mode = ISocketNotify::kActive;
    s_listeners.link(hostage.release());

    if (!AcceptSocket::accept(sock))
        sock->m_mode = ISocketNotify::kClosed;
}

//===========================================================================
void Dim::socketCloseWait(
    IFactory<ISocketNotify> * factory,
    const SockAddr & local
) {
    iSocketCheckThread();

    for (auto && ls : s_listeners) {
        if (ls.m_notify == factory && ls.m_localAddr == local) {
            closeListen(&ls);
            return;
        }
    }
}
