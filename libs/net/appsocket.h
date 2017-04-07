// Copyright Glen Knowles 2017.
// Distributed under the Boost Software License, Version 1.0.
//
// appsocket.h - dim net
#pragma once

#include "cppconf/cppconf.h"

#include "core/charbuf.h"
#include "core/types.h"

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

class IAppSocketNotifyFactory {
public:
    virtual ~IAppSocketNotifyFactory() {}
    virtual std::unique_ptr<IAppSocketNotify> create() = 0;
};

void socketListen(
    IAppSocketNotifyFactory * factory,
    AppSocket::Family fam,
    std::string_view type,
    const Endpoint & end
);
void socketStop(
    IAppSocketNotifyFactory * factory,
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
std::enable_if_t<
    std::is_base_of_v<IAppSocketNotify, S>, 
    IAppSocketNotifyFactory*
>
socketFactory() {
    class Factory : public IAppSocketNotifyFactory {
        std::unique_ptr<IAppSocketNotify> create() override {
            return std::make_unique<S>();
        }
    };
    // As per http://en.cppreference.com/w/cpp/language/inline
    // "In an inline function, function-local static objects in all function
    // definitions are shared across all translation units (they all refer to
    // the same object defined in one translation unit)" 
    //
    // Note that this is a difference betwee C and C++
    static Factory s_factory;
    return &s_factory;
}

//===========================================================================
template <typename S> inline 
std::enable_if_t<std::is_base_of_v<IAppSocketNotify, S>, void> socketListen(
    AppSocket::Family fam,
    std::string_view type,
    const Endpoint & end
) {
    auto factory = socketFactory<S>();
    socketListen(factory, fam, type, end);
}

//===========================================================================
template <typename S> inline 
std::enable_if_t<std::is_base_of_v<IAppSocketNotify, S>, void> socketStop(
    AppSocket::Family fam,
    std::string_view type,
    const Endpoint & end
) {
    auto factory = socketFactory<S>();
    socketStop(factory, fam, type, end);
}

} // namespace
