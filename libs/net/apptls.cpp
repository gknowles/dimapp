// Copyright Glen Knowles 2017 - 2023.
// Distributed under the Boost Software License, Version 1.0.
//
// apptls.cpp - dim net
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

class TlsSocket : public IAppSocket, public IAppSocketNotify {
public:
    ~TlsSocket();

    // Inherited via IAppSocket
    SocketInfo getInfo() const override;
    void disconnect(AppSocket::Disconnect why) override;
    void write(string_view data) override;
    void write(unique_ptr<SocketBuffer> buffer, size_t bytes) override;
    void read() override;

    // Inherited via IAppSocketNotify
    bool onSocketAccept(const AppSocketConnectInfo & info) override;
    void onSocketDisconnect() override;
    void onSocketDestroy() override;
    bool onSocketRead(AppSocketData & data) override;
    void onSocketBufferChanged(const AppSocketBufferInfo & info) override;

private:
    deque<unsigned> m_delayedReads;
    WinTlsConnHandle m_conn;
};

} // namespace


/****************************************************************************
*
*   Variables
*
***/

static bool s_dumpInbound = false;


/****************************************************************************
*
*   TlsSocket
*
***/

//===========================================================================
TlsSocket::~TlsSocket() {
    winTlsClose(m_conn);
}

//===========================================================================
SocketInfo TlsSocket::getInfo() const {
    return socketGetInfo(this);
}

//===========================================================================
void TlsSocket::disconnect(AppSocket::Disconnect why) {
    socketDisconnect(this, why);
}

//===========================================================================
void TlsSocket::write(string_view data) {
    CharBuf out;
    winTlsSend(&out, m_conn, data);
    socketWrite(this, out);
}

//===========================================================================
void TlsSocket::write(unique_ptr<SocketBuffer> buffer, size_t bytes) {
    write(string_view(buffer->data, bytes));
}

//===========================================================================
void TlsSocket::read() {
    assert(!m_delayedReads.empty());
    if (--m_delayedReads.front())
        return;
    m_delayedReads.pop_front();
    socketRead(this);
}

//===========================================================================
bool TlsSocket::onSocketAccept (const AppSocketConnectInfo & info) {
    m_conn = winTlsAccept();
    return notifyAccept(info);
}

//===========================================================================
void TlsSocket::onSocketDisconnect () {
    notifyDisconnect();
}

//===========================================================================
void TlsSocket::onSocketDestroy () {
    notifyDestroy();
}

//===========================================================================
bool TlsSocket::onSocketRead (AppSocketData & data) {
    assert(data.bytes);
    CharBuf out;
    CharBuf recv;
    auto sv = string_view(data.data, data.bytes);
    if (s_dumpInbound)
        logHexDebug(sv);
    bool success = winTlsRecv(&out, &recv, m_conn, sv);
    socketWrite(this, out);
    AppSocketData tmp;
    unsigned delayedReads = 0;
    for (auto && v : recv.views()) {
        tmp.data = (char *) v.data();
        tmp.bytes = (int) v.size();
        if (!notifyRead(tmp))
            delayedReads += 1;
    }
    if (!success)
        disconnect(AppSocket::Disconnect::kCryptError);
    if (delayedReads) {
        m_delayedReads.push_back(delayedReads);
        return false;
    }
    return true;
}

//===========================================================================
void TlsSocket::onSocketBufferChanged(const AppSocketBufferInfo & info) {
    notifyBufferChanged(info);
}


/****************************************************************************
*
*   TlsMatch
*
***/

namespace {
class TlsMatch : public IAppSocketMatchNotify {
    AppSocket::MatchType onMatch(
        AppSocket::Family fam,
        string_view view
    ) override;
};
} // namespace
static TlsMatch s_tlsMatch;

//===========================================================================
AppSocket::MatchType TlsMatch::onMatch(
    AppSocket::Family fam,
    string_view view
) {
    assert(fam == AppSocket::kTls);
    const char prefix[] = {
        22, // content-type handshake
        3,  // legacy major record version
        // 1,  // legacy minor record version
    };
    if (view.size() < size(prefix))
        return AppSocket::kUnknown;
    if (memcmp(prefix, view.data(), size(prefix)) == 0)
        return AppSocket::kPreferred;

    return AppSocket::kUnsupported;
}


/****************************************************************************
*
*   Public API
*
***/

//===========================================================================
void Dim::appTlsInitialize() {
    socketAddFamily(AppSocket::kTls, &s_tlsMatch);
    socketAddFilter<TlsSocket>(SockAddr{}, AppSocket::kTls);
}
