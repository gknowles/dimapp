// hash.cpp - dim services
#include "pch.h"
#pragma hdrstop

using namespace std;
using namespace Dim;


/****************************************************************************
*
*   Variables
*
***/


/****************************************************************************
*
*   Internal API
*
***/

//===========================================================================
void Dim::iHashInitialize() {
    if (sodium_init() == -1)
        logMsgCrash() << "sodium_init: failed";
}
