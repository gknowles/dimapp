// Copyright Glen Knowles 2017 - 2024.
// Distributed under the Boost Software License, Version 1.0.
//
// dimcli_userconfig.h - dim cppconf
#pragma once

#include "cppconf.h"

#define DIMCLI_LIB_KEEP_MACROS

// Disable these warnings
// Warning 5262 - disabled because [[fallthrough]] isn't supported by vs2015,
// which is supported by dimcli.
#pragma warning(disable: \
    5262 /* implicit fall-through occurs here; are you missing a break
            statement? Use [[fallthrough]] when a break statement is
            intentionally omitted between cases */ \
)
