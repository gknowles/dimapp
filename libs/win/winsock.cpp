// Copyright Glen Knowles 2015 - 2017.
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

const int kInitialCompletionQueueSize = 100;
const int kInitialSendQueueSize = 10;


/****************************************************************************
*
*   Variables
*
***/

static RIO_EXTENSION_FUNCTION_TABLE s_rio;

static mutex s_mut;
static condition_variable s_modeCv; // when run mode changes to stopped
static RunMode s_mode{kRunStopped};
static WinEvent s_cqReady;
static RIO_CQ s_cq;
static int s_cqSize = 10; // kInitialCompletionQueueSize;
static int s_cqUsed;

static atomic_int s_numSockets;


/****************************************************************************
*
*   Helpers
*
***/

//===========================================================================
static void addCqUsed_LK(int delta) {
    s_cqUsed += delta;
    assert(s_cqUsed >= 0);

    int size = s_cqSize;
    if (s_cqUsed > s_cqSize) {
        size = max(s_cqSize * 3 / 2, s_cqUsed);
    } else if (s_cqUsed < s_cqSize / 3) {
        size = max(s_cqSize / 2, kInitialCompletionQueueSize);
    }
    if (size != s_cqSize) {
        if (!s_rio.RIOResizeCompletionQueue(s_cq, size)) {
            logMsgError() << "RIOResizeCompletionQueue(" << size
                          << "): " << WinError{};
        } else {
            s_cqSize = size;
        }
    }
}


/****************************************************************************
*
*   RIO dispatch thread
*
***/

//===========================================================================
static void rioDispatchThread() {
    static constexpr int kMaxResults = 100;
    RIORESULT results[kMaxResults] = {};
    ITaskNotify * tasks[size(results)] = {};

    // cppcheck-suppress variableScope
    int count = 0;

    for (;;) {
        unique_lock<mutex> lk{s_mut};
        if (s_mode == kRunStopping) {
            s_mode = kRunStopped;
            break;
        }

        count = s_rio.RIODequeueCompletion(
            s_cq, 
            results, 
            (ULONG)size(results)
        );
        if (count == RIO_CORRUPT_CQ)
            logMsgCrash() << "RIODequeueCompletion: " << WinError{};

        if (int error = s_rio.RIONotify(s_cq))
            logMsgCrash() << "RIONotify: " << WinError{};

        lk.unlock();

        for (int i = 0; i < count; ++i) {
            auto && rr = results[i];
            auto task = (ISocketRequestTaskBase *)rr.RequestContext;
            task->m_socket = (SocketBase *)rr.SocketContext;
            task->m_xferError = rr.Status;
            task->m_xferBytes = rr.BytesTransferred;
            tasks[i] = task;
        }

        if (count)
            taskPushEvent(tasks, count);

        s_cqReady.wait();
    }

    s_modeCv.notify_one();
}


/****************************************************************************
*
*   SocketReadTask
*
***/

//===========================================================================
void SocketReadTask::onTask() {
    m_socket->onRead();
    // task object is a member of SocketBase and will be deleted when the
    // socket is deleted
}


/****************************************************************************
*
*   SocketWriteTask
*
***/

//===========================================================================
void SocketWriteTask::onTask() {
    // deleted via containing list
    m_socket->onWrite(this);
}


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
    unique_lock<mutex> lk{s_mut};
    if (auto * sock = notify->m_socket)
        return sock->m_mode;

    return Mode::kInactive;
}

//===========================================================================
// static
void SocketBase::disconnect(ISocketNotify * notify) {
    unique_lock<mutex> lk{s_mut};
    if (notify->m_socket)
        notify->m_socket->hardClose_LK();
}

//===========================================================================
// static 
void SocketBase::setNotify(
    ISocketNotify * notify, 
    ISocketNotify * newNotify
) {
    unique_lock<mutex> lk{s_mut};
    if (notify->m_socket) {
        assert(!newNotify->m_socket);
        assert(notify == notify->m_socket->m_notify);
        newNotify->m_socket = notify->m_socket;
        notify->m_socket->m_notify = newNotify;
        notify->m_socket = nullptr;
    } else {
        newNotify->m_socket = nullptr;
        newNotify->onSocketDestroy();
    }
}

//===========================================================================
// static
void SocketBase::write(
    ISocketNotify * notify,
    unique_ptr<SocketBuffer> buffer,
    size_t bytes
) {
    assert(bytes <= buffer->capacity);
    unique_lock<mutex> lk{s_mut};
    SocketBase * sock = notify->m_socket;
    if (!sock)
        return;

    sock->queueWrite_LK(move(buffer), bytes);
}

//===========================================================================
SocketBase::SocketBase(ISocketNotify * notify)
    : m_notify(notify) 
{
    s_numSockets += 1;
}

//===========================================================================
SocketBase::~SocketBase() {
    lock_guard<mutex> lk{s_mut};
    if (m_notify) {
        m_notify->m_socket = nullptr;
        m_notify->onSocketDestroy();
    }

    hardClose_LK();

    if (m_maxSending)
        addCqUsed_LK(-(m_maxSending + kMaxReceiving));

    s_numSockets -= 1;
}

//===========================================================================
void SocketBase::hardClose_LK() {
    if (m_handle == INVALID_SOCKET)
        return;

    // force immediate close by enabling shutdown timeout and setting it to 0
    linger opt = {};
    opt.l_linger = true;
    setsockopt(m_handle, SOL_SOCKET, SO_LINGER, (char *)&opt, sizeof(opt));
    closesocket(m_handle);

    m_mode = Mode::kClosing;
    m_handle = INVALID_SOCKET;
    m_unsent.clear();
}

//===========================================================================
bool SocketBase::createQueue() {
    m_read.m_buffer = socketGetBuffer();

    // Leave room for extra trailing null. Although not reported in the count
    // a trailing null is added to the end of the buffer after every read. 
    // It doesn't cost much and makes parsing easier for some text protocol 
    // streams.
    int bytes = m_read.m_buffer->capacity - 1; 

    copy(&m_read.m_rbuf, *m_read.m_buffer, bytes);

    {
        unique_lock<mutex> lk{s_mut};

        // adjust size of completion queue if required
        m_maxSending = kInitialSendQueueSize;
        addCqUsed_LK(m_maxSending + kMaxReceiving);

        // create request queue
        m_rq = s_rio.RIOCreateRequestQueue(
            m_handle,
            kMaxReceiving, // max outstanding receive requests
            1,             // max receive buffers (must be 1)
            m_maxSending,  // max outstanding send requests
            1,             // max send buffers (must be 1)
            s_cq,          // receive completion queue
            s_cq,          // send completion queue
            this           // socket context
        );
        if (m_rq == RIO_INVALID_RQ) {
            logMsgError() << "RIOCreateRequestQueue: " << WinError{};
            return false;
        }

        m_mode = Mode::kActive;
        m_notify->m_socket = this;

        // start reading from socket
        queueRead_LK();
    }

    return true;
}

//===========================================================================
void SocketBase::onRead() {
    if (int bytes = m_read.m_xferBytes) {
        SocketData data;
        data.data = (char *)m_read.m_buffer->data;
        data.bytes = bytes;

        // included uncounted trailing null
        data.data[bytes] = 0;
        
        m_notify->onSocketRead(data);

        lock_guard<mutex> lk{s_mut};
        if (m_mode == Mode::kActive)
            return queueRead_LK();
        assert(m_mode == Mode::kClosing);
    }

    m_notify->onSocketDisconnect();
    unique_lock<mutex> lk{s_mut};
    m_mode = Mode::kClosed;
    if (m_sending.empty()) {
        lk.unlock();
        delete this;
    }
}

//===========================================================================
void SocketBase::queueRead_LK() {
    if (!s_rio.RIOReceive(
        m_rq,
        &m_read.m_rbuf,
        1, // number of RIO_BUFs (must be 1)
        0, // RIO_MSG_* flags
        &m_read
    )) {
        logMsgCrash() << "RIOReceive: " << WinError{};
    }
}

//===========================================================================
void SocketBase::onWrite(SocketWriteTask * task) {
    unique_lock<mutex> lk{s_mut};

    auto it = find_if(
        m_sending.begin(), 
        m_sending.end(), 
        [task](auto && val) { return &val == task; }
    );
    assert(it != m_sending.end());
    m_sending.erase(it);
    m_numSending -= 1;

    // already disconnected and this was the last unresolved write? delete
    if (m_mode == Mode::kClosed && m_sending.empty()) {
        lk.unlock();
        delete this;
        return;
    }

    queueWriteFromUnsent_LK();
}

//===========================================================================
void SocketBase::queueWrite_LK(unique_ptr<SocketBuffer> buffer, size_t bytes) {
    if (m_mode == Mode::kClosing || m_mode == Mode::kClosed)
        return;

    if (!m_unsent.empty()) {
        auto & back = m_unsent.back();
        if (int count = min(
            back.m_buffer->capacity - (int)back.m_rbuf.Length, 
            (int)bytes
        )) {
            memcpy(
                back.m_buffer->data + back.m_rbuf.Length, 
                buffer->data, 
                count
            );
            back.m_rbuf.Length += count;
            bytes -= count;
            if (bytes) {
                memmove(buffer->data, buffer->data + count, bytes);
            }
        }
    }

    if (bytes) {
        m_unsent.emplace_back();
        auto & task = m_unsent.back();
        copy(&task.m_rbuf, *buffer, bytes);
        task.m_buffer = move(buffer);
    }

    queueWriteFromUnsent_LK();
}

//===========================================================================
void SocketBase::queueWriteFromUnsent_LK() {
    while (m_numSending < m_maxSending && !m_unsent.empty()) {
        m_sending.splice(m_sending.end(), m_unsent, m_unsent.begin());
        m_numSending += 1;
        auto & task = m_sending.back();
        if (!s_rio.RIOSend(m_rq, &task.m_rbuf, 1, 0, &task)) {
            WinError err;
            logMsgCrash() << "RIOSend: " << err;
            m_sending.pop_back();
            m_numSending -= 1;
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

    unique_lock<mutex> lk{s_mut};
    s_mode = kRunStopping;

    // wait for dispatch thread to stop
    s_cqReady.signal();
    while (s_mode != kRunStopped)
        s_modeCv.wait(lk);

    // close windows sockets
    s_rio.RIOCloseCompletionQueue(s_cq);
    if (WSACleanup())
        logMsgError() << "WSACleanup: " << WinError{};
}


/****************************************************************************
*
*   Internal API
*
***/

static WSAPROTOCOL_INFOW s_protocolInfo;

//===========================================================================
void Dim::iSocketInitialize() {
    s_mode = kRunStarting;

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

    // create RIO completion queue
    RIO_NOTIFICATION_COMPLETION ctype = {};
    ctype.Type = RIO_EVENT_COMPLETION;
    ctype.Event.EventHandle = s_cqReady.nativeHandle();
    ctype.Event.NotifyReset = false;
    s_cq = s_rio.RIOCreateCompletionQueue(s_cqSize, &ctype);
    if (s_cq == RIO_INVALID_CQ)
        logMsgCrash() << "RIOCreateCompletionQueue: " << WinError{};

    // start RIO dispatch task
    taskPushOnce("RIO Dispatch", rioDispatchThread);

    s_mode = kRunRunning;
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
        &yes, sizeof yes,
        nullptr, 0, // output buffer, buffer size
        &bytes,     // bytes returned
        nullptr,    // overlapped
        nullptr     // completion routine
    )) {
        logMsgError() << "WSAIoctl(SIO_LOOPBACK_FAST_PATH): " << WinError{};
    }

    if (SOCKET_ERROR == setsockopt(
        handle, 
        SOL_SOCKET, 
        TCP_NODELAY, 
        (char *)&yes, 
        sizeof(yes)
    )) {
        logMsgError() << "WSAIoctl(TCP_NODELAY): " << WinError{};
    }

    if (SOCKET_ERROR == setsockopt(
        handle,
        SOL_SOCKET,
        SO_REUSE_UNICASTPORT,
        (char *)&yes,
        sizeof(yes)
    )) {
        if (SOCKET_ERROR == setsockopt(
            handle,
            SOL_SOCKET,
            SO_PORT_SCALABILITY,
            (char *)&yes,
            sizeof(yes)
        )) {
            logMsgError() << "setsockopt(SO_PORT_SCALABILITY): " << WinError{};
        }
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
    return SocketBase::getMode(notify);
}

//===========================================================================
void Dim::socketDisconnect(ISocketNotify * notify) {
    SocketBase::disconnect(notify);
}

//===========================================================================
void Dim::socketSetNotify(ISocketNotify * notify, ISocketNotify * newNotify) {
    SocketBase::setNotify(notify, newNotify);
}

//===========================================================================
void Dim::socketWrite(
    ISocketNotify * notify,
    unique_ptr<SocketBuffer> buffer,
    size_t bytes
) {
    SocketBase::write(notify, move(buffer), bytes);
}
