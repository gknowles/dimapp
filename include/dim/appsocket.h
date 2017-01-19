// appsocket.h - dim core
#pragma once

#include "config.h"

#include "socket.h"
#include "types.h"

#include <functional>
#include <memory>

namespace Dim {


/****************************************************************************
*
*   Declarations
*
***/

class AppSocket {
public:
    enum Family { kTls, kHttp2, kByte };
};

class IAppSocketNotify {
public:
    IAppSocketNotify(AppSocket & sock);
    virtual ~IAppSocketNotify() {}

private:
    AppSocket & m_socket;
};

class IAppSocketMatchNotify {
public:
    virtual ~IAppSocketMatchNotify() {}

    // Exclusive matches only if it's unambiguously in the family, connection
    // families that can match anything (such as byte stream) should only
    // return true when exclusive is false.
    virtual bool OnMatch(
        AppSocket::Family fam,
        const char ptr[],
        size_t count,
        bool exclusive) = 0;
};

class IAppSocketNotifyFactory {
public:
    virtual ~IAppSocketNotifyFactory() {}
    virtual std::unique_ptr<IAppSocketNotify> create(AppSocket & sock) = 0;
};


/****************************************************************************
*
*   AppSocket listen
*
***/

void appSocketAddMatch(IAppSocketMatchNotify * notify, AppSocket::Family fam);

void appSocketAddListen(
    IAppSocketNotifyFactory * factory,
    AppSocket::Family fam,
    const std::string & type,
    Endpoint end);

void appSocketRemoveListen(
    IAppSocketNotifyFactory * factory,
    AppSocket::Family fam,
    const std::string & type,
    Endpoint end);

//===========================================================================
template <typename S>
inline void appSocketUpdateListen(
    bool add,
    AppSocket::Family fam,
    const std::string & type,
    Endpoint end) {
    static class Factory : public IAppSocketNotifyFactory {
        std::unique_ptr<IAppSocketNotify> create(AppSocket & sock) override {
            return std::make_unique<S>(sock);
        }
    } s_factory;
    if (add) {
        appSocketAddListen(&s_factory, fam, type, end);
    } else {
        appSocketRemoveListen(&s_factory, fam, type, end);
    }
}

//===========================================================================
template <typename S>
inline void appSocketAddListen(
    AppSocket::Family fam,
    const std::string & type,
    Endpoint end) {
    appSocketUpdateListen<S>(true, fam, type, end);
}

//===========================================================================
template <typename S>
inline void appSocketRemoveListen(
    AppSocket::Family fam,
    const std::string & type,
    Endpoint end) {
    appSocketUpdateListen<S>(false, fam, type, end);
}

} // namespace
