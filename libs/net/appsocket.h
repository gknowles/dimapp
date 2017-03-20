// appsocket.h - dim core
#pragma once

#include "config/config.h"

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

    enum MatchType {
        kUnknown,       // not enough data to know
        kPreferred,     // explicitly declared for protocol family 
        kSupported,     // supported in a generic way (e.g. as byte stream)
        kUnsupported,   // not valid for protocol family
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
    virtual ~IAppSocketNotify() {}

    // for connectors
    virtual void onSocketConnect(const AppSocketInfo & info){};
    virtual void onSocketConnectFailed(){};

    // for listeners
    virtual void onSocketAccept(const AppSocketInfo & info){};

    // for both
    virtual void onSocketRead(const AppSocketData & data) = 0;
    virtual void onSocketDisconnect(){};
    virtual void onSocketDestroy() { delete this; }

private:
    friend class AppSocketBase;
    AppSocketBase * m_socket{nullptr};
};

void appSocketDisconnect(IAppSocketNotify * notify);
void appSocketWrite(IAppSocketNotify * notify, std::string_view data);
void appSocketWrite(IAppSocketNotify * notify, const CharBuf & data);


/****************************************************************************
*
*   AppSocket listen
*
***/

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
    virtual std::unique_ptr<IAppSocketNotify> create() = 0;
};

void appSocketAddMatch(IAppSocketMatchNotify * notify, AppSocket::Family fam);

void appSocketAddListener(
    IAppSocketNotifyFactory * factory,
    AppSocket::Family fam,
    std::string_view type,
    const Endpoint & end);

void appSocketRemoveListener(
    IAppSocketNotifyFactory * factory,
    AppSocket::Family fam,
    std::string_view type,
    const Endpoint & end);

//===========================================================================
// Add and remove listeners with implicitly created factories. Implemented
// as templates where the template parameter is the class, derived from 
// IAppSocketNotify, that will be instantiated for incoming connections.
template <typename S>
inline void appSocketUpdateListener(
    bool add,
    AppSocket::Family fam,
    std::string_view type,
    const Endpoint & end) {
    static class Factory : public IAppSocketNotifyFactory {
        std::unique_ptr<IAppSocketNotify> create() override {
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
    std::string_view type,
    const Endpoint & end) {
    appSocketUpdateListener<S>(true, fam, type, end);
}

//===========================================================================
template <typename S>
inline void appSocketRemoveListener(
    AppSocket::Family fam,
    std::string_view type,
    const Endpoint & end) {
    appSocketUpdateListener<S>(false, fam, type, end);
}

} // namespace
