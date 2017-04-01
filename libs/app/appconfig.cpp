// Copyright Glen Knowles 2016 - 2017.
// Distributed under the Boost Software License, Version 1.0.
//
// appconfig.cpp - dim app
#include "pch.h"
#pragma hdrstop

using namespace std;
using namespace Dim;


/****************************************************************************
*
*   ConfigNotify
*
***/



/****************************************************************************
*
*   Internal API
*
***/

//===========================================================================
void Dim::iAppConfigInitialize () {
}


/****************************************************************************
*
*   Public API
*
***/

//===========================================================================
void Dim::appConfigMonitor(std::string_view file, IAppConfigNotify * notify) {
}

//===========================================================================
void Dim::appConfigChange(
    std::string_view file, 
    IAppConfigNotify * notify // = nullptr
) {
}
