// Copyright Glen Knowles 2018 - 2022.
// Distributed under the Boost Software License, Version 1.0.
//
// pipe.h - dim net
#pragma once

#include "cppconf/cppconf.h"

#include "basic/types.h"
#include "core/time.h"

#include <memory>
#include <string_view>

namespace Dim {

namespace Pipe {
    enum OpenMode : unsigned {
        // content access, one *must* be specified
        fReadOnly = 0x1,
        fWriteOnly = 0x2,
        fReadWrite = 0x4,

        // Used with pipeConnect, access level granted to remote server
        fSecurityAnonymous = 0x8,
        fSecurityIdentification = 0x10, // default
        fSecurityImpersonation = 0x20,
        fSecurityDelegation = 0x40,
    };
};

// incomplete/waiting
//   0 /  0   All bytes left the application, but there might still be some
//              in OS or hardware memory buffers.
//  >0 /  0   Data is in flight, but more can still be sent immediately.
//  >0 / >0   There is data both in flight and waiting. Acknowledgment must
//              come from peer before more can be sent.
struct PipeBufferInfo {
    size_t incomplete;  // bytes sent but not yet ACK'd by peer
    size_t waiting;     // bytes waiting to be sent
    size_t total;       // total bytes sent to pipe
};

class PipeBase;

class IPipeNotify {
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
    virtual ~IPipeNotify() = default;

    //-----------------------------------------------------------------------
    // for connectors
    virtual void onPipeConnect() {};
    virtual void onPipeConnectFailed() {};

    //-----------------------------------------------------------------------
    // for listeners
    // Returns true if the pipe is accepted
    virtual bool onPipeAccept() { return true; };

    //-----------------------------------------------------------------------
    // for both
    virtual void onPipeDisconnect() {};

    // Returns true to immediately queue up another read, or false and calls
    // pipeQueueRead later. For every time false is returned pipeQueueRead
    // MUST be called exactly once.
    virtual bool onPipeRead(size_t * bytesUsed, std::string_view data) = 0;

    // Called when incomplete falls to zero or waiting transitions between
    // zero and non-zero.
    //
    // Method invoked inside the pipeWrite() call that causes the waiting
    // bytes to exceed zero. Runs as an asynchronous event task when the
    // buffers empty.
    virtual void onPipeBufferChanged(const PipeBufferInfo & info) {}

private:
    friend class PipeBase;
    std::shared_ptr<PipeBase> m_pipe;
};

IPipeNotify::Mode pipeGetMode(IPipeNotify * notify);
void pipeClose(IPipeNotify * notify);


/****************************************************************************
*
*   Connecting pipes
*
*   The application calls pipeConnect(...) and then it gets callbacks, if the
*   connection succeeds:
*       1. onPipeConnect
*       2. called any number of times
*           a. onPipeRead
*           b. onPipeBufferChanged
*       3. onPipeDisconnect
*
*   If the connection fails:
*       1. onPipeDisconnect
*
***/

//===========================================================================
// connect
//===========================================================================
// The data, if present, is sent if connect succeeds.
void pipeConnect(
    IPipeNotify * notify,
    std::string_view pipeName,
    Pipe::OpenMode oflags,
    std::string_view data = {},
    Duration timeout = {} // 0 for default timeout
);

// For simple cases a client connecting to a pipe server can also use
// fileOpen(pipeName, ...), fileRead, and fileWrite.


/****************************************************************************
*
*   Listening pipes
*
*   The application calls pipeListen() and then:
*
*   When a remote client connects:
*       1. The notifier is constructed using the factory.
*       2. onPipeAccept and then, if accepted, called any number of times:
*           a. onPipeRead
*           b. onPipeBufferChanged
*       3. onPipeDisconnect
*
***/

//===========================================================================
// listen
//===========================================================================
void pipeListen(
    IPipeNotify * notify,
    std::string_view pipeName,
    EnumFlags<Pipe::OpenMode> oflags,
    TaskQueueHandle hq = {}
);
void pipeListen(
    IFactory<IPipeNotify> * factory,
    std::string_view pipeName,
    EnumFlags<Pipe::OpenMode> oflags
);
void pipeClose(IFactory<IPipeNotify> * factory, std::string_view pipeName);

//===========================================================================
template <typename S>
requires std::derived_from<S, IPipeNotify>
inline void pipeListen(
    std::string_view pipeName,
    EnumFlags<Pipe::OpenMode> oflags
) {
    auto factory = getFactory<IPipeNotify, S>();
    pipeListen(factory, pipeName, oflags);
}

//===========================================================================
template <typename S>
requires std::derived_from<S, IPipeNotify>
inline void pipeCloseWait(std::string_view pipeName) {
    auto factory = getFactory<IPipeNotify, S>();
    pipeCloseWait(factory, pipeName);
}

//===========================================================================
// read/write
//===========================================================================

// Queues up a read on the pipe. Only allowed (and required!) after the
// notifier has returned false from its onPipeRead handler.
void pipeRead(IPipeNotify * notify);

void pipeWrite(IPipeNotify * notify, std::string_view data);

} // namespace
