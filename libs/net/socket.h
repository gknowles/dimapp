// Copyright Glen Knowles 2015 - 2020.
// Distributed under the Boost Software License, Version 1.0.
//
// socket.h - dim net
#pragma once

#include "cppconf/cppconf.h"

#include "core/time.h"
#include "core/types.h"

#include <memory>

namespace Dim {

struct SocketInfo {
    SockAddr remote;
    SockAddr local;
};
struct SocketData {
    char * data;
    int bytes;
};

// incomplete/pending
//   0 /  0   All bytes left the application, but there might still be some
//              in OS or hardware memory buffers.
//  >0 /  0   Data is in flight, but more can still be sent immediately.
//  >0 / >0   There is data both in flight and waiting. Acknowledgment must
//              come from peer before more can be sent.
struct SocketBufferInfo {
    size_t incomplete;  // bytes sent but not yet ACK'd by peer
    size_t waiting;     // bytes waiting to be sent
    size_t total;       // total bytes sent to socket
};

class SocketBase;

class ISocketNotify {
public:
    enum Mode {
        kInactive, // not connected
        kAccepting,
        kConnecting,
        kActive,  // actively reading
        kClosing, // closed the handle
        kClosed,  // final zero-length read and all completions received
    };

public:
    virtual ~ISocketNotify() = default;

    //-----------------------------------------------------------------------
    // for connectors
    //-----------------------------------------------------------------------
    virtual void onSocketConnect(const SocketInfo & info) {};
    virtual void onSocketConnectFailed() {};

    //-----------------------------------------------------------------------
    // for listeners
    //-----------------------------------------------------------------------
    // Returns true if the socket is accepted
    virtual bool onSocketAccept(const SocketInfo & info) { return true; };

    //-----------------------------------------------------------------------
    // for both
    //-----------------------------------------------------------------------
    // Every call to onSocketConnect or onSocketAccept will, eventually, have
    // exactly one corresponding call to onSocketDisconnect. If they are never
    // called neither is this.
    virtual void onSocketDisconnect() {};

    // Only override to control whether the object is deleted, such as for a
    // static notifier. Code to run on destruction should be in the destructor.
    virtual void onSocketDestroy() { delete this; }

    // Returns true to immediately queue up another read, or return false and
    // call socketQueueRead later. For every time false is returned
    // socketQueueRead MUST be called exactly once.
    //
    // Only called after onSocketConnect or onSocketAccept has been called,
    // and before onSocketDisconnect().
    virtual bool onSocketRead(SocketData & data) = 0;

    // Called when incomplete falls to zero or waiting transitions between
    // zero and non-zero.
    //
    // Method invoked inside the socketWrite() call that causes the waiting
    // bytes to exceed zero. Runs as an asynchronous event task when the
    // buffers empty.
    virtual void onSocketBufferChanged(const SocketBufferInfo & info) {}

private:
    friend class SocketBase;
    SocketBase * m_socket{};
};


/****************************************************************************
*
*   Connecting sockets
*
*   The application calls socketConnect(...) and then, if the connection
*   succeeds:
*       1. onSocketConnect
*       2. called any number of times
*           a. onSocketRead
*           b. onSocketBufferChanged
*       3. onSocketDisconnect
*       4. onSocketDestroy
*
*   If the connection fails:
*       1. onSocketConnectFailed
*       2. onSocketDestroy
*
***/

//===========================================================================
// The data, if present, is sent if connect succeeds. This is done via
// TCP_FASTOPEN if possible so try to keep the data <= 1460 bytes.
void socketConnect(
    ISocketNotify * notify,
    const SockAddr & remote,
    const SockAddr & local = {}, // 0 for OS selected
    std::string_view data = {},
    Duration timeout = {} // 0 for default timeout
);


/****************************************************************************
*
*   Listening sockets
*
*   The application calls socketListen() and then:
*
*   When a remote client connects:
*       1. The notifier is constructed using the factory.
*       2. onSocketAccept and then, if accepted, called any number of times:
*           a. onSocketRead
*           b. onSocketBufferChanged
*       3. onSocketDisconnect
*       4. onSocketDestroy
*
***/

//===========================================================================
void socketListen(IFactory<ISocketNotify> * factory, const SockAddr & local);
void socketCloseWait(IFactory<ISocketNotify> * factory, const SockAddr & local);

//===========================================================================
template <typename S>
inline void socketListen(const SockAddr & local) {
    static_assert(std::is_base_of_v<ISocketNotify, S>);
    auto factory = getFactory<ISocketNotify, S>();
    socketListen(factory, local);
}

//===========================================================================
template <typename S>
inline void socketCloseWait(const SockAddr & local) {
    static_assert(std::is_base_of_v<ISocketNotify, S>);
    auto factory = getFactory<ISocketNotify, S>();
    socketCloseWait(factory, local);
}


/****************************************************************************
*
*   Functions common to connecting and listening sockets
*
***/

ISocketNotify::Mode socketGetMode(ISocketNotify * notify);
void socketDisconnect(ISocketNotify * notify);

// Unlinks notify and links newNotify to the socket, taking ownership of it.
// If notify wasn't linked newNotify->onSocketDestroy() is called.
void socketSetNotify(ISocketNotify * notify, ISocketNotify * newNotify);

//===========================================================================
// read/write
//===========================================================================

// Queues up a read on the socket. Only allowed (and required!) after the
// notifier has returned false from its onSocketRead handler.
void socketRead(ISocketNotify * notify);

struct SocketBuffer {
    char * const data;
    const int capacity;

    // for internal use
    const int owner;

    SocketBuffer(char * const data, int capacity, int owner)
        : data{data}
        , capacity{capacity}
        , owner{owner}
    {}
    ~SocketBuffer();
};
std::unique_ptr<SocketBuffer> socketGetBuffer();

// Writes the data and deletes the buffer.
void socketWrite(
    ISocketNotify * notify,
    std::unique_ptr<SocketBuffer> buffer,
    size_t bytes
);

} // namespace
