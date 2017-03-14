// visualc.h - dim compiler config
#pragma once

#ifndef _CPPUNWIND
#define _HAS_EXCEPTIONS 0
#endif
#define _ITERATOR_DEBUG_LEVEL 0

#ifndef DIM_LIB_KEEP_MACROS
#pragma warning(push)
#endif
#pragma warning(disable : 4100) // unreferenced formal parameter
#pragma warning( \
    disable : 4324) // structure was padded due to alignment specifier
#pragma warning( \
    disable : 4456) // declaration of 'elem' hides previous local declaration
#pragma warning(disable : 4800) // forcing value to bool 'true' or 'false'
#pragma warning(disable : 5030) // attribute 'identifier' is not recognized
#ifdef DIM_LIB_DYN_LINK
// 'identifier': class 'type' needs to have dll-interface to be used
// by clients of class 'type2'
#pragma warning(disable : 4251)
#endif
