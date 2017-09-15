// Copyright Glen Knowles 2015 - 2017.
// Distributed under the Boost Software License, Version 1.0.
//
// pch.h - tnet

#define DIMAPP_LIB_KEEP_MACROS
// Library public headers
#include "app/app.h"
#include "dimcli/cli.h"
#include "core/core.h"
#include "file/file.h"
#include "net/net.h"

// Standard headers
#include <crtdbg.h>
#include <cstdio>
#include <cstdlib>
#include <iostream>

// Platform headers
#define _WIN32_WINNT _WIN32_WINNT_WIN8
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>

// Library internal headers
// Internal headers
