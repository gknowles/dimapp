// Copyright Glen Knowles 2017 - 2024.
// Distributed under the Boost Software License, Version 1.0.
//
// appsocket.cpp - dim net
#include "pch.h"
#pragma hdrstop

using namespace std;
using namespace Dim;

// Forward declarations
namespace Dim {
struct SocketInfo;
} // namespace


/****************************************************************************
*
*   Tuning parameters
*
***/

constexpr Duration kUnmatchedTimeout = 10s;
constexpr Duration kUnmatchedMinWait = 2s;


/****************************************************************************
*
*   Declarations
*
***/

namespace {

enum FactoryFlags : unsigned {
    fListener = 0x01,
    fConsole = 0x02,
};

struct MatchKey {
    IAppSocketMatchNotify * notify{};
};

struct FactoryInfo {
    IFactory<IAppSocketNotify> * factory;
    EnumFlags<FactoryFlags> flags{};
};

struct FamilyInfo {
    AppSocket::Family family;
    vector<FactoryInfo> factories;

    bool operator==(AppSocket::Family fam) const { return fam == family; }
};

struct SockAddrInfo {
    SockAddr saddr;
    vector<FamilyInfo> families;
    unsigned listeners{0};
    unsigned consoles{0};
};

class RawSocket
    : public IAppSocket
    , public ISocketNotify
    , ITimerNotify
{
public:
    RawSocket() {}
    explicit RawSocket(IAppSocketNotify * notify);

    SocketInfo getInfo() const override;
    void disconnect(AppSocket::Disconnect why) override;
    void write(string_view data) override;
    void write(unique_ptr<SocketBuffer> buffer, size_t bytes) override;
    void read() override;

    // ISocketNotify
    void onSocketConnect(const SocketConnectInfo & info) override;
    void onSocketConnectFailed() override;
    bool onSocketAccept(const SocketConnectInfo & info) override;
    void onSocketDisconnect() override;
    void onSocketDestroy() override;
    bool onSocketRead(SocketData & data) override;
    void onSocketBufferChanged(const SocketBufferInfo & info) override;

private:
    Duration onTimer(TimePoint now) override;

    unique_ptr<SocketBuffer> m_buffer;
    size_t m_bufferUsed{0};
};

} // namespace


/****************************************************************************
*
*   Variables
*
***/

static vector<MatchKey> s_matchers;
static vector<SockAddrInfo> s_sockAddrs;

static List<IAppSocket::UnmatchedInfo> s_unmatched;
static IAppSocket::UnmatchedTimer s_unmatchedTimer;
static bool s_disableNoDataTimeout;

static auto & s_perfRequest = uperf("sock.disconnect (app request)");
static auto & s_perfNoData = uperf("sock.disconnect (no data)");
static auto & s_perfUnknown = uperf("sock.disconnect (unknown protocol)");
static auto & s_perfCrypt = uperf("sock.disconnect (crypt error)");
static auto & s_perfInactive = uperf("sock.disconnect (inactivity)");


/****************************************************************************
*
*   IAppSocket::UnmatchedInfo
*
***/

//===========================================================================
IAppSocket::UnmatchedInfo::UnmatchedInfo() {
    s_unmatched.link(this);
}


/****************************************************************************
*
*   IAppSocket::UnmatchedTimer
*
***/

//===========================================================================
Duration IAppSocket::UnmatchedTimer::onTimer(TimePoint now) {
    while (!s_unmatched.empty()) {
        auto info = s_unmatched.front();
        auto wait = info->notify->checkTimeout_LK(now);
        if (wait > 0s)
            return max(wait, kUnmatchedMinWait);
    }

    return kTimerInfinite;
}


/****************************************************************************
*
*   IAppSocket
*
***/

//===========================================================================
// static
SocketInfo IAppSocket::getInfo(const IAppSocketNotify * notify) {
    SocketInfo out = {};
    if (auto sock = notify->m_socket)
        out = sock->getInfo();
    return out;
}

//===========================================================================
// static
void IAppSocket::disconnect(
    IAppSocketNotify * notify,
    AppSocket::Disconnect why
) {
    if (auto sock = notify->m_socket)
        sock->disconnect(why);
}

//===========================================================================
// static
void IAppSocket::write(IAppSocketNotify * notify, string_view data) {
    if (auto sock = notify->m_socket)
        sock->write(data);
}

//===========================================================================
// static
void IAppSocket::write(IAppSocketNotify * notify, const CharBuf & data) {
    if (auto sock = notify->m_socket) {
        for (auto && v : data.views()) {
            sock->write(v);
        }
    }
}

//===========================================================================
// static
void IAppSocket::write(
    IAppSocketNotify * notify,
    unique_ptr<SocketBuffer> buffer,
    size_t bytes
) {
    if (auto sock = notify->m_socket)
        sock->write(move(buffer), bytes);
}

//===========================================================================
// static
void IAppSocket::read(IAppSocketNotify * notify) {
    if (auto sock = notify->m_socket)
        sock->read();
}

//===========================================================================
IAppSocket::IAppSocket(IAppSocketNotify * notify) {
    setNotify(notify);
}

//===========================================================================
IAppSocket::~IAppSocket() {
    assert(!m_pos);
    assert(!m_notify);
}

//===========================================================================
Duration IAppSocket::checkTimeout_LK(TimePoint now) {
    assert(!m_notify);
    auto wait = m_pos->expiration - now;
    if (wait <= 0s) {
        disconnect(AppSocket::Disconnect::kNoData);
        delete m_pos;
        m_pos = {};
    }
    return wait;
}

//===========================================================================
void IAppSocket::setNotify(IAppSocketNotify * notify) {
    assert(!m_notify && notify);
    m_notify = notify;
    m_notify->m_socket = this;
}

//===========================================================================
void IAppSocket::notifyConnect(const AppSocketConnectInfo & info) {
    m_accept = info;
    m_notify->onSocketConnect(m_accept);
}

//===========================================================================
void IAppSocket::notifyConnectFailed() {
    m_notify->onSocketConnectFailed();
}

//===========================================================================
void IAppSocket::notifyPingRequired() {
    m_notify->onSocketPingRequired();
}

//===========================================================================
bool IAppSocket::notifyAccept(const AppSocketConnectInfo & info) {
    m_accept = info;
    if (m_notify)
        return m_notify->onSocketAccept(m_accept);

    auto expiration = timeNow() + kUnmatchedTimeout;

    auto ui = new UnmatchedInfo;
    ui->notify = this;
    ui->expiration = expiration;
    m_pos = s_unmatched.back();

    if (!s_disableNoDataTimeout)
        timerUpdate(&s_unmatchedTimer, kUnmatchedTimeout, true);
    return true;
}

//===========================================================================
void IAppSocket::notifyDisconnect() {
    if (m_notify)
        m_notify->onSocketDisconnect();
    if (m_pos) {
        delete m_pos;
        m_pos = {};
    }
}

//===========================================================================
void IAppSocket::notifyDestroy(bool deleteThis) {
    if (m_notify) {
        m_notify->m_socket = nullptr;
        m_notify->onSocketDestroy();
        m_notify = nullptr;
    }
    if (deleteThis)
        delete this;
}

//===========================================================================
// Returns true if a factory is found or proven to not exist (nullptr).
// False means it should be tried again when there's more data.
static bool findFactory(
    AppSocket::Family * family,
    IFactory<IAppSocketNotify> ** fact,
    const SockAddr & localAddr,
    string_view data
) {
    // find best matching factory socket address for each family
    enum {
        // match types, in reverse priority order
        kUnknown,   // no match
        kWild,      // wildcard factory (0.0.0.0:0)
        kPort,      // matching port with wildcard address
        kAddr,      // matching address with wildcard port
        kExact,     // both address and port explicitly match
    };
    struct SockAddrKey {
        IFactory<IAppSocketNotify> * fact;
        int level;
    };
    vector<SockAddrKey> keys(s_matchers.size(), {nullptr, kUnknown});
    for (auto && info : s_sockAddrs) {
        int level{kUnknown};
        if (info.saddr == localAddr) {
            level = kExact;
        } else if (!info.saddr.addr) {
            if (info.saddr.port == localAddr.port) {
                level = kPort;
            } else if (!info.saddr.port) {
                level = kWild;
            }
        } else if (!info.saddr.port
            && info.saddr.addr == localAddr.addr
        ) {
            level = kAddr;
        }
        for (auto && fam : info.families) {
            if (level > keys[fam.family].level) {
                keys[fam.family] = { fam.factories.front().factory, level };
            }
        }
    }

    bool known = true;
    *fact = nullptr;

    for (auto i = 0; (size_t) i < s_matchers.size(); ++i) {
        auto fam = (AppSocket::Family) i;
        if (!keys[fam].fact)
            continue;
        if (auto matcher = s_matchers[fam].notify) {
            auto match = matcher->onMatch(fam, data);
            switch (match) {
            case AppSocket::kUnknown:
                known = false;
                [[fallthrough]];
            case AppSocket::kUnsupported:
                continue;
            case AppSocket::kSupported:
            case AppSocket::kPreferred:
                break;
            }

            // it's at least kSupported, possibly kPreferred
            *family = fam;
            *fact = keys[fam].fact;
            if (match == AppSocket::kPreferred) {
                // if it's a preferred match stop looking for better
                // matches and just use it.
                return true;
            }
        }
    }

    return known;
}

//===========================================================================
bool IAppSocket::notifyRead(AppSocketData & data) {
    if (m_notify)
        return m_notify->onSocketRead(data);

    // already queued for disconnect? ignore any incoming data
    if (!m_pos)
        return true;

    string_view view(data.data, data.bytes);
    if (!m_pos->socketData.empty())
        view = m_pos->socketData.append(view);

    IFactory<IAppSocketNotify> * fact;
    if (!findFactory(&m_accept.fam, &fact, m_accept.localAddr, view)) {
        if (m_pos->socketData.empty())
            m_pos->socketData.assign(data.data, data.bytes);
        return true;
    }

    // no longer unmatched - one way or another
    delete m_pos;
    m_pos = {};

    if (!fact) {
        logMsgDebug() << "AppSocket: unknown protocol";
        logHexDebug(view.substr(0, 128));
        disconnect(AppSocket::Disconnect::kUnknownProtocol);
        return true;
    }

    setNotify(fact->onFactoryCreate().release());

    // set notifier from registered factory
    if (!notifyAccept(m_accept))
        return true;

    // replay data received so far
    AppSocketData tmp;
    tmp.data = const_cast<char *>(view.data());
    tmp.bytes = (int) view.size();
    return m_notify->onSocketRead(tmp);
}

//===========================================================================
void IAppSocket::notifyBufferChanged(const AppSocketBufferInfo & info) {
    if (m_notify)
        m_notify->onSocketBufferChanged(info);
}


/****************************************************************************
*
*   RawSocket
*
***/

//===========================================================================
RawSocket::RawSocket(IAppSocketNotify * notify)
    : IAppSocket(notify)
{}

//===========================================================================
SocketInfo RawSocket::getInfo() const {
    return socketGetInfo(this);
}

//===========================================================================
void RawSocket::disconnect(AppSocket::Disconnect why) {
    if (m_bufferUsed) {
        socketWrite(this, move(m_buffer), m_bufferUsed);
        m_bufferUsed = 0;
    }
    switch (why) {
    case AppSocket::Disconnect::kAppRequest: s_perfRequest += 1; break;
    case AppSocket::Disconnect::kNoData: s_perfNoData += 1; break;
    case AppSocket::Disconnect::kUnknownProtocol: s_perfUnknown += 1; break;
    case AppSocket::Disconnect::kCryptError: s_perfCrypt += 1; break;
    case AppSocket::Disconnect::kInactivity: s_perfInactive += 1; break;
    }
    socketDisconnect(this);
}

//===========================================================================
void RawSocket::write(string_view data) {
    bool hadData = m_bufferUsed;
    while (!data.empty()) {
        if (!m_buffer)
            m_buffer = socketGetBuffer();
        size_t bytes = min(m_buffer->capacity - m_bufferUsed, data.size());
        memcpy(m_buffer->data + m_bufferUsed, data.data(), bytes);
        data.remove_prefix(bytes);
        m_bufferUsed += bytes;
        if (m_bufferUsed == (size_t) m_buffer->capacity) {
            socketWrite(this, move(m_buffer), m_bufferUsed);
            m_bufferUsed = 0;
        }
    }
    if (!hadData && m_bufferUsed)
        timerUpdate(this, 1ms, true);
}

//===========================================================================
void RawSocket::write(unique_ptr<SocketBuffer> buffer, size_t bytes) {
    bool hadData = m_bufferUsed;
    if (hadData)
        socketWrite(this, move(m_buffer), m_bufferUsed);
    if (bytes == (size_t) buffer->capacity) {
        socketWrite(this, move(buffer), bytes);
        m_bufferUsed = 0;
    } else {
        m_buffer = move(buffer);
        m_bufferUsed = bytes;
        if (!hadData)
            timerUpdate(this, 1ms, true);
    }
}

//===========================================================================
void RawSocket::read() {
    socketRead(this);
}

//===========================================================================
void RawSocket::onSocketConnect(const SocketConnectInfo & info) {
    AppSocketConnectInfo tmp = {
        .localAddr = info.localAddr,
        .remoteAddr = info.remoteAddr,
    };
    return notifyConnect(tmp);
}

//===========================================================================
void RawSocket::onSocketConnectFailed() {
    notifyConnectFailed();
}

//===========================================================================
bool RawSocket::onSocketAccept(const SocketConnectInfo & info) {
    AppSocketConnectInfo tmp = {
        .localAddr = info.localAddr,
        .remoteAddr = info.remoteAddr,
    };
    return notifyAccept(tmp);
}

//===========================================================================
void RawSocket::onSocketDisconnect() {
    notifyDisconnect();
}

//===========================================================================
void RawSocket::onSocketDestroy() {
    notifyDestroy();
}

//===========================================================================
bool RawSocket::onSocketRead(SocketData & data) {
    AppSocketData ad = { data.data, data.bytes };
    return notifyRead(ad);
}

//===========================================================================
void RawSocket::onSocketBufferChanged(const SocketBufferInfo & info) {
    AppSocketBufferInfo ai;
    ai.incomplete = info.incomplete;
    ai.waiting = info.waiting;
    ai.total = info.total;
    notifyBufferChanged(ai);
}

//===========================================================================
Duration RawSocket::onTimer(TimePoint now) {
    if (m_bufferUsed) {
        socketWrite(this, move(m_buffer), m_bufferUsed);
        m_bufferUsed = 0;
    }
    return kTimerInfinite;
}


/****************************************************************************
*
*   RawMatch
*
***/

namespace {
class RawMatch : public IAppSocketMatchNotify {
    AppSocket::MatchType onMatch(
        AppSocket::Family fam,
        string_view view) override;
};
} // namespace
static RawMatch s_rawMatch;

//===========================================================================
AppSocket::MatchType RawMatch::onMatch(
    AppSocket::Family fam,
    string_view view
) {
    assert(fam == AppSocket::kRaw);
    return AppSocket::kSupported;
}


/****************************************************************************
*
*   Configuration monitor
*
***/

namespace {
class AppXmlNotify : public IConfigNotify {
    void onConfigChange(const XDocument & doc) override;
};
} // namespace
static AppXmlNotify s_appXml;

//===========================================================================
void AppXmlNotify::onConfigChange(const XDocument & doc) {
    s_disableNoDataTimeout = configNumber(doc, "DisableNoDataTimeout");
    timerUpdate(
        &s_unmatchedTimer,
        s_disableNoDataTimeout ? kTimerInfinite : 0ms
    );
}


/****************************************************************************
*
*   Shutdown monitor
*
***/

namespace {
class ShutdownNotify : public IShutdownNotify {
    void onShutdownClient(bool firstTry) override;
    void onShutdownConsole(bool firstTry) override;
};
} // namespace
static ShutdownNotify s_cleanup;

//===========================================================================
void ShutdownNotify::onShutdownClient(bool firstTry) {
    for ([[maybe_unused]] auto && info : s_sockAddrs)
        assert(info.listeners == info.consoles);
}

//===========================================================================
void ShutdownNotify::onShutdownConsole(bool firstTry) {
    for ([[maybe_unused]] auto && info : s_sockAddrs)
        assert(!info.listeners && !info.consoles);

    if (firstTry) {
        for (auto && info : s_unmatched)
            info.notify->disconnect(AppSocket::Disconnect::kAppRequest);
    }
    if (!s_unmatched.empty())
        shutdownIncomplete();
}


/****************************************************************************
*
*   Internal API
*
***/

//===========================================================================
void Dim::iAppSocketInitialize() {
    shutdownMonitor(&s_cleanup);
    configMonitor("app.xml", &s_appXml);
    socketAddFamily(AppSocket::kRaw, &s_rawMatch);
}


/****************************************************************************
*
*   Public API
*
***/

//===========================================================================
SocketInfo Dim::socketGetInfo(const IAppSocketNotify * notify) {
    return IAppSocket::getInfo(notify);
}

//===========================================================================
void Dim::socketDisconnect(IAppSocketNotify * notify) {
    IAppSocket::disconnect(notify, AppSocket::Disconnect::kAppRequest);
}

//===========================================================================
void Dim::socketDisconnect(
    IAppSocketNotify * notify,
    AppSocket::Disconnect why
) {
    IAppSocket::disconnect(notify, why);
}

//===========================================================================
void Dim::socketWrite(IAppSocketNotify * notify, string_view data) {
    IAppSocket::write(notify, data);
}

//===========================================================================
void Dim::socketWrite(IAppSocketNotify * notify, const CharBuf & data) {
    IAppSocket::write(notify, data);
}

//===========================================================================
void Dim::socketWrite(
    IAppSocketNotify * notify,
    unique_ptr<SocketBuffer> buffer,
    size_t bytes
) {
    IAppSocket::write(notify, move(buffer), bytes);
}

//===========================================================================
void Dim::socketRead(IAppSocketNotify * notify) {
    IAppSocket::read(notify);
}

//===========================================================================
void Dim::socketConnect(
    IAppSocketNotify * notify,
    const SockAddr & remote,
    const SockAddr & local,
    string_view data,
    Duration wait
) {
    auto sock = make_unique<RawSocket>(notify);
    socketConnect(sock.release(), remote, local, data, wait);
}

//===========================================================================
void Dim::socketAddFamily(
    AppSocket::Family fam,
    IAppSocketMatchNotify * notify
) {
    if ((size_t) fam >= s_matchers.size())
        s_matchers.resize(fam + 1);
    s_matchers[fam].notify = notify;
}

//===========================================================================
static SockAddrInfo * findInfo_LK(const SockAddr & addr, bool findAlways) {
    for (auto && ep : s_sockAddrs) {
        if (ep.saddr == addr)
            return &ep;
    }
    if (findAlways) {
        SockAddrInfo info;
        info.saddr = addr;
        s_sockAddrs.push_back(info);
        return &s_sockAddrs.back();
    }
    return nullptr;
}

//===========================================================================
static bool addFactory(
    IFactory<IAppSocketNotify> * factory,
    EnumFlags<FactoryFlags> flags,
    const SockAddr & end,
    AppSocket::Family fam
) {
    auto info = findInfo_LK(end, true);
    bool addNew = flags.any(fListener) && ++info->listeners == 1;
    if (flags.any(fConsole))
        info->consoles += 1;
    auto & fams = info->families;
    auto afi = find(fams.begin(), fams.end(), fam);
    if (afi == fams.end())
        afi = fams.emplace(afi, FamilyInfo{fam});
    afi->factories.push_back(FactoryInfo{factory, flags});
    return addNew;
}

//===========================================================================
void Dim::socketListen(
    IFactory<IAppSocketNotify> * factory,
    const SockAddr & end,
    AppSocket::Family fam,
    bool console
) {
    EnumFlags flags = fListener;
    if (console)
        flags |= fConsole;
    if (addFactory(factory, flags, end, fam))
        socketListen<RawSocket>(end);
}

//===========================================================================
void Dim::socketCloseWait(
    IFactory<IAppSocketNotify> * factory,
    const SockAddr & end,
    AppSocket::Family fam
) {
    for (;;) {
        auto info = findInfo_LK(end, false);
        if (!info)
            break;
        auto & fams = info->families;
        if (fams.empty())
            break;
        auto afi = find(fams.begin(), fams.end(), fam);
        if (afi == fams.end())
            break;

        auto & facts = afi->factories;
        auto i = find_if(
            facts.begin(),
            facts.end(),
            [=](auto && val) {
                return val.factory == factory && val.flags.any(fListener);
            }
        );
        if (i == facts.end())
            break;

        bool noListeners = --info->listeners == 0;
        if (i->flags.any(fConsole))
            info->consoles -= 1;
        facts.erase(i);
        if (facts.empty()) {
            fams.erase(afi);
            if (fams.empty()) {
                s_sockAddrs.erase(
                    s_sockAddrs.begin() + (info - s_sockAddrs.data())
                );
            }
        }
        if (noListeners)
            socketCloseWait<RawSocket>(end);
        return;
    }

    logMsgError() << "Remove unknown listener: " << end << ", " << fam;
}

//===========================================================================
void Dim::socketAddFilter(
    IFactory<IAppSocketNotify> * factory,
    const SockAddr & end,
    AppSocket::Family fam
) {
    addFactory(factory, {}, end, fam);
}
