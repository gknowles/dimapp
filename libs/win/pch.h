// Copyright Glen Knowles 2015 - 2022.
// Distributed under the Boost Software License, Version 1.0.
//
// pch.h - dim windows platform

// Public header
#include "win/win.h"

// External library public headers
#include "dimcli/cli.h"

#include "app/app.h"
#include "core/core.h"
#include "file/file.h"
#include "json/json.h"
#include "net/net.h"
#include "system/system.h"

// Standard headers
#include <algorithm>
#include <atomic>
#include <cassert>
#include <codecvt>
#include <csignal>
#include <cstdlib>
#include <fcntl.h>
#include <io.h>
#include <latch>
#include <limits>
#include <list>
#include <locale>
#include <mutex>
#include <new.h>
#include <shared_mutex>
#include <system_error>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <vector>

// Platform headers
#pragma pack(push)
#pragma pack()
#define _WIN32_WINNT _WIN32_WINNT_WIN8
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#define UNICODE
#include <Windows.h>
#include <crtdbg.h>

// Must come after Windows.h
#include <AclAPI.h>
#include <CommCtrl.h> // ListView
#include <DbgHelp.h>
#include <lmcons.h>
#include <NTSecAPI.h> // LsaOpenPolicy
#include <Psapi.h> // GetProcessMemoryInfo
#include <sddl.h> // ConvertSidToStringSid
#include <shellapi.h>
#include <ShlObj_core.h> // SHGetKnownFolderPath
#include <WinDNS.h>
#include <winioctl.h>
#include <WinSock2.h>
#include <WS2tcpip.h> // getaddrinfo, GetAddrInfoEx
#include <VersionHelpers.h>

// Must come after lmcons.h
#include <LMAPIbuf.h> // NetApiBufferFree
#include <LMJoin.h> // NetGetJoinInformation

// Must come after WinSock2.h
#include <iphlpapi.h> // GetAdaptersAddresses, ParseNetworkString
#include <mstcpip.h> // SIO_LOOPBACK_FAST_PATH
#include <MSWSock.h> // Registered IO

#pragma comment(lib, "Dbghelp.lib")
#pragma comment(lib, "iphlpapi.lib") // GetAdaptersAddresses
#pragma comment(lib, "Netapi32.lib")
#pragma comment(lib, "RpcRT4.lib") // UuidCreate
#pragma comment(lib, "synchronization.lib") // WaitOnAddress
#pragma comment(lib, "Version.lib") // GetFileVersionInfo
#pragma comment(lib, "WS2_32.lib")
#pragma pack(pop)

// External library internal headers
#include "app/appint.h" // iAppQueueStartupTask
#include "core/timeint.h" // iClockGetTicks
#include "core/threadint.h" // iThreadSetName

// Internal headers
#include "win/appint.h"
#include "win/winint.h"
#include "win/winsockint.h"
