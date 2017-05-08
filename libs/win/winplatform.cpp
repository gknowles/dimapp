// Copyright Glen Knowles 2017.
// Distributed under the Boost Software License, Version 1.0.
//
// winplatform.cpp - dim windows platform
#include "pch.h"
#pragma hdrstop

using namespace std;
using namespace Dim;


/****************************************************************************
*
*   Platform
*
***/

//===========================================================================
void Dim::iPlatformInitialize() {
    winErrorInitialize();
    winIocpInitialize();
    winAppInitialize();
}

//===========================================================================
void Dim::iPlatformConfigInitialize() {
    winAppConfigInitialize();
}
