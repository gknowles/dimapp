// Copyright Glen Knowles 2017 - 2018.
// Distributed under the Boost Software License, Version 1.0.
//
// win.h - dim windows platform
#pragma once

#include "cppconf/cppconf.h"

#include <vector>

namespace Dim {


/****************************************************************************
*
*   Service
*
***/

bool winSvcInstall(
    const char path[],
    const char name[],
    const char displayName[],
    const char desc[],
    const std::vector<const char *> & deps,
    const char account[],
    const char password[]
);

} // namespace
