// Copyright Glen Knowles 2026.
// Distributed under the Boost Software License, Version 1.0.
//
// winatomic.cpp - dim windows platform
#include "pch.h"
#pragma hdrstop

using namespace std;
using namespace Dim;


/****************************************************************************
*
*   Atomics
*
***/

//===========================================================================
void * Dim::atomicLoadPtr(void ** ptr) {
    return atomicCmpXchgPtr(ptr, nullptr, nullptr);
}

//===========================================================================
void * Dim::atomicCmpXchgPtr(void ** ptr, void * newVal, void * oldVal) {
    return InterlockedCompareExchangePointer(ptr, newVal, oldVal);
}
