// Copyright Glen Knowles 2017 - 2018.
// Distributed under the Boost Software License, Version 1.0.
//
// wintls.h - dim windows platform tls
//
// Uses windows SChannel to secure appsocket connections
#pragma once

#include "cppconf/cppconf.h"

#include "core/charbuf.h"
#include "core/handle.h"

#include <string_view>

namespace Dim {


/****************************************************************************
*
*   Public API
*
***/

void winTlsInitialize ();


struct WinTlsConnHandle : HandleBase {};

WinTlsConnHandle winTlsAccept();
void winTlsClose(WinTlsConnHandle h);

bool winTlsRecv(
    CharBuf * sendBack,
    CharBuf * received,
    WinTlsConnHandle conn,
    std::string_view src
);

void winTlsSend(
    CharBuf * send,
    WinTlsConnHandle conn,
    std::string_view src
);

} // namespace
