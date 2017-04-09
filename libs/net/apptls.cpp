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

class TlsSocket : public IAppSocketNotify {
public:
    // IAppSocketNotify
    bool onSocketAccept (const AppSocketInfo & info) override;
    void onSocketRead (const AppSocketData & data) override;
    void onSocketDisconnect () override;
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
bool TlsSocket::onSocketAccept (const AppSocketInfo & info) {
    return false;
}

//===========================================================================
void TlsSocket::onSocketRead (const AppSocketData & data) {
}

//===========================================================================
void TlsSocket::onSocketDisconnect () {
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
static ShutdownNotify s_cleanup;
} // namespace

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
    socketAddFilter<TlsSocket>(AppSocket::kTls, "");
}
