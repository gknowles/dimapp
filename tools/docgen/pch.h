// Copyright Glen Knowles 2020 - 2021.
// Distributed under the Boost Software License, Version 1.0.
//
// pch.h - docgen

// Public header
// External library public headers
#include "dimcli/cli.h"

#define DIMAPP_LIB_KEEP_MACROS
#include "app/app.h"
#include "core/core.h"
#include "file/file.h"
#include "xml/xml.h"
#include "system/system.h"
#include "tools/tools.h"

// Standard headers
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <map>
#include <regex>
#include <unordered_set>

// Platform headers
// External library internal headers
// Internal headers
#include "intern.h"
