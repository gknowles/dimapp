// Copyright Glen Knowles 2015 - 2017.
// Distributed under the Boost Software License, Version 1.0.
//
// shutdown.h - dim core

#pragma once

#include "config/config.h"

namespace Dim {

//===========================================================================
// Monitor shutdown
// 
// Application shutdown happens in three phases, client, server, and finally 
// console. The intent is that client connections are shutdown, then 
// connections to servers used to process client requests, and finally 
// consoles (web, text, etc) used to monitor the server.
// 
// For each phase one call is made to every onShutdown*() handler with retry 
// false. Each handler begins shutting down and calls shutdownIncomplete() if 
// it doesn't finish immediately. After the first call is made to all handlers 
// the handlers that signaled incomplete are processed one at a time in 
// reverse order of registration. Each of these is called periodically, with 
// retry true, until it returns without calling shutdownIncomplete(). Only 
// then continuing with the next. The phase ends after all handlers have 
// completed.
//
// After a handler returns without calling shutdownIncomplete() it will not be 
// called again. 
//
// Do not block in the handler, as it prevents timers from running and things 
// from shutting down in parallel. This is especially important when it 
// involves file, socket, or other systems that may block in the OS. 
//
// NOTE: If the shutdown process takes too long (>2 minutes) the process
//       is terminated. This can be delayed with shutdownDelay().
//===========================================================================
class IShutdownNotify {
public:
    virtual ~IShutdownNotify() {}

    virtual void onShutdownClient(bool retry) {}
    virtual void onShutdownServer(bool retry) {}
    virtual void onShutdownConsole(bool retry) {}
};

// Used to register shutdown handlers
void shutdownMonitor(IShutdownNotify * cleanup);

// Called from inside shutdown handlers when they can't finish immediately.
void shutdownIncomplete();

// Reset shutdown timeout back to 2 minutes from now. Use with caution, called
// repeatedly it can delayed shutdown indefinitely (aka hang).
void shutdownDelay();

} // namespace
