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
struct ListenKey {
    AppSocket::Family family;
    string type;
    Endpoint end;

    bool operator==(const ListenKey & other) const {
        return tie(family, type, end)
            == tie(other.family, other.type, other.end);
    }
};

class ListenNotify : public ISocketListenNotify {
public:
    void onListenStop() override;
    std::unique_ptr<ISocketNotify> onListenCreateSocket() override;

    atomic<int> m_count{0};
};

class AppSocketNotify : public ISocketNotify {
public:
    void onSocketRead(const SocketData & data) override {}
};

} // namespace

namespace std {
//===========================================================================
template <> struct hash<ListenKey> {
    size_t operator()(const ListenKey & val) const {
        size_t out = 0;
        hashCombine(out, val.family);
        hashCombine(out, hash<string>{}(val.type));
        hashCombine(out, hash<Endpoint>{}(val.end));
        return out;
    }
};
} // namespace std


/****************************************************************************
*
*   Variables
*
***/

unordered_map<AppSocket::Family, IAppSocketMatchNotify *> s_matchers;
unordered_multimap<ListenKey, IAppSocketNotifyFactory *> s_factories;
static ListenNotify s_sockNotify;


/****************************************************************************
*
*   ListenNotify
*
***/

//===========================================================================
void ListenNotify::onListenStop() {
    m_count -= 1;
}

//===========================================================================
std::unique_ptr<ISocketNotify> ListenNotify::onListenCreateSocket() {
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
    s_matchers[fam] = notify;
}

//===========================================================================
void Dim::appSocketAddListen(
    IAppSocketNotifyFactory * factory,
    AppSocket::Family fam,
    const std::string & type,
    Endpoint end) {
    ListenKey key;
    key.family = fam;
    key.type = type;
    key.end = end;
    s_factories.insert(make_pair(key, factory));
}

//===========================================================================
void Dim::appSocketRemoveListen(
    IAppSocketNotifyFactory * factory,
    AppSocket::Family fam,
    const std::string & type,
    Endpoint end) {
    ListenKey key;
    key.family = fam;
    key.type = type;
    key.end = end;
    auto ii = s_factories.equal_range(key);
    while (ii.first != ii.second) {
        if (ii.first->second == factory) {
            ii.first = s_factories.erase(ii.first);
        } else {
            ++ii.first;
        }
    }
}
