// Copyright Glen Knowles 2015 - 2024.
// Distributed under the Boost Software License, Version 1.0.
//
// visualc.h - dim compiler config
#pragma once

#ifndef _CPPUNWIND
#define _HAS_EXCEPTIONS 0
#endif
//#define _ITERATOR_DEBUG_LEVEL 0

// Features that historically conflicted with some of the Windows SDK headers
//#define _HAS_STD_BYTE 0     // std::byte
//#define _HAS_STD_BOOLEAN 0  // std::boolean

#ifndef DIMAPP_LIB_KEEP_MACROS
#pragma warning(push)
#endif

#ifndef NDEBUG
#define _CRTDBG_MAP_ALLOC 1
#endif

#ifdef DIMAPP_PACK_ALIGNMENT
#pragma pack(1)

#pragma warning(disable: \
    4103 /* alignment changed after including header */ \
    4315 /* 'identifier' may not be aligned 8 as expected by the
            constructor */ \
    4366 /* The result of the unary '&' operator may be unaligned */ \
)
#endif

// Disable these warnings
#pragma warning(disable: \
    4100 /* unreferenced formal parameter */ \
    4324 /* structure was padded due to alignment specifier */ \
    4305 /* 'context': truncation from 'type1' to 'type2' */ \
    4456 /* declaration of 'identifier' hides previous local declaration */ \
    4457 /* declaration of 'identifier' hides function parameter */ \
    5030 /* attribute 'identifier' is not recognized */ \
    26451 /* Arithmetic overflow: Using operation 'op' on a %size1% byte value
            and then casting the result to a %size2% byte value. */ \
    26495 /* variable 'identifier' is uninitialized. Always initialize a
            member variable */ \
    26812 /* prefer 'enum class' over 'enum' */ \
)

// Enable these warnings
#pragma warning(default: \
    4062 /* enumerator 'identifier' in a switch of enum 'enumeration' is not
            explicitly handled by a case label */ \
    4255 /* 'function': no function prototype given: converting '()' to
            '(void)' */ \
    4263 /* 'function' : member function does not override any base class
            virtual member function */ \
    4265 /* 'class': class has virtual functions, but destructor is not
            virtual */ \
    4431 /* missing type specifier - int assumed */ \
    4471 /* a forward declaration of an unscoped enumeration must have an
            underlying type */ \
    4826 /* conversion from 'type1' to 'type2' is sign-extended */ \
    4928 /* illegal copy-initialization; more than one user-defined conversion
            has been implicitly applied */ \
    5022 /* 'type': multiple move constructors specified */ \
    5023 /* detected #pragma warning(push) with no corresponding #pragma
            warning(pop) */ \
    5032 /* detected #pragma warning(push) with no corresponding #pragma
            warning(pop) */ \
    5038 /* data member 'ident1' will be initialized after data member
            'ident2' */ \
    5041 /* 'member-name': out-of-line definition for constexpr static data
            member is not needed and is deprecated in C++17. */ \
    5240 /* 'attribute-name': attribute is ignored in this syntactic
            position */ \
    5249 /* 'bitfield' of type 'enumeration_name' has named enumerators with
            values that cannot be represented in the given bit field width of
            'bitfield_width'. */ \
    5262 /* implicit fall-through occurs here; are you missing a break
            statement? Use [[fallthrough]] when a break statement is
            intentionally omitted between cases */ \
    5266 /* 'const' qualifier on return type has no effect */ \
    5267 /* definition of implicit copy constructor/assignment operator for
            'type' is deprecated because it has a user-provided assignment
            operator/copy constructor */ \
)

#ifdef DIMAPP_LIB_DYN_LINK
// 'identifier': class 'type' needs to have dll-interface to be used
// by clients of class 'type2'
#pragma warning(disable : 4251)
#endif

// Standard aligned_alloc added in c++17, but not supported in MSVC.
#define mallocAligned(alignment, size) _aligned_malloc(size, alignment)
#define freeAligned(ptr) _aligned_free(ptr)

// [[no_unique_address]] wasn't immediately fully supported because it broke
// binary compatibility:
// https://devblogs.microsoft.com/cppblog/msvc-cpp20-and-the-std-cpp20-switch/#c20-no_unique_address
#if _MSC_VER >= 1929
#define NO_UNIQUE_ADDRESS [[no_unique_address, msvc::no_unique_address]]
#else
#define NO_UNIQUE_ADDRESS
#endif
