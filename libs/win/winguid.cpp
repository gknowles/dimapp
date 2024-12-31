// Copyright Glen Knowles 2020 - 2022.
// Distributed under the Boost Software License, Version 1.0.
//
// winguid.cpp - dim windows platform
#include "pch.h"
#pragma hdrstop

using namespace std;
using namespace Dim;


/****************************************************************************
*
*   Guid
*
***/

//===========================================================================
Guid Dim::newGuid() {
    Guid val;
    auto result = UuidCreate((UUID *) &val);
    if (result != RPC_S_OK)
        logMsgFatal() << "UuidCreate: " << WinError{};

    return val;
}
