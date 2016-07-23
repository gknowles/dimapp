// pch.h - dim core - windows platform
#include "dim.h"

#include "../intern.h"

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

#pragma comment(lib, "ws2_32.lib")

#include "winint.h"
#include "winsockint.h"
