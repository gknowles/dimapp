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
struct MatchKey {
    IAppSocketMatchNotify * notify{nullptr};
};

struct FamilyInfo {
    multimap<string, IAppSocketNotifyFactory *> factories;
};

struct UnmatchedInfo {
    TimePoint expiration;
    string socketData;
    AppSocketBase * notify{nullptr};
};

class EndpointInfo : public ISocketListenNotify {
public:
    void onListenStop(const Endpoint & local) override;
    std::unique_ptr<ISocketNotify> onListenCreateSocket(
        const Endpoint & local
    ) override;

    Endpoint endpoint;
    unordered_map<AppSocket::Family, FamilyInfo> families;
    bool stopping{false};
};

} // namespace


namespace Dim {
    
class AppSocketBase : public ISocketNotify, ITimerNotify {
public:
    static void disconnect(IAppSocketNotify * notify);
    static void write(IAppSocketNotify * notify, string_view data);
    static void write(IAppSocketNotify * notify, const CharBuf & data);

public:
    ~AppSocketBase();

    Duration checkTimeout_LK(TimePoint now);
    void write(std::string_view data);

    bool onSocketAccept(const SocketInfo & info) override;
    void onSocketDisconnect() override;
    void onSocketDestroy() override;
    void onSocketRead(const SocketData & data) override;

private:
    Duration onTimer(TimePoint now) override;

    AppSocketInfo m_accept;

    // set to list.end() when matching is successfully or unsuccessfully
    // completed.
    list<UnmatchedInfo>::iterator m_pos;

    IAppSocketNotify * m_notify{nullptr};
    unique_ptr<SocketBuffer> m_buffer;
    size_t m_bufferUsed{0};
};

class UnmatchedTimer : public ITimerNotify {
    Duration onTimer(TimePoint now) override;
};

} // namespace


/****************************************************************************
*
*   Variables
*
***/

static shared_mutex s_listenMut;
static MatchKey s_matchers[AppSocket::kNumFamilies];
static vector<EndpointInfo *> s_endpoints;
static vector<EndpointInfo *> s_stopping;

static mutex s_unmatchedMut;
static list<UnmatchedInfo> s_unmatched;
static UnmatchedTimer s_unmatchedTimer;

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
*   EndpointInfo
*
***/

//===========================================================================
void EndpointInfo::onListenStop(const Endpoint & local) {
    unique_lock<shared_mutex> lk{s_listenMut};
    for (auto i = s_stopping.begin(), e = s_stopping.end(); i != e; ++i) {
        if (*i == this) {
            s_stopping.erase(i);
            delete this;
            return;
        }
    }

    logMsgError() << "Stopped unknown listener, " << local;
}

//===========================================================================
std::unique_ptr<ISocketNotify> 
EndpointInfo::onListenCreateSocket(const Endpoint & local) {
    return make_unique<AppSocketBase>();
}


/****************************************************************************
*
*   UnmatchedTimer
*
***/

//===========================================================================
Duration UnmatchedTimer::onTimer(TimePoint now) {
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
*   AppSocketBase
*
***/

//===========================================================================
// static
void AppSocketBase::disconnect(IAppSocketNotify * notify) {
    s_perfExplicit += 1;
    socketDisconnect(notify->m_socket);
}

//===========================================================================
// static
void AppSocketBase::write(IAppSocketNotify * notify, std::string_view data) {
    notify->m_socket->write(data);
}

//===========================================================================
// static
void AppSocketBase::write(IAppSocketNotify * notify, const CharBuf & data) {
    for (auto && v : data.views()) {
        notify->m_socket->write(v);
    }
}

//===========================================================================
AppSocketBase::~AppSocketBase() {
    assert(m_pos == list<UnmatchedInfo>::iterator{});
}

//===========================================================================
Duration AppSocketBase::checkTimeout_LK(TimePoint now) {
    assert(!m_notify);
    auto wait = m_pos->expiration - now;
    if (wait > 0s)
        return wait;
    s_unmatched.erase(m_pos);
    m_pos = {};
    s_perfNoData += 1;
    socketDisconnect(this);
    return 0s;
}

//===========================================================================
void AppSocketBase::write(std::string_view data) {
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
bool AppSocketBase::onSocketAccept(const SocketInfo & info) {
    m_accept.local = info.local;
    m_accept.remote = info.remote;
    auto expiration = Clock::now() + kUnmatchedTimeout;

    {
        lock_guard<mutex> lk{s_unmatchedMut};
        UnmatchedInfo ui;
        ui.notify = this;
        ui.expiration = expiration;
        s_unmatched.push_back(ui);
        m_pos = --s_unmatched.end();
    }

    timerUpdate(&s_unmatchedTimer, kUnmatchedTimeout, true);
    return true;
}

//===========================================================================
void AppSocketBase::onSocketDisconnect() {
    if (m_notify)
        m_notify->onSocketDisconnect();
}

//===========================================================================
void AppSocketBase::onSocketDestroy() {
    if (m_notify)
        m_notify->onSocketDestroy();
    delete this;
}

//===========================================================================
void AppSocketBase::onSocketRead(const SocketData & data) {
    if (m_notify) {
        AppSocketData ad = {};
        ad.data = data.data;
        ad.bytes = data.bytes;
        return m_notify->onSocketRead(ad);
    }

    // already queued for disconnect? ignore any incoming data
    if (m_pos == list<UnmatchedInfo>::iterator{})
        return;

    string_view view(data.data, data.bytes);
    if (!m_pos->socketData.empty()) {
        view = m_pos->socketData.append(view);
    }
    shared_lock<shared_mutex> lk{s_listenMut};
    
    // find best matching listener endpoint for each family
    enum {
        kUnknown,   // no match
        kWild,      // wildcard listener (0.0.0.0:0)
        kPort,      // matching port with wildcard addr
        kAddr,      // matching addr with wildcard port
        kExact,     // both addr and port explicitly match
    };
    struct EndpointKey {
        IAppSocketNotifyFactory * fact{nullptr};
        int level{kUnknown};
    } keys[AppSocket::kNumFamilies];
    for (auto && ptr : s_endpoints) {
        int level{kUnknown};
        if (ptr->endpoint == m_accept.local) {
            level = kExact;
        } else if (!ptr->endpoint.addr) {
            if (ptr->endpoint.port == m_accept.local.port) {
                level = kPort;
            } else if (!ptr->endpoint.port) {
                level = kWild;
            }
        } else if (!ptr->endpoint.port 
            && ptr->endpoint.addr == m_accept.local.addr
        ) {
            level = kAddr;
        }
        for (auto && fam : ptr->families) {
            if (level > keys[fam.first].level) {
                auto i = fam.second.factories.lower_bound("");
                if (i != fam.second.factories.end()) {
                    keys[fam.first] = { i->second, level };
                }
            }
        }
    }

    bool unknown = false;
    IAppSocketNotifyFactory * fact = nullptr;
    for (int i = 0; i < AppSocket::kNumFamilies; ++i) {
        auto fam = (AppSocket::Family) i;
        if (auto matcher = s_matchers[fam].notify) {
            auto match = matcher->OnMatch(fam, view);
            switch (match) {
            case AppSocket::kUnknown:
                unknown = true;
                [[fallthrough]];
            case AppSocket::kUnsupported:
                continue;
            }

            // it's at least kSupported, possibly kPreferred
            fact = keys[fam].fact;
            m_accept.fam = fam;
            if (match == AppSocket::kPreferred) {
                // if it's a preferred match stop looking for better
                // matches and just use it.
                goto FINISH;
            }
        }
    }

    if (unknown) {
        if (m_pos->socketData.empty())
            m_pos->socketData.assign(data.data, data.bytes);
        return;
    }

FINISH:
    {
        // no longer unmatched - one way or another
        lock_guard<mutex> lk{s_unmatchedMut};
        s_unmatched.erase(m_pos);
        m_pos = {};
    }

    if (!fact) {
        s_perfUnknown += 1;
        return socketDisconnect(this);
    }

    // set notifier from registered factory
    m_notify = fact->create().release();
    m_notify->m_socket = this;

    // replay callbacks received so far
    if (!m_notify->onSocketAccept(m_accept)) {
        s_perfNotAccepted += 1;
        return socketDisconnect(this);
    }
    AppSocketData tmp;
    tmp.data = const_cast<char*>(view.data());
    tmp.bytes = (int) view.size();
    m_notify->onSocketRead(tmp);
}

//===========================================================================
Duration AppSocketBase::onTimer(TimePoint now) {
    if (m_bufferUsed) {
        socketWrite(this, move(m_buffer), m_bufferUsed);
        m_bufferUsed = 0;
    }
    return kTimerInfinite;
}


/****************************************************************************
*
*   ByteMatch
*
***/

namespace {
class ByteMatch : public IAppSocketMatchNotify {
    AppSocket::MatchType OnMatch(
        AppSocket::Family fam, 
        string_view view) override;
};
} // namespace
static ByteMatch s_byteMatch;

//===========================================================================
AppSocket::MatchType ByteMatch::OnMatch(
    AppSocket::Family fam,
    string_view view
) {
    assert(fam == AppSocket::kByte);
    return AppSocket::kSupported;
}


/****************************************************************************
*
*   Shutdown monitor
*
***/

namespace {
class ShutdownNotify : public IShutdownNotify {
    void onShutdownClient(bool retry) override;
    void onShutdownConsole(bool retry) override;
};
} // namespace
static ShutdownNotify s_cleanup;

//===========================================================================
void ShutdownNotify::onShutdownClient(bool retry) {
    shared_lock<shared_mutex> lk{s_listenMut};
    assert(s_endpoints.empty());
    if (!s_stopping.empty())
        shutdownIncomplete();
}

//===========================================================================
void ShutdownNotify::onShutdownConsole(bool retry) {
    lock_guard<mutex> lk{s_unmatchedMut};
    if (!retry) {
        for (auto && info : s_unmatched)
            socketDisconnect(info.notify);
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
    socketAddFamily(AppSocket::kByte, &s_byteMatch);
}


/****************************************************************************
*
*   Public API
*
***/

//===========================================================================
void Dim::socketDisconnect(IAppSocketNotify * notify) {
    AppSocketBase::disconnect(notify);
}

//===========================================================================
void Dim::socketWrite(IAppSocketNotify * notify, std::string_view data) {
    AppSocketBase::write(notify, data);
}

//===========================================================================
void Dim::socketWrite(IAppSocketNotify * notify, const CharBuf & data) {
    AppSocketBase::write(notify, data);
}

//===========================================================================
void Dim::socketAddFamily(
    AppSocket::Family fam, 
    IAppSocketMatchNotify * notify
) {
    unique_lock<shared_mutex> lk{s_listenMut};
    s_matchers[fam].notify = notify;
}

//===========================================================================
static EndpointInfo * findInfo_LK(const Endpoint & end, bool findAlways) {
    for (auto && ep : s_endpoints) {
        if (ep->endpoint == end) 
            return ep;
    }
    if (findAlways) {
        auto info = new EndpointInfo;
        info->endpoint = end;
        s_endpoints.push_back(info);
        return info;
    }
    return nullptr;
}

//===========================================================================
static void eraseInfo_LK(const Endpoint & end) {
    auto i = s_endpoints.begin();
    auto e = s_endpoints.end();
    for (; i != e; ++i) {
        if ((*i)->endpoint == end) {
            s_endpoints.erase(i);
            return;
        }
    }
}

//===========================================================================
void Dim::socketListen(
    IAppSocketNotifyFactory * factory,
    AppSocket::Family fam,
    string_view type,
    const Endpoint & end
) {
    bool addNew = false;
    EndpointInfo * info = nullptr;
    {
        unique_lock<shared_mutex> lk{s_listenMut};
        info = findInfo_LK(end, true);
        if (info->families.empty())
            addNew = true;
        info->families[fam].factories.insert(make_pair(type, factory));
    }
    if (addNew)
        socketListen(info, end);
}

//===========================================================================
void Dim::socketStop(
    IAppSocketNotifyFactory * factory,
    AppSocket::Family fam,
    std::string_view type,
    const Endpoint & end
) {
    EndpointInfo * info = nullptr;
    unique_lock<shared_mutex> lk{s_listenMut};
    info = findInfo_LK(end, false);
    if (info && !info->families.empty()) {
        auto fi = info->families.find(fam);
        if (fi != info->families.end()) {
            auto ii = fi->second.factories.equal_range(string(type));
            for (; ii.first != ii.second; ++ii.first) {
                if (ii.first->second == factory) {
                    fi->second.factories.erase(ii.first);
                    if (!fi->second.factories.empty())
                        return;
                    info->families.erase(fi);
                    if (!info->families.empty())
                        return;
                    socketStop(info, end);
                    eraseInfo_LK(end);
                    s_stopping.push_back(info);
                    return;
                }
            }
        }
    }

    logMsgError() << "Remove unknown listener, " << end << ", " << fam << ", "
                  << type;
}
