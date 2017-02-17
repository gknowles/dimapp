// appsocket.cpp - dim services
#include "pch.h"
#pragma hdrstop

using namespace std;
using namespace Dim;


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
    void onSocketAccept(const SocketAcceptInfo & info) override;
    void onSocketRead(const SocketData & data) override;
    void onSocketDisconnect() override;

private:
    SocketAcceptInfo m_accept;
};

} // namespace


/****************************************************************************
*
*   Variables
*
***/

static mutex s_mut;
static vector<MatchKey> s_matchers;
static unordered_map<Endpoint, EndpointInfo *> s_endpoints;
static vector<EndpointInfo *> s_stopping;


/****************************************************************************
*
*   EndpointInfo
*
***/

//===========================================================================
void EndpointInfo::onListenStop(const Endpoint & local) {
    lock_guard<mutex> lk{s_mut};
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
*   IAppSocketNotify
*
***/

//===========================================================================
IAppSocketNotify::IAppSocketNotify(AppSocket & sock)
    : m_socket{sock} {}


/****************************************************************************
*
*   AppSocketNotify
*
***/

//===========================================================================
void AppSocketNotify::onSocketAccept(const SocketAcceptInfo & info) {
    m_accept = info;
    // start 10 second timer
}

//===========================================================================
void AppSocketNotify::onSocketDisconnect() {
}

//===========================================================================
void AppSocketNotify::onSocketRead(const SocketData & data) {
}


/****************************************************************************
*
*   ByteMatch
*
***/

namespace {
class ByteMatch : public IAppSocketMatchNotify {
    bool OnMatch(
        AppSocket::Family fam, 
        const char ptr[], 
        size_t count, 
        bool excl) override;
};
} // namespace

//===========================================================================
bool ByteMatch::OnMatch(
    AppSocket::Family fam,
    const char ptr[],
    size_t count,
    bool excl) {
    assert(fam == AppSocket::kByte);
    return !excl;
}


/****************************************************************************
*
*   Shutdown monitor
*
***/

namespace {
class ShutdownMonitor : public IAppShutdownNotify {
    bool onAppQueryConsoleDestroy() override;
};
static ShutdownMonitor s_cleanup;
} // namespace

//===========================================================================
bool ShutdownMonitor::onAppQueryConsoleDestroy() {
    lock_guard<mutex> lk{s_mut};
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
    lock_guard<mutex> lk{s_mut};
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
    EndpointInfo * epi = nullptr;
    {
        lock_guard<mutex> lk{s_mut};
        auto i = s_endpoints.find(end);
        if (i != s_endpoints.end()) {
            epi = i->second;
        } else {
            epi = new EndpointInfo;
            s_endpoints.insert(i, make_pair(end, epi));
        }
        if (epi->families.empty())
            addNew = true;
        epi->families[fam].factories.insert(make_pair(type, factory));
    }
    if (addNew)
        socketListen(epi, end);
}

//===========================================================================
void Dim::appSocketRemoveListener(
    IAppSocketNotifyFactory * factory,
    AppSocket::Family fam,
    const std::string & type,
    Endpoint end) {
    EndpointInfo * epi = nullptr;
    lock_guard<mutex> lk{s_mut};
    epi = s_endpoints[end];
    if (epi && !epi->families.empty()) {
        auto fi = epi->families.find(fam);
        if (fi != epi->families.end()) {
            auto ii = fi->second.factories.equal_range(type);
            for (; ii.first != ii.second; ++ii.first) {
                if (ii.first->second == factory) {
                    fi->second.factories.erase(ii.first);
                    if (!fi->second.factories.empty())
                        return;
                    epi->families.erase(fi);
                    if (!epi->families.empty())
                        return;
                    socketStop(epi, end);
                    s_endpoints.erase(end);
                    s_stopping.push_back(epi);
                    return;
                }
            }
        }
    }

    if (!epi)
        s_endpoints.erase(end);
    logMsgError() << "Remove unknown listener, " << end << ", " << fam << ", "
                  << type;
}
