// Copyright Glen Knowles 2015 - 2017.
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

class SocketRequest;

class SocketBase {
public:
    using Mode = ISocketNotify::Mode;

public:
    static LPFN_ACCEPTEX s_AcceptEx;
    static LPFN_GETACCEPTEXSOCKADDRS s_GetAcceptExSockaddrs;
    static LPFN_CONNECTEX s_ConnectEx;

    static SocketBase::Mode getMode(ISocketNotify * notify);
    static void disconnect(ISocketNotify * notify);
    static void setNotify(ISocketNotify * notify, ISocketNotify * newNotify);
    static void write(
        ISocketNotify * notify,
        std::unique_ptr<SocketBuffer> buffer,
        size_t bytes
    );

public:
    SocketBase(ISocketNotify * notify);
    virtual ~SocketBase();

    void hardClose_LK();

    bool createQueue();
    void onReadQueue();

    // NOTE: If onRead or onWrite return false the socket has been deleted.
    //       The task is completed and may be deleted whether or not false
    //       is returned.
    bool onRead(SocketRequest * task);
    bool onWrite(SocketRequest * task);

    void queueRead_LK(SocketRequest * task);
    void queueWrite_UNLK(std::unique_ptr<SocketBuffer> buffer, size_t bytes);
    void queueWriteFromPrewrites_LK();

protected:
    ISocketNotify * m_notify{nullptr};
    SOCKET m_handle{INVALID_SOCKET};
    SocketInfo m_connInfo;
    Mode m_mode{Mode::kInactive};

private:
    RIO_RQ m_rq{RIO_INVALID_RQ};
    SocketBufferInfo m_bufInfo{};

    // used by read requests
    List<SocketRequest> m_reads;
    int m_maxReads{0};
    RIO_CQ m_readCq{RIO_INVALID_CQ};

    // used by write requests
    List<SocketRequest> m_writes;
    int m_numWrites{0};
    int m_maxWrites{0};
    List<SocketRequest> m_prewrites;
};


/****************************************************************************
*
*   Internal API
*
***/

SOCKET iSocketCreate();
SOCKET iSocketCreate(const Endpoint & local);

void iSocketAcceptInitialize();
void iSocketConnectInitialize();

// Socket buffers
void iSocketBufferInitialize(RIO_EXTENSION_FUNCTION_TABLE & rio);

void copy(RIO_BUF * out, const SocketBuffer & buf, size_t bytes);

} // namespace
