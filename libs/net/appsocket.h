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
        kByte,
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
    friend class AppSocketBase;
    AppSocketBase * m_socket{nullptr};
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
    AppSocket::Family fam,
    std::string_view type,
    const Endpoint & end
);
void socketCloseWait(
    IFactory<IAppSocketNotify> * factory,
    AppSocket::Family fam,
    std::string_view type,
    const Endpoint & end
);

//===========================================================================
// Add and remove listeners with implicitly created factories. Implemented
// as templates where the template parameter is the class, derived from 
// IAppSocketNotify, that will be instantiated for incoming connections.
//===========================================================================
template <typename S> inline 
std::enable_if_t<std::is_base_of_v<IAppSocketNotify, S>, void> socketListen(
    AppSocket::Family fam,
    std::string_view type,
    const Endpoint & end
) {
    auto factory = getFactory<IAppSocketNotify, S>();
    socketListen(factory, fam, type, end);
}

//===========================================================================
template <typename S> inline 
std::enable_if_t<std::is_base_of_v<IAppSocketNotify, S>, void> socketCloseWait(
    AppSocket::Family fam,
    std::string_view type,
    const Endpoint & end
) {
    auto factory = getFactory<IAppSocketNotify, S>();
    socketCloseWait(factory, fam, type, end);
}


/****************************************************************************
*
*   AppSocket filter
*
***/

void socketAddFilter(
    IFactory<IAppSocketNotify> * factory,
    AppSocket::Family fam,
    std::string_view type
);

//===========================================================================
template <typename S> inline 
std::enable_if_t<std::is_base_of_v<IAppSocketNotify, S>, void> socketAddFilter(
    AppSocket::Family fam,
    std::string_view type
) {
    auto factory = getFactory<IAppSocketNotify, S>();
    socketAddFilter(factory, fam, type);
}

} // namespace
