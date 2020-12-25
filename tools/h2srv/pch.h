// Copyright Glen Knowles 2016 - 2020.
// Distributed under the Boost Software License, Version 1.0.
//
// pch.h - h2srv

// Public header
// External library public headers
#include "dimcli/cli.h"

#define DIMAPP_LIB_KEEP_MACROS
#include "app/app.h"
#include "core/core.h"
#include "net/net.h"
#include "system/system.h"
#include "wintls/wintls.h"

// Standard headers
#include <cstdio>
#include <cstdlib>
#include <iostream>

// Platform headers
#pragma pack(push)
#pragma pack()
#define _WIN32_WINNT _WIN32_WINNT_WIN8
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#define UNICODE
#include <Windows.h>
#pragma pack(pop)

// External library internal headers
// Internal headers
