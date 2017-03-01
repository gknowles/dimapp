// appsocket.h - dim core
#pragma once

#include "config.h"

#include "socket.h"
#include "types.h"

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

    enum MatchType {
        kUnknown,       // not enough data to know
        kPreferred,     // explicitly declared for protocol family 
        kSupported,     // supported in a generic way (e.g. as byte stream)
        kUnsupported,   // not valid for protocol family
    };
}

class IAppSocketMatchNotify {
public:
    virtual ~IAppSocketMatchNotify() {}

    virtual AppSocket::MatchType OnMatch(
        AppSocket::Family fam,
        std::string_view view
    ) = 0;
};

class IAppSocketNotifyFactory {
public:
    virtual ~IAppSocketNotifyFactory() {}
    virtual std::unique_ptr<ISocketNotify> create() = 0;
};


/****************************************************************************
*
*   AppSocket listen
*
***/

void appSocketAddMatch(IAppSocketMatchNotify * notify, AppSocket::Family fam);

void appSocketAddListener(
    IAppSocketNotifyFactory * factory,
    AppSocket::Family fam,
    const std::string & type,
    const Endpoint & end);

void appSocketRemoveListener(
    IAppSocketNotifyFactory * factory,
    AppSocket::Family fam,
    const std::string & type,
    const Endpoint & end);

//===========================================================================
// Add and remove listeners with implicitly created factories. Implemented
// as templates where the template parameter is the class, derived from 
// IAppSocketNotify, that will be instanciated for incoming connections.
template <typename S>
inline void appSocketUpdateListener(
    bool add,
    AppSocket::Family fam,
    const std::string & type,
    const Endpoint & end) {
    static class Factory : public IAppSocketNotifyFactory {
        std::unique_ptr<ISocketNotify> create() override {
            return std::make_unique<S>();
        }
    } s_factory;
    if (add) {
        appSocketAddListener(&s_factory, fam, type, end);
    } else {
        appSocketRemoveListener(&s_factory, fam, type, end);
    }
}

//===========================================================================
template <typename S>
inline void appSocketAddListener(
    AppSocket::Family fam,
    const std::string & type,
    const Endpoint & end) {
    appSocketUpdateListener<S>(true, fam, type, end);
}

//===========================================================================
template <typename S>
inline void appSocketRemoveListener(
    AppSocket::Family fam,
    const std::string & type,
    const Endpoint & end) {
    appSocketUpdateListener<S>(false, fam, type, end);
}

} // namespace
