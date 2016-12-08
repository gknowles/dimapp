// config.h - dim core
#pragma once


/****************************************************************************
*
*   Configuration
*
***/

//---------------------------------------------------------------------------
// Configuration of this installation, these are options that must be the 
// same when building the app as when building the library.

// DIM_LIB_STANDALONE: Defines this as standalone library that is not 
// being built as part of the DIM framework.
//#define DIM_LIB_STANDALONE

// DIM_LIB_DYN_LINK: Forces all libraries that have separate source, to be 
// linked as dll's rather than static libraries on Microsoft Windows (this 
// macro is used to turn on __declspec(dllimport) modifiers, so that the 
// compiler knows which symbols to look for in a dll rather than in a static 
// library). Note that there may be some libraries that can only be linked in 
// one way (statically or dynamically), in these cases this macro has no 
// effect.
//#define DIM_LIB_DYN_LINK

// DIM_LIB_WINAPI_FAMILY_APP: Removes all functions that rely on windows
// WINAPI_FAMILY_DESKTOP mode, such as the console and environment 
// variables. Ignored for non-windows builds.
//#define DIM_LIB_WINAPI_FAMILY_APP

//---------------------------------------------------------------------------
// Configuration of this compile. These options, if desired, are set by the 
// application before including the library headers.

// DIM_LIB_KEEP_MACROS: By default the DIM_LIB_* macros defined internally
// (including in this file) are undef'd so they don't leak out to application
// code. Setting this macro leaves them available for the application to use.
// Also included are other platform specific adjustments, such as suppression
// of specific compiler warnings.

// DIM_LIB_SOURCE:


/****************************************************************************
*
*   Internal
*
***/

#if defined(DIM_LIB_SOURCE) && !defined(DIM_LIB_KEEP_MACROS)
    #define DIM_LIB_KEEP_MACROS
#endif

#ifdef DIM_LIB_WINAPI_FAMILY_APP
    #define DIM_LIB_NO_ENV
    #define DIM_LIB_NO_CONSOLE
#endif

#if defined _MSC_VER
#include "compiler/visualc.h"
#else
#error "unknown compiler"
#endif

#ifdef DIM_LIB_DYN_LINK
    #if defined(_WIN32)
        #ifdef DIM_LIB_SOURCE
            #define DIM_LIB_DECL __declspec(dllexport)
        #else
            #define DIM_LIB_DECL __declspec(dllimport)
        #endif
    #endif
#else
    #define DIM_LIB_DECL
#endif
