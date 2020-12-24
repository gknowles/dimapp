// Copyright Glen Knowles 2020.
// Distributed under the Boost Software License, Version 1.0.
//
// windebug.cpp - dim windows platform
#include "pch.h"
#pragma hdrstop

using namespace std;
using namespace Dim;


/****************************************************************************
*
*   Internal API
*
***/

//===========================================================================
void Dim::winDebugInitialize() {
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
}
