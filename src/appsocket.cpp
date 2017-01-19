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
    void onListenStop() override;
    std::unique_ptr<ISocketNotify> onListenCreateSocket() override;

    unordered_map<AppSocket::Family, FamilyInfo> families;
};

class AppSocketNotify : public ISocketNotify {
public:
    void onSocketRead(const SocketData & data) override {}
};

} // namespace


/****************************************************************************
*
*   Variables
*
***/

static mutex s_mut;
static vector<MatchKey> s_matchers;
static unordered_map<Endpoint, EndpointInfo> s_endpoints;


/****************************************************************************
*
*   EndpointInfo
*
***/

//===========================================================================
void EndpointInfo::onListenStop() {}

//===========================================================================
std::unique_ptr<ISocketNotify> EndpointInfo::onListenCreateSocket() {
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
*   Internal API
*
***/

namespace {
class ByteMatch : public IAppSocketMatchNotify {
    bool
    OnMatch(AppSocket::Family fam, const char ptr[], size_t count, bool excl)
        override;
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

//===========================================================================
void Dim::iAppSocketInitialize() {
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
void Dim::appSocketAddListen(
    IAppSocketNotifyFactory * factory,
    AppSocket::Family fam,
    const std::string & type,
    Endpoint end) {
    bool addNew = false;
    EndpointInfo * epi = nullptr;
    {
        lock_guard<mutex> lk{s_mut};
        epi = &s_endpoints[end];
        if (epi->families.empty())
            addNew = true;
        epi->families[fam].factories.insert(make_pair(type, factory));
    }
    if (addNew)
        socketListen(epi, end);
}

//===========================================================================
void Dim::appSocketRemoveListen(
    IAppSocketNotifyFactory * factory,
    AppSocket::Family fam,
    const std::string & type,
    Endpoint end) {
    EndpointInfo * epi = nullptr;
    lock_guard<mutex> lk{s_mut};
    epi = &s_endpoints[end];
    if (!epi->families.empty()) {
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
                    //                    s_stopping.insert(s_endpoints.extract(end));
                    return;
                }
            }
        }
    }

    logMsgError() << "Remove unknown listener, " << end << ", " << fam << ", "
                  << type;
}
