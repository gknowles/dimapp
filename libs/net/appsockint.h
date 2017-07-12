// Copyright Glen Knowles 2017.
// Distributed under the Boost Software License, Version 1.0.
//
// appsockint.h - dim net
#pragma once

#include "core/timer.h"
#include "net/socket.h"

#include <list>
#include <memory>
#include <string>
#include <string_view>

namespace Dim {


/****************************************************************************
*
*   IAppSocket
*
*   Used to make filters that implement stream encapsulation protocols such
*   as TLS or WebSocket.
*
***/

class IAppSocket {
public:
    struct UnmatchedInfo;
    class UnmatchedTimer;

public:
    static void disconnect(IAppSocketNotify * notify);
    static void write(IAppSocketNotify * notify, std::string_view data);
    static void write(IAppSocketNotify * notify, const CharBuf & data);
    static void write(
        IAppSocketNotify * notify, 
        std::unique_ptr<SocketBuffer> buffer, 
        size_t bytes
    );

public:
    // If constructed without a notify the incoming data will be matched 
    // against the registered listeners to find a factory and that factory 
    // will be used to create a notify. If the incoming data doesn't map to 
    // any known factory the socket is disconnected.
    IAppSocket() {}
    IAppSocket(std::unique_ptr<IAppSocketNotify> notify);

    virtual ~IAppSocket();

    virtual void disconnect() = 0;
    virtual void write(std::string_view data) = 0;
    virtual void write(std::unique_ptr<SocketBuffer> buffer, size_t bytes) = 0;

    bool notifyAccept(const AppSocketInfo & info);
    void notifyDisconnect();
    void notifyDestroy();
    void notifyRead(AppSocketData & data);

protected:
    AppSocketInfo m_accept;

private:
    friend class UnmatchedTimer;
    Duration checkTimeout_LK(TimePoint now);
    void setNotify(std::unique_ptr<IAppSocketNotify> notify);

    // set to {} when matching is completed, either successfully or 
    // unsuccessfully.
    std::list<UnmatchedInfo>::iterator m_pos;

    IAppSocketNotify * m_notify{nullptr};
};

struct IAppSocket::UnmatchedInfo {
    TimePoint expiration;
    std::string socketData;
    IAppSocket * notify{nullptr};
};

class IAppSocket::UnmatchedTimer : public ITimerNotify {
    Duration onTimer(TimePoint now) override;
};


/****************************************************************************
*
*   AppSocket notify
*
***/

void socketWrite(
    IAppSocketNotify * notify, 
    std::unique_ptr<SocketBuffer> buffer,
    size_t bytes
);

} // namespace
