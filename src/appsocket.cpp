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
    AppSocket::Family family;
    IAppSocketMatchNotify * notify;
};

struct FamilyInfo {
    unordered_multimap<string, IAppSocketNotifyFactory *> factories;
};

class EndpointInfo : public ISocketListenNotify {
public:
    void onListenStop(const Endpoint & local) override;
    std::unique_ptr<ISocketNotify> onListenCreateSocket(
        const Endpoint & local
    ) override;

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
static vector<MatchKey> s_matchers;
static unordered_map<Endpoint, EndpointInfo *> s_endpoints;
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
    auto i = s_endpoints.find(m_accept.localEnd);
    auto info = i->second;
    bool unknown = false;
    IAppSocketNotifyFactory * fact = nullptr;
    for (auto && fam : info->families) {
        for (auto && key : s_matchers) {
            if (fam.first != key.family) 
                continue;
            auto match = key.notify->OnMatch(key.family, view);
            switch (match) {
            case AppSocket::kUnknown:
                unknown = true;
                [[fallthrough]];
            case AppSocket::kUnsupported:
                continue;
            }

            // it's at least kSupported, possibly kPreferred
            auto i = fam.second.factories.lower_bound("");
            assert(i->first.empty());
            fact = i->second;
            if (match == AppSocket::kPreferred) {
                // if it's a preferred match stop looking for better
                // matches and just use it.
                goto FOUND;
            }
        }
    }

    if (unknown) {
        if (m_socketData.empty())
            m_socketData.assign(data.data, data.bytes);
        return;
    }

    if (!fact) {
        socketDisconnect(this);
        m_unmatched = true;
        return;
    }

FOUND:
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
class ShutdownMonitor : public IAppShutdownNotify {
    void onAppStartConsoleCleanup() override;
    bool onAppQueryConsoleDestroy() override;
};
static ShutdownMonitor s_cleanup;
} // namespace

//===========================================================================
void ShutdownMonitor::onAppStartConsoleCleanup() {
    lock_guard<mutex> lk{s_sockMut};
    for (auto && ptr : s_unmatched)
        socketDisconnect(ptr);
}

//===========================================================================
bool ShutdownMonitor::onAppQueryConsoleDestroy() {
    {
        lock_guard<mutex> lk{s_sockMut};
        if (!s_unmatched.empty())
            return appQueryDestroyFailed();
    }

    shared_lock<shared_mutex> lk{s_listenMut};
    assert(s_endpoints.empty());
    if (!s_stopping.empty())
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
    static ByteMatch s_match;
    appSocketAddMatch(&s_match, AppSocket::kByte);
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
    for (auto && key : s_matchers) {
        if (key.family == fam) {
            key.notify = notify;
            return;
        }
    }
    MatchKey key{fam, notify};
    s_matchers.push_back(key);
}

//===========================================================================
void Dim::appSocketAddListener(
    IAppSocketNotifyFactory * factory,
    AppSocket::Family fam,
    const std::string & type,
    Endpoint end) {
    bool addNew = false;
    EndpointInfo * info = nullptr;
    {
        unique_lock<shared_mutex> lk{s_listenMut};
        auto i = s_endpoints.find(end);
        if (i != s_endpoints.end()) {
            info = i->second;
        } else {
            info = new EndpointInfo;
            s_endpoints.insert(i, make_pair(end, info));
        }
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
    const std::string & type,
    Endpoint end) {
    EndpointInfo * info = nullptr;
    unique_lock<shared_mutex> lk{s_listenMut};
    info = s_endpoints[end];
    if (info && !info->families.empty()) {
        auto fi = info->families.find(fam);
        if (fi != info->families.end()) {
            auto ii = fi->second.factories.equal_range(type);
            for (; ii.first != ii.second; ++ii.first) {
                if (ii.first->second == factory) {
                    fi->second.factories.erase(ii.first);
                    if (!fi->second.factories.empty())
                        return;
                    info->families.erase(fi);
                    if (!info->families.empty())
                        return;
                    socketStop(info, end);
                    s_endpoints.erase(end);
                    s_stopping.push_back(info);
                    return;
                }
            }
        }
    }

    if (!info)
        s_endpoints.erase(end);
    logMsgError() << "Remove unknown listener, " << end << ", " << fam << ", "
                  << type;
}
