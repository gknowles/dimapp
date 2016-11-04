// address.cpp - dim services
#include "pch.h"
#pragma hdrstop

using namespace std;
using namespace Dim;


/****************************************************************************
*
*   Helpers
*
***/


/****************************************************************************
*
*   Address
*
***/

//===========================================================================
bool Address::operator==(const Address & right) const {
    return memcmp(this, &right, sizeof *this) == 0;
}

//===========================================================================
Address::operator bool() const {
    return data[3] || data[0] || data[1] || data[2];
}


/****************************************************************************
*
*   Endpoint
*
***/

//===========================================================================
bool Endpoint::operator==(const Endpoint & right) const {
    return port == right.port && addr == right.addr;
}

//===========================================================================
Endpoint::operator bool() const {
    return port || addr;
}
