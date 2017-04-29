// Copyright Glen Knowles 2015 - 2017.
// Distributed under the Boost Software License, Version 1.0.
//
// visualc.h - dim compiler config
#pragma once

#ifndef _CPPUNWIND
#define _HAS_EXCEPTIONS 0
#endif
#define _ITERATOR_DEBUG_LEVEL 0

#ifndef DIM_LIB_KEEP_MACROS
#pragma warning(push)
#endif

#pragma warning(disable: \
    4100 /* unreferenced formal parameter */ \
    4324 /* structure was padded due to alignment specifier */ \
    4456 /* declaration of 'identifier' hides previous local declaration */ \
    4457 /* declaration of 'identifier' hides function parameter */ \
    4800 /* forcing value to bool 'true' or 'false' */ \
    5030 /* attribute 'identifier' is not recognized */ \
)
#pragma warning(default: \
    4265 /* 'class': class has virtual functions, but destructor is not 
            virtual */ \
    4431 /* missing type specifier - int assumed */ \
    4471 /* a forward declaration of an unscoped enumeration must have an 
            underlying type */ \
    4826 /* conversion from 'type1' to 'type2' is sign-extended */ \
) 

#ifdef DIM_LIB_DYN_LINK
// 'identifier': class 'type' needs to have dll-interface to be used
// by clients of class 'type2'
#pragma warning(disable : 4251)
#endif
