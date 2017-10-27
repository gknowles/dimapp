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

const int kReadQueueSize = 10;
const int kWriteQueueSize = 100;

const int kWriteCompletionQueueSize = 10 * kWriteQueueSize;


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
static RIO_CQ s_cq{RIO_INVALID_CQ};
static int s_cqSize = kWriteCompletionQueueSize;
static int s_cqUsed;

static atomic_int s_numSockets;

static auto & s_perfIncomplete = uperf("sock write bytes (incomplete)");
static auto & s_perfWaiting = uperf("sock write bytes (waiting)");


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
        size = max(s_cqSize / 2, kWriteCompletionQueueSize);
    }
    if (size != s_cqSize) {
        if (!s_rio.RIOResizeCompletionQueue(s_cq, size)) {
            logMsgError() << "RIOResizeCompletionQueue(" 
                << s_cqSize << " -> " << size << "): " << WinError{};
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
            (ULONG) size(results)
        );
        if (count == RIO_CORRUPT_CQ)
            logMsgCrash() << "RIODequeueCompletion: " << WinError{};

        if (int error = s_rio.RIONotify(s_cq))
            logMsgCrash() << "RIONotify: " << WinError{error};

        lk.unlock();

        for (int i = 0; i < count; ++i) {
            auto && rr = results[i];
            auto task = (ISocketRequestTaskBase *)rr.RequestContext;
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
*   ReadDequeueTask
*
***/

namespace {

class ReadCompletionTask : public IWinOverlappedNotify {
    void onTask() override;
};

} // namespace

static ReadCompletionTask s_readCqTask;

//===========================================================================
void ReadCompletionTask::onTask() {
    auto socket = static_cast<SocketBase *>(overlappedKey());
    socket->onReadQueue();
}


/****************************************************************************
*
*   SocketReadTask
*
***/

//===========================================================================
void SocketReadTask::onTask() {
    abort();
}


/****************************************************************************
*
*   SocketWriteTask
*
***/

//===========================================================================
void SocketWriteTask::onTask() {
    // This task - and maybe the socket - is deleted inside onWrite.
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
    if (!bytes)
        return;
    unique_lock<mutex> lk{s_mut};
    SocketBase * sock = notify->m_socket;
    if (!sock)
        return;

    lk.release();
    sock->queueWrite_UNLK(move(buffer), bytes);
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

    {
        lock_guard<mutex> lk{s_mut};
        if (m_notify) {
            notify = m_notify;
            m_notify->m_socket = nullptr;
        }

        hardClose_LK();

        if (m_maxWrites)
            addCqUsed_LK(-(m_maxWrites + m_maxReads));

        s_numSockets -= 1;
    }

    if (m_maxReads)
        s_rio.RIOCloseCompletionQueue(m_readCq);

    // release the lock before making the calling the notify
    if (notify)
        notify->onSocketDestroy();
}

//===========================================================================
void SocketBase::hardClose_LK() {
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
    m_prewrites.clear();
}

//===========================================================================
bool SocketBase::createQueue() {
    m_maxReads = kReadQueueSize;

    for (int i = 0; i < m_maxReads; ++i) {
        m_reads.link(new SocketReadTask);
        auto task = m_reads.back();
        task->m_socket = this;
        task->m_buffer = socketGetBuffer();

        // Leave room for extra trailing null. Although not reported in the
        // count a trailing null is added to the end of the buffer after every
        // read. It doesn't cost much and makes parsing easier for some text
        // protocol streams.
        int bytes = task->m_buffer->capacity - 1; 
        copy(&task->m_rbuf, *task->m_buffer, bytes);
    }

    // create completion queue for reads
    RIO_NOTIFICATION_COMPLETION ctype = {};
    ctype.Type = RIO_IOCP_COMPLETION;
    ctype.Iocp.IocpHandle = winIocpHandle();
    ctype.Iocp.CompletionKey = this;
    ctype.Iocp.Overlapped = &s_readCqTask.overlapped();
    m_readCq = s_rio.RIOCreateCompletionQueue(m_maxReads, &ctype);
    if (m_readCq == RIO_INVALID_CQ)
        logMsgCrash() << "RIOCreateCompletionQueue(readCq): " << WinError{};

    {
        unique_lock<mutex> lk{s_mut};

        // adjust size of shared completion queue if required
        m_maxWrites = kWriteQueueSize;
        addCqUsed_LK(m_maxWrites + m_maxReads);

        // create request queue
        m_rq = s_rio.RIOCreateRequestQueue(
            m_handle,
            m_maxReads,     // max outstanding receive requests
            1,              // max receive buffers (must be 1)
            m_maxWrites,   // max outstanding send requests
            1,              // max send buffers (must be 1)
            m_readCq,       // receive completion queue
            s_cq,           // send completion queue
            nullptr         // socket context
        );
        if (m_rq == RIO_INVALID_RQ) {
            logMsgError() << "RIOCreateRequestQueue: " << WinError{};
            return false;
        }

        m_mode = Mode::kActive;
        m_notify->m_socket = this;

        // start reading from socket
        for (auto && task : m_reads) 
            queueRead_LK(&task);
    }

    onReadQueue();
    return true;
}

//===========================================================================
void SocketBase::onReadQueue() {
    static constexpr int kMaxResults = 10;
    RIORESULT results[kMaxResults];
    int count = s_rio.RIODequeueCompletion(
        m_readCq,
        results,
        (ULONG) size(results)
    );
    if (count == RIO_CORRUPT_CQ)
        logMsgCrash() << "RIODequeueCompletion(readCq): " << WinError{};

    for (int i = 0; i < count; ++i) {
        auto && rr = results[i];
        auto task = (SocketReadTask *) rr.RequestContext;
        task->m_xferError = rr.Status;
        task->m_xferBytes = rr.BytesTransferred;
        
        // This task and the socket may be deleted inside onRead.
        if (!onRead(task)) 
            return;
    }

    if (int error = s_rio.RIONotify(m_readCq))
        logMsgCrash() << "RIONotify: " << WinError{error};
}

//===========================================================================
bool SocketBase::onRead(SocketReadTask * task) {
    int bytes = task->m_xferBytes;
    if (bytes) {
        SocketData data;
        data.data = (char *)task->m_buffer->data;
        data.bytes = bytes;

        // included uncounted trailing null
        data.data[bytes] = 0;
        
        m_notify->onSocketRead(data);
    }

    bool mustDelete = false;
    {
        unique_lock<mutex> lk{s_mut};
        if (bytes) {
            if (m_mode == Mode::kActive) {
                queueRead_LK(task);
                return true;
            }

            assert(m_mode == Mode::kClosing || m_mode == Mode::kClosed);
        }
        delete task;

        mustDelete = m_reads.empty() && m_writes.empty();

        if (m_mode != Mode::kClosed) {
            m_mode = Mode::kClosed;
            lk.unlock();
            m_notify->onSocketDisconnect();
        }
    }

    if (mustDelete) {
        delete this;
        return false;
    }
    return true;
}

//===========================================================================
void SocketBase::queueRead_LK(SocketReadTask * task) {
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
bool SocketBase::onWrite(SocketWriteTask * task) {
    unique_lock<mutex> lk{s_mut};

    auto bytes = task->m_rbuf.Length;
    delete task;
    m_numWrites -= 1;
    s_perfIncomplete -= bytes;
    m_bufInfo.incomplete -= bytes;

    // already disconnected and this was the last unresolved write? delete
    if (m_reads.empty() && m_writes.empty()) {
        lk.unlock();
        delete this;
        return false;
    }

    bool wasUnsent = !m_prewrites.empty();
    queueWriteFromPrewrites_LK();
    if (wasUnsent && m_prewrites.empty()    // data no longer waiting
        || !m_numWrites                 // send queue is empty
    ) {
        assert(!m_bufInfo.waiting);
        assert(m_numWrites || !m_bufInfo.incomplete);
        auto info = m_bufInfo;
        lk.unlock();
        m_notify->onSocketBufferChanged(info);
    }
    return true;
}

//===========================================================================
void SocketBase::queueWrite_UNLK(
    unique_ptr<SocketBuffer> buffer, 
    size_t bytes
) {
    assert(bytes);
    unique_lock<mutex> lk{s_mut, adopt_lock};
    if (m_mode == Mode::kClosing || m_mode == Mode::kClosed)
        return;

    s_perfWaiting += (unsigned) bytes;
    m_bufInfo.waiting += bytes;
    bool wasUnsent = !m_prewrites.empty();

    if (wasUnsent) {
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
        m_prewrites.link(new SocketWriteTask);
        auto task = m_prewrites.back();
        task->m_socket = this;
        task->m_buffer = move(buffer);
        copy(&task->m_rbuf, *task->m_buffer, bytes);
    }

    queueWriteFromPrewrites_LK();
    if (auto task = m_prewrites.back()) {
        if (task->m_qtime == TimePoint::min())
            task->m_qtime = Clock::now();
    } else if (!wasUnsent) {
        // data is now waiting
        assert(m_bufInfo.waiting <= bytes);
        auto info = m_bufInfo;
        lk.unlock();
        m_notify->onSocketBufferChanged(info);
    }
}

//===========================================================================
void SocketBase::queueWriteFromPrewrites_LK() {
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
