// winplatform.cpp - dim core - windows platform
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
}
