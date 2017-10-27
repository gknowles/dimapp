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

class ISocketRequestTaskBase : public ITaskNotify, public ListBaseLink<> {
public:
    RIO_BUF m_rbuf{};
    std::unique_ptr<SocketBuffer> m_buffer;

    // filled in after completion
    WinError m_xferError{(WinError::NtStatus)0};
    int m_xferBytes{0};
    SocketBase * m_socket{nullptr};

private:
    void onTask() override = 0;
};

class SocketReadTask : public ISocketRequestTaskBase {
    void onTask() override;
};

class SocketWriteTask : public ISocketRequestTaskBase {
public:
    TimePoint m_qtime{};

private:
    void onTask() override;
};

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

    // NOTE: If onRead or onWrite return false you must immediately delete 
    //       this socket.
    bool onRead(SocketReadTask * task);
    bool onWrite(SocketWriteTask * task);

    void queueRead_LK(SocketReadTask * task);
    void queueWrite_UNLK(std::unique_ptr<SocketBuffer> buffer, size_t bytes);
    void queueWriteFromUnsent_LK();

protected:
    ISocketNotify * m_notify{nullptr};
    SOCKET m_handle{INVALID_SOCKET};
    SocketInfo m_connInfo;
    Mode m_mode{Mode::kInactive};

private:
    RIO_RQ m_rq{RIO_INVALID_RQ};

    // used by single read request
    List<SocketReadTask> m_reads;
    int m_maxReads{0};
    RIO_CQ m_readCq{RIO_INVALID_CQ};

    // used by write requests
    List<SocketWriteTask> m_sending;
    int m_numSending{0};
    int m_maxSending{0};
    List<SocketWriteTask> m_unsent;
    SocketBufferInfo m_bufInfo{};
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
