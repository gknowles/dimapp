// Copyright Glen Knowles 2015 - 2022.
// Distributed under the Boost Software License, Version 1.0.
//
// winaddress.cpp - dim windows platform
#include "pch.h"
#pragma hdrstop

using namespace std;
using namespace Dim;


/****************************************************************************
*
*   Helpers
*
***/

//===========================================================================
static bool parse(
    sockaddr_storage * out,
    uint16_t * port,
    uint8_t * prefixLen,
    string_view src
) {
    auto wsrc = toWstring(src);
    NET_ADDRESS_INFO info;
    DWORD type = NET_STRING_IP_ADDRESS;
    if (port) {
        type = NET_STRING_IP_SERVICE;
    } else if (prefixLen) {
        type = NET_STRING_IP_NETWORK;
    }
    WinError err = ParseNetworkString(
        wsrc.data(),
        type,
        &info,
        port,
        prefixLen
    );
    if (err) {
        if (err != ERROR_INVALID_PARAMETER)
            logMsgError() << "ParseNetworkString: " << err;
        *out = {};
        if (port)
            *port = 0;
        if (prefixLen)
            *prefixLen = (uint8_t) -1;
        return false;
    }
    static_assert(sizeof(*out) >= sizeof(info.IpAddress));
    memcpy(out, &info.IpAddress, sizeof info.IpAddress);
    return true;
}


/****************************************************************************
*
*   HostAddr
*
***/

//===========================================================================
bool Dim::parse(HostAddr * out, string_view src) {
    sockaddr_storage sas;
    if (!parse(&sas, nullptr, nullptr, src)) {
        *out = {};
        return false;
    }
    copy(out, sas);
    return true;
}

//===========================================================================
string Dim::toString(const HostAddr & addr) {
    ostringstream os;
    os << addr;
    return os.str();
}


/****************************************************************************
*
*   SockAddr
*
***/

//===========================================================================
bool Dim::parse(SockAddr * out, string_view src, int defaultPort) {
    sockaddr_storage sas;
    uint16_t port;
    if (!parse(&sas, &port, nullptr, src)) {
        port = 0;
        if (!parse(&sas, nullptr, nullptr, src)) {
            *out = {};
            return false;
        }
    }
    copy(out, sas);
    out->port = port ? port : defaultPort;
    return true;
}

//===========================================================================
string Dim::toString(const SockAddr & addr) {
    ostringstream os;
    os << addr;
    return os.str();
}


/****************************************************************************
*
*   SubnetAddr
*
***/

//===========================================================================
bool Dim::parse(SubnetAddr * out, string_view src) {
    sockaddr_storage sas;
    uint8_t prefixLen;
    if (!parse(&sas, nullptr, &prefixLen, src)) {
        *out = {};
        return false;
    }
    copy(&out->addr, sas);
    out->prefixLen = prefixLen;
    return true;
}


/****************************************************************************
*
*   sockaddr_storage
*
***/

//===========================================================================
size_t Dim::bytesUsed(const sockaddr_storage & storage) {
    if (storage.ss_family == AF_INET) {
        return sizeof sockaddr_in;
    } else {
        assert(storage.ss_family == AF_INET6);
        return sizeof sockaddr_in6;
    }
}

//===========================================================================
void Dim::copy(sockaddr_storage * out, const HostAddr & addr) {
    *out = {};
    auto ia = reinterpret_cast<sockaddr_in6 *>(out);
    ia->sin6_family = AF_INET6;
    ia->sin6_scope_id = htonl(addr.scope);
    assert(sizeof ia->sin6_addr.u.Byte == sizeof addr.data);
    memcpy(
        ia->sin6_addr.u.Byte,
        addr.data,
        size(ia->sin6_addr.u.Byte)
    );
}

//===========================================================================
void Dim::copy(HostAddr * out, const sockaddr_storage & storage) {
    *out = {};
    if (storage.ss_family == AF_INET) {
        auto & ia = reinterpret_cast<const sockaddr_in &>(storage);
        out->ipv4(ntohl(ia.sin_addr.s_addr));
    } else {
        assert(storage.ss_family == AF_INET6);
        auto & ia = reinterpret_cast<const sockaddr_in6 &>(storage);
        out->scope = ntohl(ia.sin6_scope_id);
        assert(sizeof ia.sin6_addr.u.Byte == sizeof out->data);
        memcpy(
            out->data,
            ia.sin6_addr.u.Byte,
            size(out->data)
        );
    }
}

//===========================================================================
void Dim::copy(sockaddr_storage * out, const SockAddr & src) {
    auto ia = reinterpret_cast<sockaddr_in6 *>(out);
    copy(out, src.addr);
    ia->sin6_port = htons((short)src.port);
}

//===========================================================================
void Dim::copy(SockAddr * out, const sockaddr_storage & storage) {
    auto & ia = reinterpret_cast<const sockaddr_in6 &>(storage);
    copy(&out->addr, storage);
    out->port = ntohs(ia.sin6_port);
}


/****************************************************************************
*
*   HostAddr query
*
***/

namespace {

struct QueryTask : IWinOverlappedNotify {
    ADDRINFOEXW * results{};
    ISockAddrNotify * notify{};
    HANDLE cancel{};
    int id;

    vector<SockAddr> addrs;
    WinError err{0};

    void onTask() override;
};

} // namespace

//===========================================================================
// Variables
//===========================================================================
static int s_lastCancelId;
static unordered_map<int, QueryTask> s_tasks;

//===========================================================================
// Callback
//===========================================================================
static void CALLBACK addressQueryCallback(
    DWORD error,
    DWORD bytes,
    OVERLAPPED * overlapped
) {
    auto evt = reinterpret_cast<WinOverlappedEvent *>(overlapped);
    auto task = static_cast<QueryTask *>(evt->notify);
    task->err = error;
    if (task->results) {
        ADDRINFOEXW * result = task->results;
        while (result) {
            if (result->ai_family == AF_INET) {
                SockAddr addr;
                sockaddr_storage sas{};
                memcpy(&sas, result->ai_addr, result->ai_addrlen);
                copy(&addr, sas);
                task->addrs.push_back(addr);
            }
            result = result->ai_next;
        }
        FreeAddrInfoExW(task->results);
    }
    taskPushEvent(task);
}

//===========================================================================
// QueryTask
//===========================================================================
void QueryTask::onTask() {
    if (err && err != WSA_E_CANCELLED) {
        logMsgError() << "GetAddrInfoEx: " << err;
    }
    notify->onSockAddrFound(addrs.data(), (int)addrs.size());
    s_tasks.erase(id);
}

//===========================================================================
// Public API
//===========================================================================
void Dim::addressQuery(
    int * cancelId,
    ISockAddrNotify * notify,
    string_view name,
    int defaultPort
) {
    QueryTask * task = nullptr;
    for (;;) {
        *cancelId = ++s_lastCancelId;
        auto ib = s_tasks.try_emplace(*cancelId);
        if (ib.second) {
            task = &ib.first->second;
            break;
        }
    }
    task->id = *cancelId;
    task->notify = notify;

    // if the name is the string form of an address just return the address
    SockAddr addr;
    if (parse(&addr, name, defaultPort)) {
        task->addrs.push_back(addr);
        taskPushEvent(task);
        return;
    }

    // Asynchronous completion requires wchar_t version of:
    auto wname = toWstring(name);
    auto wport = to_wstring(defaultPort);

    // extract non-default port if present in name
    size_t pos = wname.rfind(L':');
    if (pos != string::npos) {
        wport = wname.substr(pos + 1);
        wname.resize(pos);
    }

    WinError err = GetAddrInfoExW(
        wname.c_str(),
        wport.c_str(),
        NS_ALL,
        NULL, // namespace provider id
        NULL, // hints
        &task->results,
        NULL, // timeout
        &task->overlapped(),
        &addressQueryCallback,
        &task->cancel
    );
    if (err != ERROR_IO_PENDING)
        addressQueryCallback(err, 0, &task->overlapped());
}

//===========================================================================
void Dim::addressCancelQuery(int cancelId) {
    auto it = s_tasks.find(cancelId);
    if (it != s_tasks.end())
        GetAddrInfoExCancel(&it->second.cancel);
}

//===========================================================================
void Dim::addressGetLocal(vector<HostAddr> * out) {
    out->resize(0);

    ULONG resLen = 15'000;
    string buf;
    for (auto i = 0; i < 3; ++i) {
        buf.resize(resLen);
        auto result = (IP_ADAPTER_ADDRESSES *) buf.data();
        auto err = GetAdaptersAddresses(
            AF_UNSPEC,
            GAA_FLAG_SKIP_ANYCAST | GAA_FLAG_SKIP_MULTICAST
                | GAA_FLAG_SKIP_DNS_SERVER | GAA_FLAG_SKIP_FRIENDLY_NAME
            ,
            NULL,
            result,
            &resLen
        );
        if (!err)
            break;
        buf.resize(0);
        if (err != ERROR_BUFFER_OVERFLOW)
            logMsgFatal() << "GetAdaptersAddresses: " << WinError(err);
    }

    SockAddr sockAddr;
    auto ptr = (IP_ADAPTER_ADDRESSES *) (buf.empty() ? nullptr : buf.data());
    for (; ptr; ptr = ptr->Next) {
        auto addr = ptr->FirstUnicastAddress;
        for (; addr; addr = addr->Next) {
            if (addr->DadState == IpDadStatePreferred
                || addr->DadState == IpDadStateDeprecated
            ) {
                copy(
                    &sockAddr,
                    (sockaddr_storage &) *addr->Address.lpSockaddr
                );
                switch (sockAddr.addr.type()) {
                default:
                    break;
                case HostAddr::kLoopback:
                case HostAddr::kPrivate:
                case HostAddr::kPublic:
                    out->push_back(sockAddr.addr);
                    break;
                }
            }
        }
    }

    // If there are no addresses toss on the loopback with the null port so we
    // can at least pretend.
    if (out->empty()) {
        (void) parse(&sockAddr.addr, "127.0.0.1");
        out->push_back(sockAddr.addr);
    }

    ranges::sort(*out, [](auto & a, auto & b) {
        return a.type() > b.type()
            || a.type() == b.type() && a < b;
    });
}
