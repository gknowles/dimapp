// Copyright Glen Knowles 2015 - 2023.
// Distributed under the Boost Software License, Version 1.0.
//
// winsock.cpp - dim windows platform
#include "pch.h"
#pragma hdrstop

using namespace std;
using namespace Dim;


/****************************************************************************
*
*   Tuning parameters
*
***/

const int kReadQueueSize = 10;
const int kWriteQueueSize = 100;

// How long data can wait to be sent. When the queue time exceeds this value
// the socket is disconnected, the assumption being that the end consumer, if
// they still care, has a retry in flight and would discard it anyway as
// being expired.
constexpr auto kMaxPrewriteQueueTime = 2min;


/****************************************************************************
*
*   Declarations
*
***/

namespace {

enum RequestType {
    kReqInvalid,
    kReqRead,
    kReqWrite,
};

} // namespace

struct Dim::SocketRequest : ListLink<> {
    RequestType m_type{kReqInvalid};
    RIO_BUF m_rbuf{};
    unique_ptr<SocketBuffer> m_buffer;
    TimePoint m_qtime;

    // filled in after completion
    WinError m_xferError{0};
    int m_xferBytes{0};
};


/****************************************************************************
*
*   Variables
*
***/

static RIO_EXTENSION_FUNCTION_TABLE s_rio;

static atomic_int s_numSockets;

static auto & s_perfReadTotal = uperf(
    "sock.read bytes (total)",
    PerfFormat::kSiUnits
);
static auto & s_perfIncomplete = uperf(
    "sock.write bytes (incomplete)",
    PerfFormat::kSiUnits
);
static auto & s_perfWaiting = uperf(
    "sock.write bytes (waiting)",
    PerfFormat::kSiUnits
);
static auto & s_perfWriteTotal = uperf(
    "sock.write bytes (total)",
    PerfFormat::kSiUnits
);

// The data in the send queue is either too massive or too old.
static auto & s_perfBacklog = uperf("sock.disconnect (write backlog)");


/****************************************************************************
*
*   SocketBase
*
***/

// static
LPFN_ACCEPTEX SocketBase::s_AcceptEx;
LPFN_GETACCEPTEXSOCKADDRS SocketBase::s_GetAcceptExSockaddrs;
LPFN_CONNECTEX SocketBase::s_ConnectEx;

//===========================================================================
// static
SocketBase::Mode SocketBase::getMode(const ISocketNotify * notify) {
    if (auto sock = notify->m_socket)
        return sock->mode();

    return Mode::kInactive;
}

//===========================================================================
// static
SocketInfo SocketBase::getInfo(const ISocketNotify * notify) {
    SocketInfo out = {};
    if (auto sock = notify->m_socket) {
        out.mode = sock->m_mode;
        out.lastModeTime = sock->m_lastModeTime;
        out.localAddr = sock->m_connInfo.localAddr;
        out.remoteAddr = sock->m_connInfo.remoteAddr;
        out.lastReadTime = sock->m_lastReadTime;
        out.readTotal = sock->m_bytesRead;
        out.lastWriteTime = sock->m_lastWriteTime;
        out.buffer = sock->m_bufInfo;
    }
    return out;
}

//===========================================================================
// static
void SocketBase::disconnect(ISocketNotify * notify) {
    if (auto sock = notify->m_socket)
        sock->hardClose();
}

//===========================================================================
SocketBase::SocketBase(ISocketNotify * notify)
    : m_notify(notify)
{
    s_numSockets += 1;
}

//===========================================================================
SocketBase::~SocketBase() {
    assert(!m_reads && !m_writes);
    ISocketNotify * notify = nullptr;

    if (m_notify) {
        notify = m_notify;
        m_notify->m_socket = nullptr;
    }
    hardClose();
    m_prereads.clear();

    s_numSockets -= 1;

    if (m_cq != RIO_INVALID_CQ)
        s_rio.RIOCloseCompletionQueue(m_cq);

    if (notify)
        notify->onSocketDestroy();
}

//===========================================================================
void SocketBase::hardClose() {
    if (m_handle == INVALID_SOCKET)
        return;

    // force immediate close by enabling shutdown timeout and setting the
    // timeout to 0 seconds
    linger opt = {};
    opt.l_onoff = true;
    opt.l_linger = 0;
    setsockopt(m_handle, SOL_SOCKET, SO_LINGER, (char *)&opt, sizeof opt);
    closesocket(m_handle);

    m_mode = Mode::kClosing;
    m_handle = INVALID_SOCKET;
    while (auto req = m_prewrites.front()) {
        auto bytes = req->m_rbuf.Length;
        m_bufInfo.waiting -= bytes;
        s_perfWaiting -= bytes;
        delete req;
    }
}

//===========================================================================
bool SocketBase::createQueue() {
    m_maxReads = kReadQueueSize;
    m_maxWrites = kWriteQueueSize;

    // create completion queue
    RIO_NOTIFICATION_COMPLETION ctype = {};
    ctype.Type = RIO_IOCP_COMPLETION;
    ctype.Iocp.IocpHandle = winIocpHandle();
    ctype.Iocp.Overlapped = &overlapped();
    m_cq = s_rio.RIOCreateCompletionQueue(m_maxReads + m_maxWrites, &ctype);
    if (m_cq == RIO_INVALID_CQ) {
        logMsgFatal() << "RIOCreateCompletionQueue: " << WinError{};
        __assume(0);
    }

    // create request queue
    m_rq = s_rio.RIOCreateRequestQueue(
        m_handle,
        m_maxReads,     // max outstanding receive requests
        1,              // max receive buffers (must be 1)
        m_maxWrites,    // max outstanding send requests
        1,              // max send buffers (must be 1)
        m_cq,           // receive completion queue
        m_cq,           // send completion queue
        this            // socket context
    );
    if (m_rq == RIO_INVALID_RQ) {
        logMsgError() << "RIOCreateRequestQueue: " << WinError{};
        return false;
    }

    for (int i = 0; i < m_maxReads; ++i) {
        m_prereads.link(new SocketRequest);
        auto task = m_prereads.back();
        task->m_type = kReqRead;
        task->m_buffer = socketGetBuffer();

        // Leave room for extra trailing null. Although not reported in the
        // count a trailing null is added to the end of the buffer after every
        // read. It doesn't cost much and makes parsing easier for some text
        // protocol streams.
        int bytes = task->m_buffer->capacity;
        copy(&task->m_rbuf, *task->m_buffer, bytes);
    }

    m_mode = Mode::kActive;
    m_notify->m_socket = this;

    return true;
}

//===========================================================================
void SocketBase::enableEvents() {
    while (m_prereads)
        requeueRead();

    // trigger first RIONotify
    if (int error = s_rio.RIONotify(m_cq))
        logMsgFatal() << "RIONotify: " << WinError{error};
}


//===========================================================================
void SocketBase::mode(Mode mode) {
    if (mode != m_mode) {
        m_mode = mode;
        m_lastModeTime = timeNow();
    }
}

//===========================================================================
void SocketBase::onTask() {
    static constexpr int kMaxResults = 10;
    RIORESULT results[kMaxResults];
    int count = s_rio.RIODequeueCompletion(
        m_cq,
        results,
        (ULONG) size(results)
    );
    if (count == RIO_CORRUPT_CQ)
        logMsgFatal() << "RIODequeueCompletion: " << WinError{};

    for (int i = 0; i < count; ++i) {
        auto && rr = results[i];
        auto task = (SocketRequest *) rr.RequestContext;
        task->m_xferError = rr.Status;
        task->m_xferBytes = rr.BytesTransferred;

        // This task - and the socket - may be deleted inside onRead/onWrite.
        if (task->m_type == kReqRead) {
            if (!onRead(task))
                return;
        } else {
            assert(task->m_type == kReqWrite);
            if (!onWrite(task))
                return;
        }
    }

    if (int error = s_rio.RIONotify(m_cq))
        logMsgFatal() << "RIONotify: " << WinError{error};
}

//===========================================================================
// static
void SocketBase::read(ISocketNotify * notify) {
    if (auto sock = notify->m_socket)
        sock->requeueRead();
}

//===========================================================================
void SocketBase::requeueRead() {
    auto task = m_prereads.front();

    // Queuing reads is only allowed after an automatic requeuing was
    // rejected via returning false from onSocketRead.
    assert(task);

    if (m_mode == Mode::kActive) {
        m_reads.link(task);
        queueRead(task);
    } else {
        assert(m_mode == Mode::kClosing || m_mode == Mode::kClosed);
        task->m_xferBytes = 0;
        task->m_xferError = 0;
        onRead(task);
    }
}

//===========================================================================
void SocketBase::queueRead(SocketRequest * task) {
    if (!s_rio.RIOReceive(
        m_rq,
        &task->m_rbuf,
        1, // number of RIO_BUFs (must be 1)
        0, // RIO_MSG_* flags
        task
    )) {
        logMsgFatal() << "RIOReceive: " << WinError{};
    }
}

//===========================================================================
bool SocketBase::onRead(SocketRequest * task) {
    if (int bytes = task->m_xferBytes) {
        s_perfReadTotal += bytes;
        m_bytesRead += bytes;
        m_lastReadTime = timeNow();

        SocketData data;
        data.data = (char *)task->m_buffer->data;
        data.bytes = bytes;

        // included uncounted trailing null
        data.data[bytes] = 0;

        if (!m_notify->onSocketRead(data)) {
            // new read will be explicitly requested by application
            m_prereads.link(task);
            return true;
        }

        if (m_mode == Mode::kActive) {
            queueRead(task);
            return true;
        }

        assert(m_mode == Mode::kClosing || m_mode == Mode::kClosed);
    }

    delete task;

    if (!m_reads) {
        if (m_mode != Mode::kClosed) {
            m_mode = Mode::kClosed;
            m_notify->onSocketDisconnect();
        }
        if (!m_writes) {
            delete this;
            return false;
        }
    }
    return true;
}

//===========================================================================
// static
void SocketBase::write(
    ISocketNotify * notify,
    unique_ptr<SocketBuffer> buffer,
    size_t bytes
) {
    assert(bytes <= (size_t) buffer->capacity);
    if (!bytes)
        return;
    if (auto sock = notify->m_socket)
        sock->queuePrewrite(move(buffer), bytes);
}

//===========================================================================
void SocketBase::queuePrewrite(
    unique_ptr<SocketBuffer> buffer,
    size_t bytes
) {
    assert(bytes);
    if (m_mode == Mode::kClosing || m_mode == Mode::kClosed)
        return;

    s_perfWaiting += (unsigned) bytes;
    m_bufInfo.waiting += bytes;
    bool wasPrewrites = (bool) m_prewrites;

    if (wasPrewrites) {
        auto back = m_prewrites.back();
        if (int count = min(
            back->m_buffer->capacity - (int)back->m_rbuf.Length,
            (int)bytes
        )) {
            memcpy(
                back->m_buffer->data + back->m_rbuf.Length,
                buffer->data,
                count
            );
            back->m_rbuf.Length += count;
            bytes -= count;
            if (bytes)
                memmove(buffer->data, buffer->data + count, bytes);
        }
    }

    if (bytes) {
        m_prewrites.link(new SocketRequest);
        auto task = m_prewrites.back();
        task->m_type = kReqWrite;
        task->m_buffer = move(buffer);
        copy(&task->m_rbuf, *task->m_buffer, bytes);
    }

    queueWrites();
    if (auto task = m_prewrites.back()) {
        if (task->m_qtime == TimePoint::min()) {
            auto now = timeNow();
            task->m_qtime = now;
            auto maxQTime = now - m_prewrites.front()->m_qtime;
            if (maxQTime > kMaxPrewriteQueueTime) {
                s_perfBacklog += 1;
                return hardClose();
            }
        }
        if (!wasPrewrites) {
            // data is now waiting
            assert(m_bufInfo.waiting <= bytes);
            auto info = m_bufInfo;
            m_notify->onSocketBufferChanged(info);
        }
    }
}

//===========================================================================
void SocketBase::queueWrites() {
    while (m_numWrites < m_maxWrites && m_prewrites) {
        m_writes.link(m_prewrites.front());
        m_numWrites += 1;
        auto task = m_writes.back();
        auto bytes = task->m_rbuf.Length;
        s_perfWaiting -= bytes;
        m_bufInfo.waiting -= bytes;
        if (!s_rio.RIOSend(m_rq, &task->m_rbuf, 1, 0, task)) {
            WinError err;
            logMsgFatal() << "RIOSend: " << err;
            delete task;
            m_numWrites -= 1;
        } else {
            s_perfIncomplete += bytes;
            m_bufInfo.incomplete += bytes;
        }
    }
}

//===========================================================================
bool SocketBase::onWrite(SocketRequest * task) {
    auto bytes = task->m_rbuf.Length;
    delete task;
    s_perfIncomplete -= bytes;
    s_perfWriteTotal += bytes;
    m_numWrites -= 1;
    m_bufInfo.incomplete -= bytes;
    m_bufInfo.total += bytes;
    m_lastWriteTime = timeNow();

    // already disconnected and this was the last unresolved write? delete
    if (m_mode == Mode::kClosed && !m_reads && !m_writes) {
        delete this;
        return false;
    }

    bool wasPrewrites = (bool) m_prewrites;
    queueWrites();
    if (wasPrewrites && !m_prewrites // data no longer waiting
        || !m_numWrites              // send queue is empty
    ) {
        assert(!m_bufInfo.waiting);
        assert(m_numWrites || !m_bufInfo.incomplete);
        auto info = m_bufInfo;
        m_notify->onSocketBufferChanged(info);
    }
    return true;
}


/****************************************************************************
*
*   ShutdownNotify
*
***/

thread_local bool t_socketInitialized;

namespace {

class ShutdownNotify : public IShutdownNotify {
    void onShutdownConsole(bool firstTry) override;
};

} // namespace

static ShutdownNotify s_cleanup;

//===========================================================================
void ShutdownNotify::onShutdownConsole(bool firstTry) {
    if (s_numSockets)
        return shutdownIncomplete();

    // close windows sockets
    if (WSACleanup())
        logMsgError() << "WSACleanup: " << WinError{};
    t_socketInitialized = false;
}


/****************************************************************************
*
*   Internal API
*
***/

static WSAPROTOCOL_INFOW s_protocolInfo;

//===========================================================================
void Dim::iSocketCheckThread() {
    if (!t_socketInitialized)
        logMsgFatal() << "Socket services must be called on the event thread";
}

//===========================================================================
void Dim::iSocketInitialize() {
    t_socketInitialized = true;

    WSADATA data = {};
    WinError err = WSAStartup(WINSOCK_VERSION, &data);
    if (err || data.wVersion != WINSOCK_VERSION) {
        logMsgFatal() << "WSAStartup(version=" << hex << WINSOCK_VERSION
            << "): " << err << ", version " << data.wVersion;
    }

    // get extension functions
    SOCKET s = WSASocketW(
        AF_INET6,
        SOCK_STREAM,
        IPPROTO_TCP,
        NULL, // protocol info (additional creation options)
        0,    // socket group
        WSA_FLAG_REGISTERED_IO | WSA_FLAG_NO_HANDLE_INHERIT
    );
    if (s == INVALID_SOCKET)
        logMsgFatal() << "WSASocketW: " << WinError{};

    // get RIO functions
    GUID extId = WSAID_MULTIPLE_RIO;
    s_rio.cbSize = sizeof s_rio;
    DWORD bytes;
    if (WSAIoctl(
        s,
        SIO_GET_MULTIPLE_EXTENSION_FUNCTION_POINTER,
        &extId,
        sizeof extId,
        &s_rio,
        sizeof s_rio,
        &bytes,
        nullptr, // overlapped
        nullptr  // completion routine
    )) {
        logMsgFatal() << "WSAIoctl(get RIO functions): " << WinError{};
    }

    // get AcceptEx function
    extId = WSAID_ACCEPTEX;
    if (WSAIoctl(
        s,
        SIO_GET_EXTENSION_FUNCTION_POINTER,
        &extId,
        sizeof extId,
        &SocketBase::s_AcceptEx,
        sizeof SocketBase::s_AcceptEx,
        &bytes,
        nullptr, // overlapped
        nullptr  // completion routine
    )) {
        logMsgFatal() << "WSAIoctl(get AcceptEx): " << WinError{};
    }

    extId = WSAID_GETACCEPTEXSOCKADDRS;
    if (WSAIoctl(
        s,
        SIO_GET_EXTENSION_FUNCTION_POINTER,
        &extId,
        sizeof extId,
        &SocketBase::s_GetAcceptExSockaddrs,
        sizeof SocketBase::s_GetAcceptExSockaddrs,
        &bytes,
        nullptr, // overlapped
        nullptr  // completion routine
    )) {
        logMsgFatal() << "WSAIoctl(get GetAcceptExSockAddrs): " << WinError{};
    }

    // get ConnectEx function
    extId = WSAID_CONNECTEX;
    if (WSAIoctl(
        s,
        SIO_GET_EXTENSION_FUNCTION_POINTER,
        &extId,
        sizeof extId,
        &SocketBase::s_ConnectEx,
        sizeof SocketBase::s_ConnectEx,
        &bytes,
        nullptr, // overlapped
        nullptr  // completion routine
    )) {
        logMsgFatal() << "WSAIoctl(get ConnectEx): " << WinError{};
    }

    int piLen = sizeof s_protocolInfo;
    if (getsockopt(
        s,
        SOL_SOCKET,
        SO_PROTOCOL_INFOW,
        (char *) &s_protocolInfo,
        &piLen
    )) {
        logMsgFatal() << "getsockopt(SO_PROTOCOL_INFOW): " << WinError{};
    }
    if (~s_protocolInfo.dwServiceFlags1 & XP1_IFS_HANDLES) {
        logMsgFatal() << "socket and file handles not compatible";
    }

    closesocket(s);

    // initialize buffer allocator
    iSocketBufferInitialize(s_rio);
    // Don't register cleanup until all dependents (aka sockbuf) have
    // registered their cleanups (aka have been initialized)
    shutdownMonitor(&s_cleanup);
    iSocketAcceptInitialize();
    iSocketConnectInitialize();
}

//===========================================================================
SOCKET Dim::iSocketCreate() {
    SOCKET handle = WSASocketW(
        FROM_PROTOCOL_INFO,
        FROM_PROTOCOL_INFO,
        FROM_PROTOCOL_INFO,
        &s_protocolInfo,
        0,
        WSA_FLAG_REGISTERED_IO | WSA_FLAG_NO_HANDLE_INHERIT
    );
    if (handle == INVALID_SOCKET) {
        logMsgError() << "WSASocket: " << wsaError();
        return INVALID_SOCKET;
    }

    if (!SetFileCompletionNotificationModes(
        (HANDLE) handle,
        FILE_SKIP_SET_EVENT_ON_HANDLE
    )) {
        logMsgError()
            << "SetFileCompletionNotificationModes(SKIP_EVENT_ON_HANDLE): "
            << WinError{};
    }

    int yes = 1;
    int no = 0;

    // Allow local ports to be shared among multiple outgoing connections. Like
    // with incoming connections as long as the combination of remote and
    // local's address and port is unique everything is fine.
    if (SOCKET_ERROR == setsockopt(
        handle,
        SOL_SOCKET,
        SO_REUSE_UNICASTPORT,
        (char *) &yes,
        sizeof yes
    )) {
        if (SOCKET_ERROR == setsockopt(
            handle,
            SOL_SOCKET,
            SO_PORT_SCALABILITY,
            (char *) &yes,
            sizeof yes
        )) {
            logMsgError() << "setsockopt(SO_PORT_SCALABILITY): " << wsaError();
        }
    }

    // Set to dual mode, supporting both Ipv6 and Ipv4 connections.
    if (SOCKET_ERROR == setsockopt(
        handle,
        IPPROTO_IPV6,
        IPV6_V6ONLY,
        (char *) &no,
        sizeof no
    )) {
        logMsgDebug() << "setsockopt(IPV6_V6ONLY): " << wsaError();
    }

    // Disable Nagle algorithm
    if (SOCKET_ERROR == setsockopt(
        handle,
        IPPROTO_TCP,
        TCP_NODELAY,
        (char *) &yes,
        sizeof yes
    )) {
        logMsgError() << "setsockopt(TCP_NODELAY): " << wsaError();
    }

    // Allow some data to be sent during the initial handshake. [RFC 7413]
    if (SOCKET_ERROR == setsockopt(
        handle,
        IPPROTO_TCP,
        TCP_FASTOPEN,
        (char *) &yes,
        sizeof yes
    )) {
        if (IsWindowsVersionOrGreater(10, 0, 1607))
            logMsgDebug() << "setsockopt(TCP_FASTOPEN): " << wsaError();
    }

    return handle;
}

//===========================================================================
SOCKET Dim::iSocketCreate(const SockAddr & addr) {
    SOCKET handle = iSocketCreate();
    if (handle == INVALID_SOCKET)
        return handle;

    sockaddr_storage sas;
    copy(&sas, addr);
    if (SOCKET_ERROR == ::bind(handle, (sockaddr *)&sas, sizeof sas)) {
        auto err = wsaError();
        logMsgError() << "bind(" << addr << "): " << err;
        closesocket(handle);
        return INVALID_SOCKET;
    }

    return handle;
}


/****************************************************************************
*
*   Public API
*
***/

TokenTable s_socketModeTbl({
    { ISocketNotify::kAccepting, "accepting" },
    { ISocketNotify::kConnecting, "connecting" },
    { ISocketNotify::kActive, "active" },
    { ISocketNotify::kClosing, "closing" },
    { ISocketNotify::kClosed, "closed" },
});

//===========================================================================
std::string_view Dim::toString(ISocketNotify::Mode mode) {
    return s_socketModeTbl.findName(mode, "UNKNOWN");
}

//===========================================================================
ISocketNotify::Mode Dim::socketGetMode(const ISocketNotify * notify) {
    iSocketCheckThread();
    return SocketBase::getMode(notify);
}

//===========================================================================
SocketInfo Dim::socketGetInfo(const ISocketNotify * notify) {
    iSocketCheckThread();
    return SocketBase::getInfo(notify);
}

//===========================================================================
void Dim::socketDisconnect(ISocketNotify * notify) {
    iSocketCheckThread();
    SocketBase::disconnect(notify);
}

//===========================================================================
void Dim::socketWrite(
    ISocketNotify * notify,
    unique_ptr<SocketBuffer> buffer,
    size_t bytes
) {
    iSocketCheckThread();
    SocketBase::write(notify, move(buffer), bytes);
}

//===========================================================================
void Dim::socketRead(ISocketNotify * notify) {
    iSocketCheckThread();
    SocketBase::read(notify);
}
