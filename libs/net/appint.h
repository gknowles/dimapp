// Copyright Glen Knowles 2015 - 2017.
// Distributed under the Boost Software License, Version 1.0.
//
// appint.h - dim net
//
// Initialize and destroy methods called by appRun()
#pragma once

namespace Dim {

// AppSocket
void iAppSocketInitialize();

// Http
void iHttpRouteInitialize();

// Socket Manager
void iSockMgrInitialize();

} // namespace
