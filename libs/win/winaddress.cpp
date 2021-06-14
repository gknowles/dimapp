// Copyright Glen Knowles 2015 - 2020.
// Distributed under the Boost Software License, Version 1.0.
//
// winaddress.cpp - dim windows platform
#include "pch.h"
#pragma hdrstop

using namespace std;
using namespace Dim;


/****************************************************************************
*
*   SockAddr
*
***/

//===========================================================================
bool Dim::parse(SockAddr * end, string_view src, int defaultPort) {
    sockaddr_storage sas;
    int sasLen = sizeof sas;
    auto tmp = toWstring(src);
    if (SOCKET_ERROR == WSAStringToAddressW(
        tmp.data(),
        AF_INET,
        NULL, // protocol info
        (sockaddr *)&sas,
        &sasLen
    )) {
        if (SOCKET_ERROR == WSAStringToAddressW(
            tmp.data(),
            AF_INET6,
            NULL, // protocol info
            (sockaddr *)&sas,
            &sasLen
        )) {
            *end = {};
            return false;
        }
    }
    copy(end, sas);
    if (!end->port)
        end->port = defaultPort;
    return true;
}

//===========================================================================
namespace Dim {
ostream & operator<<(ostream & os, const SockAddr & src) {
    sockaddr_storage sas;
    copy(&sas, src);
    wchar_t tmp[256];
    DWORD tmpLen = (DWORD) size(tmp);
    if (SOCKET_ERROR == WSAAddressToStringW(
        (sockaddr *)&sas,
        sizeof sas,
        NULL, // protocol info
        tmp,
        &tmpLen
    )) {
        os << "(bad_sockaddr)";
    } else {
        os << utf8(tmp, tmpLen);
    }
    return os;
}
} // namespace


/****************************************************************************
*
*   sockaddr_storage
*
***/

//===========================================================================
void Dim::copy(sockaddr_storage * out, const SockAddr & src) {
    *out = {};
    if (!src.addr.data[0]
        && !src.addr.data[1]
        && src.addr.data[2] == kIpv4MappedAddress
    ) {
        auto ia = reinterpret_cast<sockaddr_in *>(out);
        ia->sin_family = AF_INET;
        ia->sin_port = htons((short)src.port);
        ia->sin_addr.s_addr = htonl(src.addr.data[3]);
    } else {
        auto ia = reinterpret_cast<sockaddr_in6 *>(out);
        ia->sin6_family = AF_INET6;
        ia->sin6_port = htons((short)src.port);
        ia->sin6_scope_id = htonl(src.scope);
        auto uint32addr = reinterpret_cast<uint32_t *>(ia->sin6_addr.u.Byte);
        uint32addr[0] = htonl(src.addr.data[0]);
        uint32addr[1] = htonl(src.addr.data[1]);
        uint32addr[2] = htonl(src.addr.data[2]);
        uint32addr[3] = htonl(src.addr.data[3]);
    }
}

//===========================================================================
void Dim::copy(SockAddr * out, const sockaddr_storage & storage) {
    *out = {};
    if (storage.ss_family == AF_INET) {
        auto ia = reinterpret_cast<const sockaddr_in &>(storage);
        out->port = ntohs(ia.sin_port);
        out->addr.data[2] = kIpv4MappedAddress;
        out->addr.data[3] = ntohl(ia.sin_addr.s_addr);
    } else {
        assert(storage.ss_family == AF_INET6);
        auto ia = reinterpret_cast<const sockaddr_in6 &>(storage);
        out->port = ntohs(ia.sin6_port);
        out->scope = ntohl(ia.sin6_scope_id);
        auto uint32addr = reinterpret_cast<uint32_t *>(ia.sin6_addr.u.Byte);
        out->addr.data[0] = ntohl(uint32addr[0]);
        out->addr.data[1] = ntohl(uint32addr[1]);
        out->addr.data[2] = ntohl(uint32addr[2]);
        out->addr.data[3] = ntohl(uint32addr[3]);
    }
}

//===========================================================================
size_t Dim::bytesUsed(const sockaddr_storage & storage) {
    if (storage.ss_family == AF_INET) {
        return sizeof sockaddr_in;
    } else {
        assert(storage.ss_family == AF_INET6);
        return sizeof sockaddr_in6;
    }
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
    ADDRINFO * result;
    WinError err = getaddrinfo(
        "..localmachine",
        NULL, // service name
        NULL, // hints
        &result
    );
    if (err)
        logMsgFatal() << "getaddrinfo(..localmachine): " << err;

    SockAddr end;
    sockaddr_storage sas;
    while (result) {
        if (result->ai_family == AF_INET) {
            memcpy(&sas, result->ai_addr, result->ai_addrlen);
            copy(&end, sas);
            out->push_back(end.addr);
        }
        result = result->ai_next;
    }

    // if there are no addresses toss on the loopback with the null port so we
    // can at least pretend.
    if (out->empty()) {
        (void) parse(&end, "127.0.0.1", 9);
        out->push_back(end.addr);
    }
}
