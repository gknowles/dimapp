// Copyright Glen Knowles 2017.
// Distributed under the Boost Software License, Version 1.0.
//
// appsocket.cpp - dim net
#include "pch.h"
#pragma hdrstop

using namespace std;
using namespace Dim;


/****************************************************************************
*
*   Tuning parameters
*
***/

const Duration kUnmatchedTimeout = 10s;
const Duration kUnmatchedMinWait = 2s;


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
    IAppSocketMatchNotify * notify{nullptr};
};

struct FactoryInfo {
    IFactory<IAppSocketNotify> * factory;
    FactoryFlags flags{};
};

struct FamilyInfo {
    AppSocket::Family family;
    vector<FactoryInfo> factories;

    bool operator==(AppSocket::Family fam) const { return fam == family; }
};

struct EndpointInfo {
    Endpoint endpoint;
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
    void disconnect() override;
    void write(string_view data) override;
    void write(unique_ptr<SocketBuffer> buffer, size_t bytes) override;

    // ISocketNotify
    bool onSocketAccept(const SocketInfo & info) override;
    void onSocketDisconnect() override;
    void onSocketDestroy() override;
    void onSocketRead(SocketData & data) override;

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

static shared_mutex s_listenMut;
static vector<MatchKey> s_matchers;
static vector<EndpointInfo> s_endpoints;

static mutex s_unmatchedMut;
static list<IAppSocket::UnmatchedInfo> s_unmatched;
static IAppSocket::UnmatchedTimer s_unmatchedTimer;
static bool s_disableNoDataTimeout;

// time expired before enough data was received to determine the protocol
static auto & s_perfNoData = uperf("sock disconnect no data");
// incoming data didn't match any registered protocol
static auto & s_perfUnknown = uperf(
    "sock disconnect unknown protocol");
// local application called disconnect
static auto & s_perfExplicit = uperf("sock disconnect app explicit");
// local application rejected the accept
static auto & s_perfNotAccepted = uperf("sock disconnect not accepted");


/****************************************************************************
*
*   IAppSocket::UnmatchedTimer
*
***/

//===========================================================================
Duration IAppSocket::UnmatchedTimer::onTimer(TimePoint now) {
    lock_guard<mutex> lk{s_unmatchedMut};
    while (!s_unmatched.empty()) {
        auto & info = s_unmatched.front();
        auto wait = info.notify->checkTimeout_LK(now);
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
void IAppSocket::disconnect(IAppSocketNotify * notify) {
    s_perfExplicit += 1;
    notify->m_socket->disconnect();
}

//===========================================================================
// static
void IAppSocket::write(IAppSocketNotify * notify, string_view data) {
    notify->m_socket->write(data);
}

//===========================================================================
// static
void IAppSocket::write(IAppSocketNotify * notify, const CharBuf & data) {
    for (auto && v : data.views()) {
        notify->m_socket->write(v);
    }
}

//===========================================================================
// static 
void IAppSocket::write(
    IAppSocketNotify * notify, 
    unique_ptr<SocketBuffer> buffer, 
    size_t bytes
) {
    notify->m_socket->write(move(buffer), bytes);
}

//===========================================================================
IAppSocket::IAppSocket(unique_ptr<IAppSocketNotify> notify) {
    setNotify(move(notify));
}

//===========================================================================
IAppSocket::~IAppSocket() {
    assert(m_pos == list<UnmatchedInfo>::iterator{});
    assert(!m_notify);
}

//===========================================================================
Duration IAppSocket::checkTimeout_LK(TimePoint now) {
    assert(!m_notify);
    auto wait = m_pos->expiration - now;
    if (wait <= 0s) {
        s_perfNoData += 1;
        disconnect();
        s_unmatched.erase(m_pos);
        m_pos = {};
    }
    return wait;
}

//===========================================================================
void IAppSocket::setNotify(unique_ptr<IAppSocketNotify> notify) {
    assert(!m_notify);
    m_notify = notify.release();
    m_notify->m_socket = this;
}

//===========================================================================
void IAppSocket::notifyConnect(const AppSocketInfo & info) {
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
bool IAppSocket::notifyAccept(const AppSocketInfo & info) {
    m_accept = info;
    if (m_notify) {
        if (m_notify->onSocketAccept(m_accept)) 
            return true;

        s_perfNotAccepted += 1;
        disconnect();
        return false;
    }

    auto expiration = Clock::now() + kUnmatchedTimeout;

    {
        lock_guard<mutex> lk{s_unmatchedMut};
        UnmatchedInfo ui;
        ui.notify = this;
        ui.expiration = expiration;
        s_unmatched.push_back(ui);
        m_pos = --s_unmatched.end();
    }

    if (!s_disableNoDataTimeout)
        timerUpdate(&s_unmatchedTimer, kUnmatchedTimeout, true);
    return true;
}

//===========================================================================
void IAppSocket::notifyDisconnect() {
    if (m_notify)
        m_notify->onSocketDisconnect();
    if (m_pos != list<UnmatchedInfo>::iterator{}) {
        s_unmatched.erase(m_pos);
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
    const Endpoint & localEnd,
    string_view data
) {
    shared_lock<shared_mutex> lk{s_listenMut};
    
    // find best matching factory endpoint for each family
    enum {
        // match types, in reverse priority order
        kUnknown,   // no match
        kWild,      // wildcard factory (0.0.0.0:0)
        kPort,      // matching port with wildcard addr
        kAddr,      // matching addr with wildcard port
        kExact,     // both addr and port explicitly match
    };
    struct EndpointKey {
        IFactory<IAppSocketNotify> * fact;
        int level;
    };
    vector<EndpointKey> keys(s_matchers.size(), {nullptr, kUnknown});
    for (auto && info : s_endpoints) {
        int level{kUnknown};
        if (info.endpoint == localEnd) {
            level = kExact;
        } else if (!info.endpoint.addr) {
            if (info.endpoint.port == localEnd.port) {
                level = kPort;
            } else if (!info.endpoint.port) {
                level = kWild;
            }
        } else if (!info.endpoint.port 
            && info.endpoint.addr == localEnd.addr
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
    for (int i = 0; i < s_matchers.size(); ++i) {
        auto fam = (AppSocket::Family) i;
        if (auto matcher = s_matchers[fam].notify) {
            auto match = matcher->OnMatch(fam, data);
            switch (match) {
            case AppSocket::kUnknown:
                known = false;
                [[fallthrough]];
            case AppSocket::kUnsupported:
                continue;
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
void IAppSocket::notifyRead(AppSocketData & data) {
    if (m_notify)
        return m_notify->onSocketRead(data);

    // already queued for disconnect? ignore any incoming data
    if (m_pos == list<UnmatchedInfo>::iterator{})
        return;

    string_view view(data.data, data.bytes);
    if (!m_pos->socketData.empty())
        view = m_pos->socketData.append(view);

    IFactory<IAppSocketNotify> * fact;
    if (!findFactory(&m_accept.fam, &fact, m_accept.local, view)) {
        if (m_pos->socketData.empty())
            m_pos->socketData.assign(data.data, data.bytes);
        return;
    }

    {
        // no longer unmatched - one way or another
        lock_guard<mutex> lk{s_unmatchedMut};
        s_unmatched.erase(m_pos);
        m_pos = {};
    }

    if (!fact) {
        s_perfUnknown += 1;
        return disconnect();
    }

    setNotify(fact->onFactoryCreate());

    // set notifier from registered factory
    if (!notifyAccept(m_accept))
        return;

    // replay data received so far
    AppSocketData tmp;
    tmp.data = const_cast<char*>(view.data());
    tmp.bytes = (int) view.size();
    m_notify->onSocketRead(tmp);
}


/****************************************************************************
*
*   RawSocket
*
***/

//===========================================================================
void RawSocket::disconnect() {
    if (m_bufferUsed) {
        socketWrite(this, move(m_buffer), m_bufferUsed);
        m_bufferUsed = 0;
    }
    socketDisconnect(this);
}

//===========================================================================
void RawSocket::write(string_view data) {
    bool hadData = m_bufferUsed;
    while (!data.empty()) {
        if (!m_buffer) 
            m_buffer = socketGetBuffer();
        size_t bytes = min(m_buffer->len - m_bufferUsed, data.size());
        memcpy(m_buffer->data + m_bufferUsed, data.data(), bytes);
        data.remove_prefix(bytes);
        m_bufferUsed += bytes;
        if (m_bufferUsed == m_buffer->len) {
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
    if (bytes == buffer->len) {
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
bool RawSocket::onSocketAccept(const SocketInfo & info) {
    AppSocketInfo ai = {};
    ai.local = info.local;
    ai.remote = info.remote;
    return notifyAccept(ai);
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
void RawSocket::onSocketRead(SocketData & data) {
    AppSocketData ad = { data.data, data.bytes };
    notifyRead(ad);
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
    AppSocket::MatchType OnMatch(
        AppSocket::Family fam, 
        string_view view) override;
};
} // namespace
static RawMatch s_rawMatch;

//===========================================================================
AppSocket::MatchType RawMatch::OnMatch(
    AppSocket::Family fam,
    string_view view
) {
    assert(fam == AppSocket::kRaw);
    return AppSocket::kSupported;
}


/****************************************************************************
*
*   Config monitor 
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
    s_disableNoDataTimeout = configUnsigned(doc, "DisableNoDataTimeout");
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
    shared_lock<shared_mutex> lk{s_listenMut};
    for (auto && info [[maybe_unused]] : s_endpoints) {
        assert(info.listeners == info.consoles);
    }
}

//===========================================================================
void ShutdownNotify::onShutdownConsole(bool firstTry) {
    {
        shared_lock<shared_mutex> lk{s_listenMut};
        for (auto && info [[maybe_unused]] : s_endpoints) {
            assert(!info.listeners && !info.consoles);
        }
    }

    lock_guard<mutex> lk{s_unmatchedMut};
    if (firstTry) {
        for (auto && info : s_unmatched)
            info.notify->disconnect();
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
void Dim::socketDisconnect(IAppSocketNotify * notify) {
    IAppSocket::disconnect(notify);
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
void Dim::socketAddFamily(
    AppSocket::Family fam, 
    IAppSocketMatchNotify * notify
) {
    unique_lock<shared_mutex> lk{s_listenMut};
    if (fam >= s_matchers.size())
        s_matchers.resize(fam + 1);
    s_matchers[fam].notify = notify;
}

//===========================================================================
static EndpointInfo * findInfo_LK(const Endpoint & end, bool findAlways) {
    for (auto && ep : s_endpoints) {
        if (ep.endpoint == end) 
            return &ep;
    }
    if (findAlways) {
        EndpointInfo info;
        info.endpoint = end;
        s_endpoints.push_back(info);
        return &s_endpoints.back();
    }
    return nullptr;
}

//===========================================================================
static bool addFactory(
    IFactory<IAppSocketNotify> * factory,
    FactoryFlags flags,
    const Endpoint & end,
    AppSocket::Family fam
) {
    unique_lock<shared_mutex> lk{s_listenMut};
    auto info = findInfo_LK(end, true);
    bool addNew = (flags & fListener) && ++info->listeners == 1;
    if (flags & fConsole)
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
    const Endpoint & end,
    AppSocket::Family fam,
    bool console
) {
    FactoryFlags flags = fListener;
    if (console)
        flags |= fConsole;
    if (addFactory(factory, flags, end, fam))
        socketListen<RawSocket>(end);
}

//===========================================================================
void Dim::socketCloseWait(
    IFactory<IAppSocketNotify> * factory,
    const Endpoint & end,
    AppSocket::Family fam
) {
    for (;;) {
        unique_lock<shared_mutex> lk{s_listenMut};
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
                return val.factory == factory && (val.flags & fListener); 
            }
        );
        if (i == facts.end())
            break;

        bool noListeners = --info->listeners == 0;
        if (i->flags & fConsole)
            info->consoles -= 1;
        facts.erase(i);
        if (facts.empty()) {
            fams.erase(afi);
            if (fams.empty()) {
                s_endpoints.erase(
                    s_endpoints.begin() + (info - s_endpoints.data())
                );
            }
        }
        lk.unlock();
        if (noListeners)
            socketCloseWait<RawSocket>(end);
        return;
    }

    logMsgError() << "Remove unknown listener: " << end << ", " << fam;
}

//===========================================================================
void Dim::socketAddFilter(
    IFactory<IAppSocketNotify> * factory,
    const Endpoint & end,
    AppSocket::Family fam
) {
    addFactory(factory, {}, end, fam);
}
