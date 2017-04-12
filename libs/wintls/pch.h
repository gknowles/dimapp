// Copyright Glen Knowles 2017.
// Distributed under the Boost Software License, Version 1.0.
//
// pch.h - dim windows platform tls

// Public header
#include "wintls/wintls.h"

// External library public headers
#include "core/core.h"

// External library internal headers
// Standard headers
// Platform headers
#define _WIN32_WINNT _WIN32_WINNT_WIN8
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#define SECURITY_WIN32
#include <Security.h>
#include <schnlsp.h>
#include <WinDNS.h>

#pragma comment(lib, "crypt32.lib")
#pragma comment(lib, "dnsapi.lib")
#pragma comment(lib, "ncrypt.lib")
#pragma comment(lib, "secur32.lib")

// Internal headers
#include "win/winint.h"
