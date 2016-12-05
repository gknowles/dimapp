// config_suffix.h - dim core
#pragma once

// Restore as many compiler settings as we can so they don't leak into
// the applications
#ifndef DIM_LIB_KEEP_MACROS

// clear all dim header macros so they don't leak into the application
#ifdef DIM_LIB_STANDALONE
    #undef DIM_LIB_STANDALONE
    #undef DIM_LIB_DYN_LINK

    #undef DIM_LIB_SOURCE
    #undef DIM_LIB_DECL
#endif

#ifdef _MSC_VER
    #pragma warning(pop)
#endif

#endif
