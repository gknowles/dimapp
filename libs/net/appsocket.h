// Copyright Glen Knowles 2017.
// Distributed under the Boost Software License, Version 1.0.
//
// appsocket.h - dim net
#pragma once

#include "cppconf/cppconf.h"

#include "core/charbuf.h"
#include "core/types.h"
#include "core/util.h"

#include <functional>
#include <memory>
#include <string_view>

namespace Dim {


/****************************************************************************
*
*   Declarations
*
***/

namespace AppSocket {
    enum Family { 
        kTls, 
        kHttp2, 
        kRaw,
        kNumFamilies,
    };
}

struct AppSocketInfo {
    AppSocket::Family fam;
    std::string_view type;
    Endpoint remote;
    Endpoint local;
};
struct AppSocketData {
    char * data;
    int bytes;
};


/****************************************************************************
*
*   AppSocket notify
*
***/

class IAppSocketNotify {
public:
    virtual ~IAppSocketNotify () {}

    // for connectors
    virtual void onSocketConnect (const AppSocketInfo & info) {};
    virtual void onSocketConnectFailed (){};

    // for listeners
    // returns true if the socket is accepted
    virtual bool onSocketAccept (const AppSocketInfo & info) { return true; };

    // for both
    virtual void onSocketRead (const AppSocketData & data) = 0;
    virtual void onSocketDisconnect () {};
    virtual void onSocketDestroy () { delete this; }

private:
    friend class IAppSocket;
    IAppSocket * m_socket{nullptr};
};

void socketDisconnect(IAppSocketNotify * notify);
void socketWrite(IAppSocketNotify * notify, std::string_view data);
void socketWrite(IAppSocketNotify * notify, const CharBuf & data);


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
    virtual ~IAppSocketMatchNotify() {}

    virtual AppSocket::MatchType OnMatch(
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
    const Endpoint & end,
    AppSocket::Family fam
);
void socketCloseWait(
    IFactory<IAppSocketNotify> * factory,
    const Endpoint & end,
    AppSocket::Family fam
);

//===========================================================================
// Add and remove listeners with implicitly created factories. Implemented
// as templates where the template parameter is the class, derived from 
// IAppSocketNotify, that will be instantiated for incoming connections.
//===========================================================================
template <typename S> inline 
std::enable_if_t<std::is_base_of_v<IAppSocketNotify, S>, void> socketListen(
    const Endpoint & end,
    AppSocket::Family fam
) {
    auto factory = getFactory<IAppSocketNotify, S>();
    socketListen(factory, end, fam);
}

//===========================================================================
template <typename S> inline 
std::enable_if_t<std::is_base_of_v<IAppSocketNotify, S>, void> socketCloseWait(
    const Endpoint & end,
    AppSocket::Family fam
) {
    auto factory = getFactory<IAppSocketNotify, S>();
    socketCloseWait(factory, end, fam);
}


/****************************************************************************
*
*   AppSocket filter
*
*   Filters are like listeners except that they only apply to endpoints that
*   are already being listened to, rather than creating additional bindings. 
*
***/

void socketAddFilter(
    IFactory<IAppSocketNotify> * factory,
    const Endpoint & end,
    AppSocket::Family fam
);

//===========================================================================
template <typename S> inline 
std::enable_if_t<std::is_base_of_v<IAppSocketNotify, S>, void> socketAddFilter(
    const Endpoint & end,
    AppSocket::Family fam
) {
    auto factory = getFactory<IAppSocketNotify, S>();
    socketAddFilter(factory, end, fam);
}

} // namespace
