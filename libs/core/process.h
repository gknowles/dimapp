// Copyright Glen Knowles 2015 - 2018.
// Distributed under the Boost Software License, Version 1.0.
//
// process.h - dim core
#pragma once

#include "cppconf/cppconf.h"

namespace Dim {


/****************************************************************************
*
*   Process exit codes
*
***/

enum {
    EX_OK = 0,

    // Microsoft specific
    EX_ABORTED = 3, // used by CRT in assert() failures and abort()

    // Start of exit codes defined by this application
    EX__APPBASE = 10,

    // On *nix the following EX_* constants are defined in <sysexits.h>
    // These are centered around why a command line utility might fail.
    EX__BASE = 64,       // base value for standard error codes
    EX_USAGE = 64,       // bad command line
    EX_DATAERR = 65,     // bad input file
    EX_NOINPUT = 66,     // missing or inaccessible input file
    EX_NOUSER = 67,      // user doesn't exist
    EX_NOHOST = 68,      // host doesn't exist (not merely connect failed)
    EX_UNAVAILABLE = 69, // failed for some other reason
    EX_SOFTWARE = 70,    // internal software error
    EX_OSERR = 71,       // some OS operation failed
    EX_OSFILE = 72,      // some operation on a system file failed
    EX_CANTCREAT = 73,   // can't create user supplied output file
    EX_IOERR = 74,       // console or file read/write error
    EX_TEMPFAIL = 75,    // temporary failure, trying again later may work
    EX_PROTOCOL = 76,    // illegal response from remote system
    EX_NOPERM = 77,      // insufficient permission
    EX_CONFIG = 78,      // configuration error
};

} // namespace
