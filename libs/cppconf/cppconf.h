// Copyright Glen Knowles 2015 - 2017.
// Distributed under the Boost Software License, Version 1.0.
//
// cppconf.h - dim cppconf
#pragma once


/****************************************************************************
*
*   Configuration
*
***/

//---------------------------------------------------------------------------
// Configuration of this installation, these are options that must be the
// same when building the app as when building the library.

// DIMAPP_LIB_DYN_LINK: Forces all libraries that have separate source, to be
// linked as dll's rather than static libraries on Microsoft Windows (this
// macro is used to turn on __declspec(dllimport) modifiers, so that the
// compiler knows which symbols to look for in a dll rather than in a static
// library). Note that there may be some libraries that can only be linked in
// one way (statically or dynamically), in these cases this macro has no
// effect.
//#define DIMAPP_LIB_DYN_LINK

// DIMAPP_LIB_WINAPI_FAMILY_APP: Removes all functions that rely on windows
// WINAPI_FAMILY_DESKTOP mode, such as the console and environment
// variables. Ignored for non-windows builds.
//#define DIMAPP_LIB_WINAPI_FAMILY_APP

//---------------------------------------------------------------------------
// Configuration of this compile. These options, if desired, are set by the
// application before including the library headers.

// DIMAPP_LIB_KEEP_MACROS: By default the DIMAPP_LIB_* macros defined internally
// (including in this file) are undef'd so they don't leak out to application
// code. Setting this macro leaves them available for the application to use.
// Also included are other platform specific adjustments, such as suppression
// of specific compiler warnings.

//---------------------------------------------------------------------------
// Internal use only

// DIMAPP_LIB_SOURCE: Used internally to indicate that the library itself is
// being compiled rather than user code.


/****************************************************************************
*
*   Internal
*
***/

#if defined(DIMAPP_LIB_SOURCE) && !defined(DIMAPP_LIB_KEEP_MACROS)
#define DIMAPP_LIB_KEEP_MACROS
#endif

#ifdef DIMAPP_LIB_WINAPI_FAMILY_APP
#define DIMAPP_LIB_NO_ENV
#define DIMAPP_LIB_NO_CONSOLE
#endif

#if defined _MSC_VER
#include "compiler/visualc.h"
#elif !defined(_WIN32)
#include "sysexits.h"
#endif

#ifdef DIMAPP_LIB_DYN_LINK
#if defined(_WIN32)
#ifdef DIMAPP_LIB_SOURCE
#define DIMAPP_LIB_DECL __declspec(dllexport)
#else
#define DIMAPP_LIB_DECL __declspec(dllimport)
#endif
#endif
#else
#define DIMAPP_LIB_DECL
#endif


/****************************************************************************
*
*   Universally required headers
*
***/

#include <cstddef>  // size_t, byte, NULL, offsetof
