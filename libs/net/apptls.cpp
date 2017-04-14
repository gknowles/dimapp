// Copyright Glen Knowles 2017.
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

class TlsSocket
    : public IAppSocket
    , public IAppSocketNotify 
{
public:
    void disconnect() override;
    void write(string_view data) override;
    void write(unique_ptr<SocketBuffer> buffer, size_t bytes) override;

    // IAppSocketNotify
    bool onSocketAccept(const AppSocketInfo & info) override;
    void onSocketDisconnect() override;
    void onSocketDestroy() override;
    void onSocketRead(AppSocketData & data) override;

private:
    bool m_skippedHeader{false};
};

} // namespace


/****************************************************************************
*
*   Variables
*
***/


/****************************************************************************
*
*   TlsSocket
*
***/

//===========================================================================
void TlsSocket::disconnect() {
    socketDisconnect(this);
}

//===========================================================================
void TlsSocket::write(string_view data) {
    socketWrite(this, data);
}

//===========================================================================
void TlsSocket::write(unique_ptr<SocketBuffer> buffer, size_t bytes) {
    socketWrite(this, move(buffer), bytes);
}

//===========================================================================
bool TlsSocket::onSocketAccept (const AppSocketInfo & info) {
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
void TlsSocket::onSocketRead (AppSocketData & data) {
    assert(data.bytes);
    if (!m_skippedHeader) {
        m_skippedHeader = true;
        data.data += 1;
        data.bytes -= 1;
    }

    notifyRead(data);
}


/****************************************************************************
*
*   TlsMatch
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
    assert(fam == AppSocket::kTls);
    if (view[0] == 't')
        return AppSocket::kPreferred;

    return AppSocket::kUnsupported;
}


/****************************************************************************
*
*   Shutdown monitor
*
***/

namespace {
class ShutdownNotify : public IShutdownNotify {
    void onShutdownClient(bool retry) override;
};
} // namespace
static ShutdownNotify s_cleanup;

//===========================================================================
void ShutdownNotify::onShutdownClient(bool retry) {
}


/****************************************************************************
*
*   Public API
*
***/

//===========================================================================
void Dim::appTlsInitialize() {
    shutdownMonitor(&s_cleanup);
    socketAddFamily(AppSocket::kTls, &s_byteMatch);
    socketAddFilter<TlsSocket>(Endpoint{}, AppSocket::kTls);
}
