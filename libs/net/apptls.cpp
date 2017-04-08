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
*   Tuning parameters
*
***/

/****************************************************************************
*
*   Declarations
*
***/

/****************************************************************************
*
*   Variables
*
***/


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
    assert(fam == AppSocket::kByte);
    return AppSocket::kSupported;
}


/****************************************************************************
*
*   Public API
*
***/

//===========================================================================
void Dim::appTlsInitialize() {
    socketAddFamily(AppSocket::kByte, &s_byteMatch);
}
