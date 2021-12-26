// Copyright Glen Knowles 2017 - 2021.
// Distributed under the Boost Software License, Version 1.0.
//
// appsocket.h - dim net
#pragma once

#include "cppconf/cppconf.h"

#include "core/charbuf.h"
#include "core/time.h"
#include "core/types.h"

#include <functional>
#include <memory>
#include <string_view>
#include <type_traits>

namespace Dim {


/****************************************************************************
*
*   Declarations
*
***/

namespace AppSocket {
    enum Family {
        kInvalid,
        kTls,
        kHttp1,
        kHttp2,
        kRaw,
        kNumFamilies,
    };
}

struct AppSocketInfo {
    AppSocket::Family fam;
    SockAddr remote;
    SockAddr local;
};
struct AppSocketData {
    char * data;
    int bytes;
};
struct AppSocketBufferInfo {
    size_t incomplete;
    size_t waiting;
    size_t total;
};


/****************************************************************************
*
*   AppSocket notify
*
***/

class IAppSocketNotify {
public:
    virtual ~IAppSocketNotify () = default;

    // for connectors
    virtual void onSocketConnect(const AppSocketInfo & info) {};
    virtual void onSocketConnectFailed() {};
    virtual void onSocketPingRequired() {};

    // for listeners
    // Returns true if the socket is accepted, false to disconnect
    virtual bool onSocketAccept(const AppSocketInfo & info) { return true; };

    // for both
    virtual void onSocketDisconnect() {};
    virtual void onSocketDestroy() { delete this; }
    virtual bool onSocketRead(AppSocketData & data) = 0;
    virtual void onSocketBufferChanged(const AppSocketBufferInfo & info) {}

private:
    friend class IAppSocket;
    IAppSocket * m_socket = {};
};

void socketDisconnect(IAppSocketNotify * notify);
void socketWrite(IAppSocketNotify * notify, std::string_view data);
void socketWrite(IAppSocketNotify * notify, const CharBuf & data);
void socketRead(IAppSocketNotify * notify);


/****************************************************************************
*
*   AppSocket connect
*
***/

void socketConnect(
    IAppSocketNotify * notify,
    const SockAddr & remote,
    const SockAddr & local = {},
    std::string_view initialData = {},
    Duration timeout = {}
);


/****************************************************************************
*
*   AppSocket family
*
***/

namespace AppSocket {
    enum MatchType {
        kUnknown,       // not enough data to know
        kPreferred,     // explicitly declared for protocol family
        kSupported,     // supported in a generic way (e.g. as byte stream)
        kUnsupported,   // not valid for protocol family
    };
} // namespace

class IAppSocketMatchNotify {
public:
    virtual ~IAppSocketMatchNotify() = default;

    virtual AppSocket::MatchType onMatch(
        AppSocket::Family fam,
        std::string_view view
    ) = 0;
};

void socketAddFamily(AppSocket::Family fam, IAppSocketMatchNotify * notify);


/****************************************************************************
*
*   AppSocket listen
*
***/

void socketListen(
    IFactory<IAppSocketNotify> * factory,
    const SockAddr & addr,
    AppSocket::Family fam,
    bool console = false // console connections for monitoring
);
void socketCloseWait(
    IFactory<IAppSocketNotify> * factory,
    const SockAddr & end,
    AppSocket::Family fam
);

//===========================================================================
// Add and remove listeners with implicitly created factories. Implemented
// as templates where the template parameter is the class, derived from
// IAppSocketNotify, that will be instantiated for incoming connections.
//===========================================================================
template <typename S>
requires std::derived_from<S, IAppSocketNotify>
inline void socketListen(
    const SockAddr & addr,
    AppSocket::Family fam,
    bool console = false
) {
    auto factory = getFactory<IAppSocketNotify, S>();
    socketListen(factory, addr, fam, console);
}

//===========================================================================
template <typename S>
requires std::derived_from<S, IAppSocketNotify>
inline void socketCloseWait(const SockAddr & addr, AppSocket::Family fam) {
    auto factory = getFactory<IAppSocketNotify, S>();
    socketCloseWait(factory, addr, fam);
}


/****************************************************************************
*
*   AppSocket filter
*
*   Factory filters are like listeners except they only apply to addresses
*   that are already being listened to, rather than creating additional
*   bindings.
*
***/

void socketAddFilter(
    IFactory<IAppSocketNotify> * factory,
    const SockAddr & addr,
    AppSocket::Family fam
);

//===========================================================================
template <typename S>
requires std::derived_from<S, IAppSocketNotify>
inline void socketAddFilter(const SockAddr & addr, AppSocket::Family fam) {
    auto factory = getFactory<IAppSocketNotify, S>();
    socketAddFilter(factory, addr, fam);
}

// Add socket filter to already created socket
void socketAddFilter(IAppSocketNotify * notify, AppSocket::Family fam);

} // namespace
