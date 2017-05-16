// Copyright Glen Knowles 2017.
// Distributed under the Boost Software License, Version 1.0.
//
// wintls.h - dim windows platform tls
//
// Uses windows SChannel to secure appsocket connections
#pragma once

#include "cppconf/cppconf.h"

#include <string_view>


/****************************************************************************
*
*   default_delete (for unique_ptr)
*
***/

namespace std {

template<>
struct default_delete<const CERT_CONTEXT> {
    void operator()(const CERT_CONTEXT * ptr) const;
};

template<>
struct default_delete<CredHandle> {
    void operator()(CredHandle * ptr) const;
};

} // namespace


namespace Dim {


/****************************************************************************
*
*   Declarations
*
***/


/****************************************************************************
*
*   Internal APIs
*
***/

std::unique_ptr<CredHandle> iWinTlsCreateCred();

} // namespace
