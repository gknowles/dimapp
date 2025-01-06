// Copyright Glen Knowles 2017 - 2025.
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

namespace AppSocket {

enum class Disconnect {
    // The application called disconnect.
    kAppRequest,

    // Time expired before enough data was received to determine the
    // protocol of an inbound connection.
    kNoData,

    // Data received from inbound connection didn't match any registered
    // protocol.
    kUnknownProtocol,

    // Unable to decrypt cipher text
    kCryptError,

    // It's been too long since the far side has been heard from.
    kInactivity,
};

} // namespace


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
    static SocketInfo getInfo(const IAppSocketNotify * notify);
    static void disconnect(
        IAppSocketNotify * notify,
        AppSocket::Disconnect why
    );
    static void write(IAppSocketNotify * notify, std::string_view data);
    static void write(IAppSocketNotify * notify, const CharBuf & data);
    static void write(
        IAppSocketNotify * notify,
        std::unique_ptr<SocketBuffer> buffer,
        size_t bytes
    );
    static void read(IAppSocketNotify * notify);

public:
    // If constructed without a notify the incoming data will be matched
    // against the registered listeners to find a factory and that factory
    // will be used to create a notify. If the incoming data doesn't map to
    // any known factory the socket is disconnected.
    IAppSocket() = default;
    IAppSocket(IAppSocketNotify * notify);
    virtual ~IAppSocket();

    virtual SocketInfo getInfo() const = 0;
    virtual void disconnect(AppSocket::Disconnect why) = 0;
    virtual void write(std::string_view data) = 0;
    virtual void write(std::unique_ptr<SocketBuffer> buffer, size_t bytes) = 0;
    virtual void read() = 0;

    void notifyConnect(const AppSocketConnectInfo & info);
    void notifyConnectFailed();
    void notifyPingRequired();
    bool notifyAccept(const AppSocketConnectInfo & info);
    void notifyDisconnect();
    void notifyDestroy(bool deleteThis = true);
    bool notifyRead(AppSocketData & data);
    void notifyBufferChanged(const AppSocketBufferInfo & info);

protected:
    AppSocketConnectInfo m_accept;

private:
    Duration checkTimeout(TimePoint now);
    void setNotify(IAppSocketNotify * notify);

    // set to nullptr when matching is completed, either successfully or
    // unsuccessfully.
    UnmatchedInfo * m_pos = {};

    IAppSocketNotify * m_notify = {};
};

struct IAppSocket::UnmatchedInfo : ListLink<> {
    TimePoint expiration;
    std::string socketData;
    IAppSocket * notify = {};

    UnmatchedInfo();
};

class IAppSocket::UnmatchedTimer : public ITimerNotify {
    Duration onTimer(TimePoint now) override;
};


/****************************************************************************
*
*   AppSocket notify
*
***/

SocketInfo socketGetInfo(IAppSocketNotify * notify);

void socketDisconnect(IAppSocketNotify * notify, AppSocket::Disconnect why);

void socketWrite(
    IAppSocketNotify * notify,
    std::unique_ptr<SocketBuffer> buffer,
    size_t bytes
);

} // namespace
