// appsocket.cpp - dim services
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

class AppSocketNotify : public ISocketNotify {
public:
    ~AppSocketNotify();

    Duration checkTimeout_LK(TimePoint now);

    void onSocketAccept(const SocketAcceptInfo & info) override;
    void onSocketRead(const SocketData & data) override;
    void onSocketDisconnect() override;

private:
    bool m_accepting{false};
    SocketAcceptInfo m_accept;
    TimePoint m_expiration;
    list<AppSocketNotify*>::iterator m_pos;
    string m_socketData;
    bool m_unmatched{false};
};

class NoDataTimer : public ITimerNotify {
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

static mutex s_sockMut;
static list<AppSocketNotify*> s_unmatched;
static NoDataTimer s_noDataTimer;


/****************************************************************************
*
*   EndpointInfo
*
***/

//===========================================================================
void EndpointInfo::onListenStop(const Endpoint & local) {
    unique_lock<shared_mutex> lk{s_listenMut};
    for (auto && ptr : s_stopping) {
        if (ptr == this) {
            auto it = s_stopping.begin() + (&ptr - s_stopping.data());
            s_stopping.erase(it);
            delete this;
            return;
        }
    }

    logMsgError() << "Stopped unknown listener, " << local;
}

//===========================================================================
std::unique_ptr<ISocketNotify> 
EndpointInfo::onListenCreateSocket(const Endpoint & local) {
    return make_unique<AppSocketNotify>();
}


/****************************************************************************
*
*   NoDataTimer
*
***/

//===========================================================================
Duration NoDataTimer::onTimer(TimePoint now) {
    lock_guard<mutex> lk{s_sockMut};
    while (!s_unmatched.empty()) {
        auto ptr = s_unmatched.front();
        auto wait = ptr->checkTimeout_LK(now);
        if (wait > 0s)
            return max(wait, kUnmatchedMinWait);
    }

    return kTimerInfinite;
}


/****************************************************************************
*
*   AppSocketNotify
*
***/

//===========================================================================
AppSocketNotify::~AppSocketNotify() {
    lock_guard<mutex> lk{s_sockMut};
    if (m_accepting) 
        s_unmatched.erase(m_pos);
}

//===========================================================================
Duration AppSocketNotify::checkTimeout_LK(TimePoint now) {
    assert(m_accepting);
    auto wait = m_expiration - now;
    if (wait > 0s)
        return wait;
    m_accepting = false;
    s_unmatched.erase(m_pos);
    socketDisconnect(this);
    return 0s;
}

//===========================================================================
void AppSocketNotify::onSocketAccept(const SocketAcceptInfo & info) {
    m_accept = info;
    m_accepting = true;
    m_expiration = Clock::now() + kUnmatchedTimeout;

    {
        lock_guard<mutex> lk{s_sockMut};
        s_unmatched.push_back(this);
        m_pos = --s_unmatched.end();
    }

    timerUpdate(&s_noDataTimer, kUnmatchedTimeout, true);
}

//===========================================================================
void AppSocketNotify::onSocketDisconnect() {
}

//===========================================================================
void AppSocketNotify::onSocketRead(const SocketData & data) {
    if (m_unmatched)
        return;

    string_view view(data.data, data.bytes);
    if (!m_socketData.empty()) {
        view = m_socketData.append(view);
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
            if (match == AppSocket::kPreferred) {
                // if it's a preferred match stop looking for better
                // matches and just use it.
                goto FINISH;
            }
        }
    }

    if (unknown) {
        if (m_socketData.empty())
            m_socketData.assign(data.data, data.bytes);
        return;
    }

FINISH:
    if (!fact) {
        socketDisconnect(this);
        m_unmatched = true;
        return;
    }

    // replace this sockets notifier with one from registered factory
    auto sock = fact->create().release();
    socketSetNotify(this, sock);
    // replay callbacks received so far
    sock->onSocketAccept(m_accept);
    SocketData tmp;
    tmp.data = const_cast<char*>(view.data());
    tmp.bytes = (int) view.size();
    sock->onSocketRead(tmp);
    // delete this now orphaned AppSocketNotify
    delete this;
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
*   Http2Match
*
***/

namespace {
    class Http2Match : public IAppSocketMatchNotify {
        AppSocket::MatchType OnMatch(
            AppSocket::Family fam, 
            string_view view) override;
    };
} // namespace
static Http2Match s_http2Match;

//===========================================================================
AppSocket::MatchType Http2Match::OnMatch(
    AppSocket::Family fam,
    string_view view
) {
    assert(fam == AppSocket::kHttp2);
    const char kPrefaceData[] = "PRI * HTTP/2.0\r\n\r\nSM\r\n\r\n";
    const size_t kPrefaceDataLen = size(kPrefaceData);
    size_t num = min(kPrefaceDataLen, view.size());
    if (view.compare(0, num, kPrefaceData, num) != 0)
        return AppSocket::kUnsupported;
    if (num == kPrefaceDataLen)
        return AppSocket::kPreferred;

    return AppSocket::kUnknown;
}


/****************************************************************************
*
*   Shutdown monitor
*
***/

namespace {
class ShutdownMonitor : public IAppShutdownNotify {
    bool onAppQueryClientDestroy() override;
    void onAppStartConsoleCleanup() override;
    bool onAppQueryConsoleDestroy() override;
};
static ShutdownMonitor s_cleanup;
} // namespace

  //===========================================================================
bool ShutdownMonitor::onAppQueryClientDestroy() {
    shared_lock<shared_mutex> lk{s_listenMut};
    assert(s_endpoints.empty());
    if (!s_stopping.empty())
        return appQueryDestroyFailed();

    return true;
}

//===========================================================================
void ShutdownMonitor::onAppStartConsoleCleanup() {
    lock_guard<mutex> lk{s_sockMut};
    for (auto && ptr : s_unmatched)
        socketDisconnect(ptr);
}

//===========================================================================
bool ShutdownMonitor::onAppQueryConsoleDestroy() {
    lock_guard<mutex> lk{s_sockMut};
    if (!s_unmatched.empty())
        return appQueryDestroyFailed();

    return true;
}


/****************************************************************************
*
*   Internal API
*
***/

//===========================================================================
void Dim::iAppSocketInitialize() {
    appMonitorShutdown(&s_cleanup);
    appSocketAddMatch(&s_byteMatch, AppSocket::kByte);
    appSocketAddMatch(&s_http2Match, AppSocket::kHttp2);
}


/****************************************************************************
*
*   Public API
*
***/

//===========================================================================
void Dim::appSocketAddMatch(
    IAppSocketMatchNotify * notify,
    AppSocket::Family fam) {
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
void Dim::appSocketAddListener(
    IAppSocketNotifyFactory * factory,
    AppSocket::Family fam,
    string_view type,
    const Endpoint & end) {
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
void Dim::appSocketRemoveListener(
    IAppSocketNotifyFactory * factory,
    AppSocket::Family fam,
    std::string_view type,
    const Endpoint & end) {
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
