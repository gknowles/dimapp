// winsystem.cpp - dim core - windows platform
#include "pch.h"
#pragma hdrstop

using namespace std;
using namespace Dim;


/****************************************************************************
*
*   System
*
***/

//===========================================================================
void Dim::iSystemInitialize() {
    winErrorInitialize();
    winIocpInitialize();
}
