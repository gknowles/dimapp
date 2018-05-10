// Copyright Glen Knowles 2018.
// Distributed under the Boost Software License, Version 1.0.
//
// pch.h - resupd

// Public header
// External library public headers
#include "dimcli/cli.h"

#define DIMAPP_LIB_KEEP_MACROS
#include "app/app.h"
#include "core/core.h"
#include "file/file.h"
#include "net/net.h"

// Standard headers
#include <crtdbg.h>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <unordered_set>

// Platform headers
#pragma pack(push)
#pragma pack()
#define _WIN32_WINNT _WIN32_WINNT_WIN8
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#pragma pack(pop)

// External library internal headers
// Internal headers
#include "win/winint.h"
