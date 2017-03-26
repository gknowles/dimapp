// Copyright Glen Knowles 2016 - 2017.
// Distributed under the Boost Software License, Version 1.0.
//
// pch.h - h2srv

#define DIM_LIB_KEEP_MACROS
#include "app/app.h"
#include "cli/cli.h"
#include "core/core.h"
#include "net/net.h"

#include <crtdbg.h>
#include <cstdio>
#include <cstdlib>
#include <experimental/filesystem>
#include <iostream>

#define _WIN32_WINNT _WIN32_WINNT_WIN8
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
