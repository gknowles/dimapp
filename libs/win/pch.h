// Copyright Glen Knowles 2015 - 2017.
// Distributed under the Boost Software License, Version 1.0.
//
// pch.h - dim windows platform

#include "app/app.h"
#include "core/core.h"
#include "net/net.h"

#include "core/platformint.h"

#include <algorithm>
#include <atomic>
#include <cassert>
#include <limits>
#include <list>
#include <mutex>
#include <thread>
#include <unordered_map>
#include <vector>

#define _WIN32_WINNT _WIN32_WINNT_WIN8
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <WS2tcpip.h> // getaddrinfo, GetAddrInfoEx
#include <WinSock2.h>
#include <Windows.h>

// must come after WinSock2.h
#include <MSWSock.h> // Registered IO
#include <mstcpip.h> // SIO_LOOPBACK_FAST_PATH

#pragma comment(lib, "synchronization.lib") // WaitOnAddress
#pragma comment(lib, "ws2_32.lib")

#include "winint.h"
#include "winsockint.h"
