// Copyright Glen Knowles 2017.
// Distributed under the Boost Software License, Version 1.0.
//
// pch.h - dim windows platform tls

#include "core/core.h"
#include "wintls/wintls.h"

#define _WIN32_WINNT _WIN32_WINNT_WIN8
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#define SECURITY_WIN32
#include <Security.h>
#include <schnlsp.h>

#pragma comment(lib, "crypt32.lib")
#pragma comment(lib, "ncrypt.lib")
#pragma comment(lib, "secur32.lib")

#include "win/winint.h"