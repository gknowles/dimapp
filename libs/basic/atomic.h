// Copyright Glen Knowles 2026.
// Distributed under the Boost Software License, Version 1.0.
//
// atomic.h - dim basic
#pragma once

#include "cppconf/cppconf.h"

namespace Dim {


/****************************************************************************
*
*   Atomic
*
***/

void * atomicLoadPtr(void ** ptr);
void * atomicCmpXchgPtr(void ** ptr, void * newVal, void * oldVal);

} // namespace
