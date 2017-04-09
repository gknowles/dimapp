// Copyright Glen Knowles 2015 - 2017.
// Distributed under the Boost Software License, Version 1.0.
//
// socket.h - dim net
#pragma once

#include "cppconf/cppconf.h"

#include "core/types.h"

#include <memory>

namespace Dim {

struct SocketInfo {
    Endpoint remote;
    Endpoint local;
};
struct SocketData {
    char * data;
    int bytes;
};

class ISocketNotify {
public:
    enum Mode {
        kInactive, // not connected
        kAccepting,
        kConnecting,
        kActive,  // actively reading
        kClosing, // closed the handle
        kClosed,  // final zero-length read received
    };

public:
    virtual ~ISocketNotify() {}

    // for connectors
    virtual void onSocketConnect (const SocketInfo & info) {};
    virtual void onSocketConnectFailed () {};

    // for listeners
    // returns true if the socket is accepted
    virtual bool onSocketAccept (const SocketInfo & info) { return true; };

    // for both
    virtual void onSocketRead (const SocketData & data) = 0;
    virtual void onSocketDisconnect () {};
    virtual void onSocketDestroy () { delete this; }

private:
    friend class SocketBase;
    SocketBase * m_socket{nullptr};
};

ISocketNotify::Mode socketGetMode(ISocketNotify * notify);
void socketDisconnect(ISocketNotify * notify);

// Unlinks notify and links newNotify to the socket, taking ownership of it. 
// If notify wasn't linked newNotify->onSocketDestroy() is called. 
void socketSetNotify(ISocketNotify * notify, ISocketNotify * newNotify);

//===========================================================================
// connect
//===========================================================================
void socketConnect(
    ISocketNotify * notify,
    const Endpoint & remote,
    const Endpoint & local,
    Duration timeout = {} // 0 for default timeout
);

//===========================================================================
// listen
//===========================================================================
class ISocketListenNotify {
public:
    virtual ~ISocketListenNotify() {}
    virtual void onListenStop(const Endpoint & local) = 0;
    virtual std::unique_ptr<ISocketNotify> onListenCreateSocket(
        const Endpoint & local
    ) = 0;
};
void socketListen(ISocketListenNotify * notify, const Endpoint & local);
void socketStop(ISocketListenNotify * notify, const Endpoint & local);

//===========================================================================
// write
//===========================================================================
struct SocketBuffer {
    char * data;
    int len;

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
