// Copyright Glen Knowles 2015 - 2018.
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
const auto kMaxPrewriteQueueTime = 2min;


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

struct Dim::SocketRequest : ListBaseLink<> {
    RequestType m_type{kReqInvalid};
    RIO_BUF m_rbuf{};
    unique_ptr<SocketBuffer> m_buffer;
    TimePoint m_qtime{};

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

static auto & s_perfReadTotal = uperf("sock.read bytes (total)");
static auto & s_perfIncomplete = uperf("sock.write bytes (incomplete)");
static auto & s_perfWaiting = uperf("sock.write bytes (waiting)");
static auto & s_perfWriteTotal = uperf("sock.write bytes (total)");

// The data in the send queue is either too massive or too old.
static auto & s_perfBacklog = uperf("sock.disconnect (write backlog)");


/****************************************************************************
*
*   Helpers
*
***/


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
SocketBase::Mode SocketBase::getMode(ISocketNotify * notify) {
    if (auto sock = notify->m_socket)
        return sock->m_mode;

    return Mode::kInactive;
}

//===========================================================================
// static
void SocketBase::disconnect(ISocketNotify * notify) {
    if (auto sock = notify->m_socket)
        sock->hardClose();
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
// static
void SocketBase::read(ISocketNotify * notify) {
    if (auto sock = notify->m_socket) {
        auto task = sock->m_prereads.front();

        // Queuing reads is only allowed after an automatic requeuing was
        // rejected via returning false from onSocketRead.
        assert(task);

        sock->m_reads.link(task);
        sock->queueRead(task);
    }
}

//===========================================================================
SocketBase::SocketBase(ISocketNotify * notify)
    : m_notify(notify)
{
    s_numSockets += 1;
}

//===========================================================================
SocketBase::~SocketBase() {
    assert(m_reads.empty() && m_writes.empty());
    ISocketNotify * notify{nullptr};

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
    setsockopt(m_handle, SOL_SOCKET, SO_LINGER, (char *)&opt, sizeof(opt));
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
        logMsgCrash() << "RIOCreateCompletionQueue: " << WinError{};
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
        m_reads.link(new SocketRequest);
        auto task = m_reads.back();
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
    for (auto && task : m_reads)
        queueRead(&task);

    // trigger first RIONotify
    if (int error = s_rio.RIONotify(m_cq))
        logMsgCrash() << "RIONotify: " << WinError{error};
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
        logMsgCrash() << "RIODequeueCompletion: " << WinError{};

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
        logMsgCrash() << "RIONotify: " << WinError{error};
}

//===========================================================================
bool SocketBase::onRead(SocketRequest * task) {
    if (int bytes = task->m_xferBytes) {
        s_perfReadTotal += bytes;
        SocketData data;
        data.data = (char *)task->m_buffer->data;
        data.bytes = bytes;

        // included uncounted trailing null
        data.data[bytes] = 0;

        bool more = m_notify->onSocketRead(data);

        if (m_mode == Mode::kActive) {
            if (more) {
                queueRead(task);
            } else {
                m_prereads.link(task);
            }
            return true;
        }

        assert(m_mode == Mode::kClosing || m_mode == Mode::kClosed);
    }

    delete task;

    if (m_mode != Mode::kClosed) {
        m_mode = Mode::kClosed;
        m_notify->onSocketDisconnect();
    }

    if (m_reads.empty() && m_writes.empty()) {
        delete this;
        return false;
    }
    return true;
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
        logMsgCrash() << "RIOReceive: " << WinError{};
    }
}

//===========================================================================
bool SocketBase::onWrite(SocketRequest * task) {
    auto bytes = task->m_rbuf.Length;
    delete task;
    m_numWrites -= 1;
    s_perfIncomplete -= bytes;
    m_bufInfo.incomplete -= bytes;

    // already disconnected and this was the last unresolved write? delete
    if (m_mode == Mode::kClosed && m_reads.empty() && m_writes.empty()) {
        delete this;
        return false;
    }

    bool wasPrewrites = !m_prewrites.empty();
    queueWrites();
    if (wasPrewrites && m_prewrites.empty() // data no longer waiting
        || !m_numWrites                     // send queue is empty
    ) {
        assert(!m_bufInfo.waiting);
        assert(m_numWrites || !m_bufInfo.incomplete);
        auto info = m_bufInfo;
        m_notify->onSocketBufferChanged(info);
    }
    return true;
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
    s_perfWriteTotal += (unsigned) bytes;
    m_bufInfo.waiting += bytes;
    m_bufInfo.total += bytes;
    bool wasPrewrites = !m_prewrites.empty();

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
            auto now = Clock::now();
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
    while (m_numWrites < m_maxWrites && !m_prewrites.empty()) {
        m_writes.link(m_prewrites.front());
        m_numWrites += 1;
        auto task = m_writes.back();
        auto bytes = task->m_rbuf.Length;
        s_perfWaiting -= bytes;
        m_bufInfo.waiting -= bytes;
        if (!s_rio.RIOSend(m_rq, &task->m_rbuf, 1, 0, task)) {
            WinError err;
            logMsgCrash() << "RIOSend: " << err;
            delete task;
            m_numWrites -= 1;
        } else {
            s_perfIncomplete += bytes;
            m_bufInfo.incomplete += bytes;
        }
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
    if (s_numSockets)
        return shutdownIncomplete();

    // close windows sockets
    if (WSACleanup())
        logMsgError() << "WSACleanup: " << WinError{};
}


/****************************************************************************
*
*   Internal API
*
***/

thread_local bool t_socketInitialized;
static WSAPROTOCOL_INFOW s_protocolInfo;

//===========================================================================
void Dim::iSocketCheckThread() {
    if (!t_socketInitialized)
        logMsgCrash() << "Socket services must be called on the event thread";
}

//===========================================================================
void Dim::iSocketInitialize() {
    t_socketInitialized = true;

    WSADATA data = {};
    WinError err = WSAStartup(WINSOCK_VERSION, &data);
    if (err || data.wVersion != WINSOCK_VERSION) {
        logMsgCrash() << "WSAStartup(version=" << hex << WINSOCK_VERSION
            << "): " << err << ", version " << data.wVersion;
    }

    // get extension functions
    SOCKET s = WSASocketW(
        AF_UNSPEC,
        SOCK_STREAM,
        IPPROTO_TCP,
        NULL, // protocol info (additional creation options)
        0,    // socket group
        WSA_FLAG_REGISTERED_IO
    );
    if (s == INVALID_SOCKET)
        logMsgCrash() << "WSASocketW: " << WinError{};

    // get RIO functions
    GUID extId = WSAID_MULTIPLE_RIO;
    s_rio.cbSize = sizeof(s_rio);
    DWORD bytes;
    if (WSAIoctl(
        s,
        SIO_GET_MULTIPLE_EXTENSION_FUNCTION_POINTER,
        &extId,
        sizeof(extId),
        &s_rio,
        sizeof(s_rio),
        &bytes,
        nullptr, // overlapped
        nullptr  // completion routine
    )) {
        logMsgCrash() << "WSAIoctl(get RIO functions): " << WinError{};
    }

    // get AcceptEx function
    extId = WSAID_ACCEPTEX;
    if (WSAIoctl(
        s,
        SIO_GET_EXTENSION_FUNCTION_POINTER,
        &extId,
        sizeof(extId),
        &SocketBase::s_AcceptEx,
        sizeof(SocketBase::s_AcceptEx),
        &bytes,
        nullptr, // overlapped
        nullptr  // completion routine
    )) {
        logMsgCrash() << "WSAIoctl(get AcceptEx): " << WinError{};
    }

    extId = WSAID_GETACCEPTEXSOCKADDRS;
    if (WSAIoctl(
        s,
        SIO_GET_EXTENSION_FUNCTION_POINTER,
        &extId,
        sizeof(extId),
        &SocketBase::s_GetAcceptExSockaddrs,
        sizeof(SocketBase::s_GetAcceptExSockaddrs),
        &bytes,
        nullptr, // overlapped
        nullptr  // completion routine
    )) {
        logMsgCrash() << "WSAIoctl(get GetAcceptExSockAddrs): " << WinError{};
    }

    // get ConnectEx function
    extId = WSAID_CONNECTEX;
    if (WSAIoctl(
        s,
        SIO_GET_EXTENSION_FUNCTION_POINTER,
        &extId,
        sizeof(extId),
        &SocketBase::s_ConnectEx,
        sizeof(SocketBase::s_ConnectEx),
        &bytes,
        nullptr, // overlapped
        nullptr  // completion routine
    )) {
        logMsgCrash() << "WSAIoctl(get ConnectEx): " << WinError{};
    }

    int piLen = sizeof(s_protocolInfo);
    if (getsockopt(
        s,
        SOL_SOCKET,
        SO_PROTOCOL_INFOW,
        (char *) &s_protocolInfo,
        &piLen
    )) {
        logMsgCrash() << "getsockopt(SO_PROTOCOL_INFOW): " << WinError{};
    }
    if (~s_protocolInfo.dwServiceFlags1 & XP1_IFS_HANDLES) {
        logMsgCrash() << "socket and file handles not compatible";
    }

    closesocket(s);

    // initialize buffer allocator
    iSocketBufferInitialize(s_rio);
    // Don't register cleanup until all dependents (aka sockbuf) have
    // registered their cleanups (aka been initialized)
    shutdownMonitor(&s_cleanup);
    iSocketAcceptInitialize();
    iSocketConnectInitialize();
}

//===========================================================================
SOCKET Dim::iSocketCreate() {
    SOCKET handle = WSASocketW(
        AF_UNSPEC,
        SOCK_STREAM,
        IPPROTO_TCP,
        &s_protocolInfo,
        0,
        WSA_FLAG_REGISTERED_IO
    );
    if (handle == INVALID_SOCKET) {
        logMsgError() << "WSASocket: " << WinError{};
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

    DWORD bytes;
    if (SOCKET_ERROR == WSAIoctl(
        handle,
        SIO_LOOPBACK_FAST_PATH,
        &yes, sizeof(yes),
        nullptr, 0, // output buffer, buffer size
        &bytes,     // bytes returned
        nullptr,    // overlapped
        nullptr     // completion routine
    )) {
        logMsgError() << "WSAIoctl(SIO_LOOPBACK_FAST_PATH): " << WinError{};
    }

    if (SOCKET_ERROR == setsockopt(
        handle,
        IPPROTO_TCP,
        TCP_NODELAY,
        (char *) &yes,
        sizeof(yes)
    )) {
        logMsgError() << "setsockopt(TCP_NODELAY): " << WinError{};
    }

    if (SOCKET_ERROR == setsockopt(
        handle,
        SOL_SOCKET,
        SO_REUSE_UNICASTPORT,
        (char *) &yes,
        sizeof(yes)
    )) {
        if (SOCKET_ERROR == setsockopt(
            handle,
            SOL_SOCKET,
            SO_PORT_SCALABILITY,
            (char *) &yes,
            sizeof(yes)
        )) {
            logMsgError() << "setsockopt(SO_PORT_SCALABILITY): " << WinError{};
        }
    }

    if (SOCKET_ERROR == setsockopt(
        handle,
        IPPROTO_TCP,
        TCP_FASTOPEN,
        (char *) &yes,
        sizeof(yes)
    )) {
        if (IsWindowsVersionOrGreater(10, 0, 1607))
            logMsgDebug() << "setsockopt(TCP_FASTOPEN): " << WinError{};
    }

    return handle;
}

//===========================================================================
SOCKET Dim::iSocketCreate(const Endpoint & end) {
    SOCKET handle = iSocketCreate();
    if (handle == INVALID_SOCKET)
        return handle;

    sockaddr_storage sas;
    copy(&sas, end);
    if (SOCKET_ERROR == ::bind(handle, (sockaddr *)&sas, sizeof(sas))) {
        logMsgError() << "bind(" << end << "): " << WinError{};
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

//===========================================================================
ISocketNotify::Mode Dim::socketGetMode(ISocketNotify * notify) {
    iSocketCheckThread();
    return SocketBase::getMode(notify);
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
