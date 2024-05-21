// Copyright Glen Knowles 2016 - 2022.
// Distributed under the Boost Software License, Version 1.0.
//
// crypt.cpp - dim core
#include "pch.h"
#pragma hdrstop

using namespace std;
using namespace Dim;


/****************************************************************************
*
*   Variables
*
***/

static random_device s_rdev;


/****************************************************************************
*
*   Crypt random
*
***/

//===========================================================================
void Dim::cryptRandomBytes(void * vptr, size_t count) {
    auto * ptr = static_cast<char *>(vptr);
    unsigned val = 0;
    for (; count >= sizeof val; count -= sizeof val) {
        val = s_rdev();
        memcpy(ptr, &val, sizeof val);
        ptr += sizeof val;
    }
    if (count) {
        val = s_rdev();
        switch (count) {
        case 3:
            *ptr++ = val & 0xff; val >>= 8;
            [[fallthrough]];
        case 2:
            *ptr++ = val & 0xff; val >>= 8;
            [[fallthrough]];
        case 1:
            *ptr++ = val & 0xff;
        }
    }
}
