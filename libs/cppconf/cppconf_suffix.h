// Copyright Glen Knowles 2016 - 2022.
// Distributed under the Boost Software License, Version 1.0.
//
// cppconf_suffix.h - dim cppconf
#pragma once

// Restore as many compiler settings as we can so they don't leak into
// the application.
#ifndef DIMAPP_LIB_KEEP_MACROS

// Clear all DIMAPP_* header macros so they don't leak into the application.
#undef DIMAPP_LIB_DYN_LINK

#undef DIMAPP_LIB_SOURCE
#undef DIMAPP_LIB_DECL

#undef DIMAPP_LIB_BUILD_DEBUG

#ifdef _MSC_VER
#pragma warning(pop)
#endif

#endif
