// Copyright Glen Knowles 2015 - 2023.
// Distributed under the Boost Software License, Version 1.0.
//
// winsockint.h - dim windows platform
#pragma once

#include <list>
#include <memory>

namespace Dim {


/****************************************************************************
*
*   SocketBase
*
***/

struct SocketRequest;

class SocketBase : IWinOverlappedNotify {
public:
    using Mode = ISocketNotify::Mode;

public:
    static LPFN_ACCEPTEX s_AcceptEx;
    static LPFN_GETACCEPTEXSOCKADDRS s_GetAcceptExSockaddrs;
    static LPFN_CONNECTEX s_ConnectEx;

    static SocketBase::Mode getMode(const ISocketNotify * notify);
    static SocketInfo getInfo(const ISocketNotify * notify);
    static void disconnect(ISocketNotify * notify);
    static void write(
        ISocketNotify * notify,
        std::unique_ptr<SocketBuffer> buffer,
        size_t bytes
    );
    static void read(ISocketNotify * notify);

public:
    SocketBase(ISocketNotify * notify);
    virtual ~SocketBase();

    void hardClose();
    bool createQueue();
    void enableEvents();

protected:
    void mode(Mode mode);
    Mode mode() const { return m_mode; }

    ISocketNotify * m_notify = {};
    SOCKET m_handle = INVALID_SOCKET;
    SocketConnectInfo m_connInfo = {};

private:
    void onTask() override;

    // NOTE: If onRead or onWrite return false the socket has been deleted.
    //       Additionally the task is completed and may be deleted whether or
    //       not false is returned.
    bool onRead(SocketRequest * task);
    bool onWrite(SocketRequest * task);

    void requeueRead();
    void queueRead(SocketRequest * task);
    void queuePrewrite(std::unique_ptr<SocketBuffer> buffer, size_t bytes);
    void queueWrites();

    Mode m_mode = Mode::kInactive;
    TimePoint m_lastModeTime = {};

    RIO_RQ m_rq = {RIO_INVALID_RQ};
    RIO_CQ m_cq = {RIO_INVALID_CQ};

    // used by read requests
    List<SocketRequest> m_reads;
    int m_maxReads{0};
    List<SocketRequest> m_prereads;
    TimePoint m_lastReadTime = {};   // When read was last received.
    size_t m_bytesRead = 0;

    // used by write requests
    List<SocketRequest> m_writes;
    int m_numWrites{0};
    int m_maxWrites{0};
    List<SocketRequest> m_prewrites;
    TimePoint m_lastWriteTime = {};  // When write was last sent.
    SocketBufferInfo m_bufInfo = {};
};


/****************************************************************************
*
*   Internal API
*
***/

void iSocketAcceptInitialize();
void iSocketConnectInitialize();

void iSocketCheckThread();
SOCKET iSocketCreate();
SOCKET iSocketCreate(const SockAddr & local);
void iSocketSetConnectTimeout(SOCKET s, Duration wait);

// Socket buffers
void iSocketBufferInitialize(RIO_EXTENSION_FUNCTION_TABLE & rio);

void copy(RIO_BUF * out, const SocketBuffer & buf, size_t bytes);

WinError wsaError();

} // namespace
